use strict;

package usrMods;


## checks a given string if its content is ASCII or HEX and checks also length
#  parameter needed by the function
#  checkString('string to check', 'ASCII or HEX', 'length to check')
#  returns 0 = ok, 1 = ASCII or HEX didn't fit, 2 = wrong length
sub checkString {
	my $chkStr = shift;
	my $chkCnt = shift;
	my $chkLen = shift;

	if      ($chkCnt eq 'ASCII' && $chkStr !~ /[A-Za-z0-9]/) {
		return 1;

	} elsif ($chkCnt eq 'HEX' && $chkStr !~ /[A-F0-9]/) {
		return 1;
	} 
	
	if (length($chkStr) != $chkLen) {
		return 2;
	}
	
	return 0;
}

## generates an DEC or HEX string with the given length
#  parameter needed by the function
#  randString('DEC or HEX', 'length of string')
#  returns the generated string
sub randString {
	my $chkCnt = shift;
	my $chkLen = shift;

	my $ret;
	my @chars;
	
	if      ($chkCnt eq 'H') {
		@chars = ('A'..'F', '0'..'9');	
	} elsif ($chkCnt eq 'D') {
		@chars = ('0'..'9');
		$ret = "TLU";
	}
	
	while($chkLen--){ $ret .= $chars[rand @chars] };
	return $ret;
}

## steps through the defined directory, reads every single file untill it finds the given model id ----------
my @handover;
my $dir = 'devicetypes';																					# directory with the HM device files

sub searchXMLFiles($) {
	my $hn = shift;
	
	#print "parameter: $hn\n";
	
	opendir(DIR, $dir) or die $!;																			# open the directory


	while (my $file = readdir(DIR)) {																		# step through the files
		
		next unless (-f "$dir/$file");																		# we only want files
		next unless ($file =~ m/\.xml$/);        															# use a regular expression to find files ending in .xml
	
		#-- create parser object --------------------------------------------------------------
		my $parser = XML::LibXML->new();																	# create the xml object
		my $doc    = $parser->parse_file("$dir/$file");														# open the file
		my $xc = XML::LibXML::XPathContext->new( $doc->documentElement()  );								# create parser object
		
	
		#-- get the device version ------------------------------------------------------------
		my($sections) = $xc->findnodes('/device');															# set pointer to device
		my $devVer = $sections->getAttribute('version');													# get the device version	
	
	
		#-- get the device name and id --------------------------------------------------------
		foreach $sections ($xc->findnodes('/device/supported_types/type')) {								# set pointer to type
	
			my $devName = $sections->getAttribute('name');													# get the device name	
			my $devID = $sections->getAttribute('id');														# get the device id	
			#print "$devName, $devID\n";
	
			
			my ($devIdx, $devCVal) = (0,0);
			my $devFW = 0;
	
			foreach my $param ($sections->findnodes('./parameter')) {										# set pointer to parameter
				$devIdx = $param->getAttribute('index');													# get the device index	
				
				if ($devIdx ==  '9.0') {																	# reflects the firmware
					
					my $condOP = $param->getAttribute('cond_op');
					if (( $condOP eq "GE" ) || ( $condOP eq "LE" ) || ( $condOP eq "EQ" )) {
						$devFW = eval $param->getAttribute('const_value');
						#print "$devFW\n";
					}
				}
	
				if ($devIdx != '10.0') {																	# only 10.0 is interesting
					next;
				}
	
				$devCVal = eval $param->getAttribute('const_value');										# getting the const value

				if (hex("0x$hn") == $devCVal) {
					push @handover, { modelID => "$devCVal", firmwareVer => "$devFW", file => "$dir/$file" };
				} 
				
				if ($hn) {																					# search is used from outside, therefore no output needed
					next;
				}

				#-- generating output ---------------------------------------------------------
				print sprintf("0x%.4x   0x%.2x    %-25s   %-65s", $devCVal, $devFW, $devID, $devName) ."   $file\n";
			}
		}
	}

	closedir(DIR);																							# close the directory again
	return @handover;
}
## ----------------------------------------------------------------------------------------------------------

## -- print out functions -----------------------------------------------------------------------------------
sub prnHexStr {
	my $in = shift;
	my $len = shift;
	#$len = "%." .$len 
		
	$in = sprintf("%.".$len."x", $in);
	$in =~ s/(..)/0x$&,/g;
	return $in;
}
sub prnASCIIStr {
	my $in = shift;
	$in =~ s/(.)/'$&',/g;
	return $in;
}

sub printDefaltTable {
	my %dT = %{shift()};

	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- eeprom defaults table ------------------------------------------------------------------------------------------------\n";
	print "uint16_t EEMEM eMagicByte;\n";
	print "uint8_t  EEMEM eHMID[3]  = {" .prnHexStr($dT{'hmID'},6) ."};\n";
	print "uint8_t  EEMEM eHMSR[10] = {" .prnASCIIStr($dT{'serial'}) ."};\n";
	print "\n";
	print "// if HMID and Serial are not set, then eeprom ones will be used\n";
	print "uint8_t HMID[3] = {" .prnHexStr($dT{'hmID'},6) ."};\n";
	print "uint8_t HMSR[10] = {" .prnASCIIStr($dT{'serial'}) ."}; // $dT{'serial'}\n";
	print "\n";
	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- settings of HM device for AS class -----------------------------------------------------------------------------------\n";
	print "const uint8_t devIdnt[] PROGMEM = {\n";
	print "     /* Firmware version  1 byte */  " .prnHexStr($dT{'firmwareVer'},2) ."                               // don't know for what it is good for\n";
	print "     /* Model ID          2 byte */  " .prnHexStr($dT{'modelID'},4) ."                          // model ID, describes HM hardware. Own devices should use high values due to HM starts from 0\n";
	print "     /* Sub Type ID       1 byte */  " .prnHexStr($dT{'subtypeID'},2) ."                               // not needed for FHEM, it's something like a group ID\n";
	print "     /* Device Info       3 byte */  " .prnHexStr($dT{'deviceInfo'},6) ."                     // describes device, not completely clear yet. includes amount of channels\n";
	print "};\n\n";
}

