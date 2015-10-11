use strict;
use warnings;
use XML::LibXML;


## --------------import constants----------------------------------------------------------------------------
use devDefinition;

my %cType             =usrRegs::usr_getHash("configType");




## ----------------------------------------------------------------------------------------------------------
## ---------- checking basic informations -------------------------------------------------------------------

## ---------- serial check -----------------------
# serial - check content, only A-Z, a-z, 0-9 allowed 
my $ret = checkString($cType{'serial'}, 'A', 10);
if ($ret != 0) {
	print "generating new serial...\n";
	$cType{'serial'} = randString('D',7);
}

## ---------- hm id check ------------------------
if (length($cType{'hmID'}) == 0) {
	print "generating new hm ID...\n";
	
	$cType{'hmID'} = sprintf("%04x%02x", rand(0xFFFF), rand(0xFF) );
}

## ---------- model id check ---------------------
# model id is mandatory, if 0 then exit
if (length($cType{'modelID'}) == 0) {
	print "model ID empty, exit...\n";
	exit;
}

# check if we will find a version in the hm config directory
my @fileList          =searchXMLFiles($cType{'modelID'});
#for my $href ( @fileList ) {													# some debug
#    print sprintf("modelID: %.4x, FW: %.2x, File: %-25s\n", $href->{'modelID'}, $href->{'firmwareVer'},$href->{'file'});
#}


## ---------- firmware version check -------------
# found more than one file, lets choose
# if we found one file take over the firmware
my $numArr= scalar @fileList;

if      ($numArr > 1) {
	print "\nthere is more then one device with the given model ID available, please select one...\n";
	# lets choose one line
	for(my $i=0; $i < $numArr; $i++) {
		my $href = $fileList[$i];
    	print sprintf("%d    modelID: %.4x, FW: %.2x, File: %-25s\n", $i, $href->{'modelID'}, $href->{'firmwareVer'},$href->{'file'});
	}

	print "please select a line by number and press return <default 0>: ";
	my $cnlCnt = 0;
	chomp ($cnlCnt = <STDIN>);
	if ($cnlCnt eq '') {
		$cnlCnt = 0;
	}

	if ($cnlCnt > $numArr) {
		print "out of range, exit!\n";
		exit;
	}
	
	$cType{'firmwareVer'} = $fileList[$cnlCnt]{'firmwareVer'};	

} elsif ($numArr == 1) {
	$cType{'firmwareVer'} = $fileList[$numArr]{'firmwareVer'};

} 

if ($numArr < 1) {																					# check while info is not available from an existing device
	## ---------- name ------------------------------------
	# get subtypeID from original document
	#	$cType{'subtypeID'} = int(rand(0xFFFFFF));

	## ---------- description -----------------------------

	## ---------- subtype id check ------------------------

	## ---------- deviceInfo check ------------------------

	## ---------- battValue -------------------------------	
	
	## ---------- battVisib -------------------------------
	
	## ---------- burstRx ---------------------------------
	
	## ---------- localResDis -----------------------------


}







## ----------------------------------------------------------------------------------------------------------
## ---------- generating channel address table --------------------------------------------------------------
my %cnlTypeA = ();
my %cnlType  = ();
my %rL = usrRegs::usr_getHash('regList');

## ---------- channel 0, list 0 -----------------------
# -- 0x0A - 0x0C mandatory for the master ID so add
$cnlTypeA{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8' };
$cnlTypeA{'00 00 0x0b.0'}  = { 'idx' => '0x0b.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_2', 'type' => 'integer', 'interface' => 'config', 'index' => '11', 'bit' => '0', 'size' => '8' };
$cnlTypeA{'00 00 0x0c.0'}  = { 'idx' => '0x0c.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_3', 'type' => 'integer', 'interface' => 'config', 'index' => '12', 'bit' => '0', 'size' => '8' };


if ($numArr > 0) {																					# get the information from an existing file




} else {																							# get the information from devDefinition.pm
	
	# -- open file and store handle
	my $xmlParser = XML::LibXML->new();																# create the xml object
	my $xmlDoc    = $xmlParser->parse_file("linkset.xml");											# open the file
	my $xO        = XML::LibXML::XPathContext->new( $xmlDoc->documentElement() );					# create parser object
	my %rO;																							# return object for getParamSet function
	
	# -- check local reset disable
	if ($cType{'localResDis'} == 1) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = getParamSet($xO, 'xmlMain', 'type', 'MASTER', 'LOCAL_RESET_DISABLE');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}

	# -- check battery value
	if ($cType{'battValue'} > 0) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = getParamSet($xO, 'xmlMain', 'type', 'MASTER', 'LOW_BAT_LIMIT');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}

	# -- internal keys visible
	if ($cType{'intKeysVis'} > 0) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = getParamSet($xO, 'xmlMain', 'type', 'MASTER', 'INTERNAL_KEYS_VISIBLE');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}


	## ---------- channel x, list x -----------------------
	# -- step through the channel array
	foreach my $rLKey (sort keys %rL) {	
		#print "x:  $rL{$rLKey}{'type'}\n";															# some debug

		# step through the referer list
		foreach my $xPrms ($xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/channel/paramset/subset')) {	
			my $secName = $xPrms->getAttribute('ref');
			#print "$rLKey:   $secName\n";															# some debug
			
			# step through the single items
			my @xa = $xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/paramset[@id="'.$secName.'"]/parameter/@id');
			#print "  @xa\n";																		# some debug

			# now, as we have a list in the array, step through the array and get the registers
			foreach my $xName (@xa) {
				$xName =~ s/id="|\"|\s//g;															# filter the string
				#print "$xName\n";																	# some debug
				
				# now it is about populating the $cnlTypeA
				%rO = getParamSet($xO, $rL{$rLKey}{'type'}, 'id', $secName, $xName);
				next if (!%rO);
				$cnlTypeA{sprintf("%.2d %.2d %s", $rLKey, $rO{'list'}, $rO{'idx'})}  = { 'cnl' => $rLKey, %rO };
			}
		}
	}

}


# -- register should be completed per channel, now sort out the slice address and channel device list table
my $lastIdx = 0;
foreach my $test (sort keys %cnlTypeA) {														# some debug
	#print "idx: $test: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'list'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}  \n";
	
	# push the register content into an array
	next              if($lastIdx == $cnlTypeA{$test}{'index'});
	push @{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'list'} ) }{ 'regSet'} }, $cnlTypeA{$test}{'index'};
	$cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'list'} ) }{'cnl'} = $cnlTypeA{$test}{'cnl'};
	$cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'list'} ) }{'lst'} = $cnlTypeA{$test}{'list'};
	$lastIdx = $cnlTypeA{$test}{'index'};
}
	
#foreach my $test (sort keys %cnlTypeA) { 														# some debug
#	print "$test: $rO{$test}  \n"; 
#	print "cnl: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'list'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}  \n"
#}


# -- cleanup the array and remember start address and length
my $slcIdx = 0; my $slcLen = 0; my $phyAddr = 31;
foreach my $test (sort keys %cnlType) {
	$slcLen = scalar(@{ $cnlType{$test}{'regSet'} });

	my $peers = 1;
	$peers = $rL{$cnlType{$test}{'cnl'}}{'peers'}    if( ( $cnlType{$test}{'cnl'} > 0 ) && ( $cnlType{$test}{'lst'} >= 3 ) && ( $cnlType{$test}{'lst'} <= 4 ) );
	
	$cnlType{$test}{'slcIdx'}  = $slcIdx;
	$cnlType{$test}{'slcLen'}  = $slcLen;
	$cnlType{$test}{'phyAddr'} = $phyAddr;
	$cnlType{$test}{'peers'}   = $peers;

	# check if hidden flag is set
	my $hidden = 0;
	$hidden = $rL{$cnlType{$test}{'cnl'}}{'hidden'}          if ($cnlType{$test}{'cnl'} > 0);
	$cnlType{$test}{'hidden'} = $hidden;

	$phyAddr += $peers * $slcLen;
	$slcIdx  += $slcLen;
	
	#print "$test: @{ $cnlType{$test}{'regSet'} }\n";											# some debug
	#print "cnl, lst, sIdx, sLen, pAddr\n";
	#print sprintf("%.1d, %.1d, 0x%.2x, %.1d, 0x%.4x,", $cnlType{$test}{'cnl'}, $cnlType{$test}{'lst'}, $cnlType{$test}{'slcIdx'}, $cnlType{$test}{'slcLen'}, $cnlType{$test}{'phyAddr'} ) ."\n";
}
# prefil cnlType with list1's
foreach my $rLKey (sort keys %rL) {	
	my $test = sprintf("%.2d %.2d", $rLKey, 1);
	
	if (!$cnlType{$test}) {
		$cnlType{$test}{'cnl'} = $rLKey;
		$cnlType{$test}{'lst'} = 1;
		$cnlType{$test}{'slcIdx'} = 0;
		$cnlType{$test}{'slcLen'} = 0;
		$cnlType{$test}{'phyAddr'} = 0;
		$cnlType{$test}{'hidden'} = $rL{$rLKey}{'hidden'};	
		
		#my @x=();
		#$cnlType{$test}{'regSet'} = @x;
		#print "nicht vorhanden\n";

	}

	

		
	#print "x:  $rL{$rLKey}{'type'}     $rLKey    $test\n";
	
	#$peers = $rL{$cnlType{$test}{'cnl'}}{'peers'}    if( ( $cnlType{$test}{'cnl'} > 0 ) && ( $cnlType{$test}{'lst'} >= 3 ) && ( $cnlType{$test}{'lst'} <= 4 ) );
	
	#$cnlType{$test}{'slcIdx'}  = $slcIdx;
	#$cnlType{$test}{'slcLen'}  = $slcLen;
	#$cnlType{$test}{'phyAddr'} = $phyAddr;
	#$cnlType{$test}{'peers'}   = $peers;

}


# -- cleanup the channel array, find dublicates and reshape the addressing - fix the peers physical address, while phyAddr is at max from earlier function
foreach my $test (sort keys %cnlType) {
	if ( $cnlType{$test}{'lst'} == (3 || 4) ) {
		$cnlType{$test}{'phyAddrPeers'} = $phyAddr;
		$phyAddr += $cnlType{$test}{'peers'} * 4;
		#print "xxx\n";
	}

	next          if ( !$cnlType{$test}{'regSet'} );  
	next          if (!"@{ $cnlType{$test}{'regSet'}}");										# skip empty regSets
	
	foreach my $rest (sort {$b cmp $a} keys %cnlType) {
		next      if ( !$cnlType{$rest}{'regSet'} );  
		next      if ( ($test eq $rest) || (!"@{ $cnlType{$rest}{'regSet'}}") );				# comparsion of same regset makes no sense
		
		#print "search in $rest: @{ $cnlType{$rest}{'regSet'} } \n";							# some debug
		#print "fits!! test:$test, rest:$rest \n"   if ( "@{$cnlType{$test}{'regSet'}}"  eq "@{$cnlType{$rest}{'regSet'}}" ) ;
		if ( "@{$cnlType{$test}{'regSet'}}"  eq "@{$cnlType{$rest}{'regSet'}}" ) {				# found a similarity, work on the cnlType set
			@{$cnlType{$rest}{'regSet'}} = ();													# content of regSet not needed any more
			$cnlType{$rest}{'slcIdx'} = $cnlType{$test}{'slcIdx'};								# set the slice index
		}
	}	
}

# -- prepare the reglist object with the details of the user module, which library to load and so on
	# step through the modules definition, find dublicates by sort and comparsion with the former statement 
	# eliminate the xml appreviation and print it with the include statement

	my $oldRLKey=""; my $oldModIdx;
	foreach my $rLKey (sort { $rL{$a}{'type'} cmp $rL{$b}{'type'} } keys %rL) {	
		my $cmName = "cm" .substr($rL{$rLKey}{'type'},3);
		$rL{$rLKey}{'libName'} = $cmName .".h";
		#$rL{$rLKey}{'libName'} = $rL{$rLKey}{'type'} .".h";

		$rL{$rLKey}{'modName'} = $cmName;
		#$rL{$rLKey}{'modName'} = $rL{$rLKey}{'type'};
		#$rL{$rLKey}{'modClass'} = $rL{$rLKey}{'type'};

		if ( $rL{$rLKey}{'type'} eq  $oldRLKey) {
			$oldModIdx += 1; 
		} else {
			$oldModIdx = 0; 
		}
		$rL{$rLKey}{'modIdx'} = $oldModIdx;
		$rL{$rLKey}{'maxIdxSize'} = $oldModIdx+1;
		
		$oldRLKey = $rL{$rLKey}{'type'} ;
	}
	
	# correct the max index size, sort for lib name and sort for max idx size, remember the biggest value and write it to the remaining ones
	my $xmlParser = XML::LibXML->new();																# create the xml object
	my $xmlDoc    = $xmlParser->parse_file("linkset.xml");											# open the file
	my $xO        = XML::LibXML::XPathContext->new( $xmlDoc->documentElement() );					# create parser object

	my @sortRlKey = sort { $rL{$a}{'type'} cmp $rL{$b}{'type'} || $rL{$b}{'maxIdxSize'} cmp $rL{$a}{'maxIdxSize'} } keys(%rL);
	$oldRLKey=""; my $oldMaxIdx=0;
	foreach my $rLKey ( @sortRlKey ) {	
		if ( $rL{$rLKey}{'type'} eq  $oldRLKey) {
			$rL{$rLKey}{'maxIdxSize'} = $oldMaxIdx;
		}
		$oldRLKey = $rL{$rLKey}{'type'};
		$oldMaxIdx = $rL{$rLKey}{'maxIdxSize'};
		#print $rL{$rLKey}{'modName'} ." " .$rL{$rLKey}{'modIdx'} ." " .$rL{$rLKey}{'maxIdxSize'} ."\n";

		# -- load the stage and config part from linkset.xml
			#print "x:  $rL{$rLKey}{'type'}\n";														# some debug

			# step through the function name list
			foreach my $xPrms ($xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/stage_modul/function')) {	
				my $secName = $xPrms->getAttribute('name');
				push @{$rL{$rLKey}{'stage_modul'}}, $secName;
				#print "$rLKey:   $secName\n";														# some debug
			}

			# step through the config module list
			foreach my $xPrms ($xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/config_modul/function')) {	
				my $secName = $xPrms->getAttribute('name');
				push @{$rL{$rLKey}{'config_modul'}}, $secName;
				#print "$rLKey: $rL{$rLKey}{'type'}  $secName\n";									# some debug
			}
		}	
			
		#	# step through the single items
		#	my @xa = $xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/paramset[@id="'.$secName.'"]/parameter/@id');
		#	#print "  @xa\n";																		# some debug
	

## ----------------------------------------------------------------------------------------------------------
## ---------- print register.h ------------------------------------------------------------------------------
print "\n\n\n";
printLoadLibs();
printDefaltTable(\%cType);
printChannelSliceTable(\%cnlType);
printChannelDeviceListTable(\%cnlType);
printPeerDeviceListTable(\%cnlType);
printDevDeviceListTable(\%cnlType);
printModuleTable(\%cnlType);
printStartFunctions();

#print $cType{'battValue'};


## generate sub routine which could be called for first time start
## to setup peer connections, power savings, etc








## ----------------------------------------------------------------------------------------------------------
## -- print out functions -----------------------------------------------------------------------------------
sub prnHexStr {
	my $in = shift;
	my $len = shift;
	$in = $in ."0"x(($len*2) - length($in));
		
	#$in = sprintf("%.".$len."x", $in);
	$in =~ s/(..)/0x$&,/g;
	return $in;
}
sub prnASCIIStr {
	my $in = shift;
	$in =~ s/(.)/'$&',/g;
	return $in;
}

sub printLoadLibs {
	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- load libraries -------------------------------------------------------------------------------------------------------\n";
	print "#include <AS.h>                                                         // the asksin framework\n";
	print "#include \"hardware.h\"                                                   // hardware definition\n";

	my $oldLibName ="";
	foreach my $rLKey (sort { $rL{$a}{'libName'} cmp $rL{$b}{'libName'} }  keys %rL) {	
		if ($rL{$rLKey}{'libName'} eq $oldLibName) {next;};
		print "#include <" .$rL{$rLKey}{'libName'} .">\n";
		$oldLibName = $rL{$rLKey}{'libName'};
	}
	print "\n";


	print "//- stage modules --------------------------------------------------------------------------------------------------------\n";
	print "AS hm;                                                                  // asksin framework\n";

	$oldRLKey = "";	
	foreach my $rLKey (sort { $rL{$a}{'type'} cmp $rL{$b}{'type'} } keys %rL) {	
		if ($rL{$rLKey}{'type'} eq $oldRLKey) {next;};

		print "\n";
		my $xLine = "$rL{$rLKey}{'modName'} $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'maxIdxSize'}];";
		print $xLine ." "x(72-length($xLine)) ."// create instances of channel module\n";
		foreach (@{$rL{$rLKey}{'stage_modul'}}) {
			my $sLine = $_ .";";
			print $sLine ." "x(72-length($sLine)) ."// declare function to jump in\n";
		}
		$oldRLKey = $rL{$rLKey}{'type'};
	}

	print "\n";
}

sub printStartFunctions {
	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- first time and regular start functions -------------------------------------------------------------------------------\n\n";
	
	print "void everyTimeStart(void) {\n";
	print "    // place here everything which should be done on each start or reset of the device\n";	
	print "    // typical usecase are loading default values or user class configurations\n\n";	
	
	print "    // init the homematic framework\n";

	print "    hm.confButton.config($cType{'confKeyMode'}, CONFIG_KEY_PCIE, CONFIG_KEY_INT);"  ." "x11  ."// configure the config button, mode, pci byte and pci bit\n";
	print "    hm.ld.init($cType{'statusLED'}, &hm);"  ." "x49  ."// set the led\n";
	print "    hm.ld.set(welcome);"  ." "x49  ."// show something\n";
	
	if ($cType{'battValue'} > 0 ) {
		print "    hm.bt.set($cType{'battValue'}, $cType{'battChkDura'});"  ." "x(52-length($cType{'battChkDura'}))  ."// set battery check, internal, 2.7 reference, measurement each hour\n";
	}

	#if ($cType{'powerMode'} > 0 ) {
		print "    hm.pw.setMode($cType{'powerMode'});"  ." "x51  ."// set power management mode\n";
	#}
	
	print "\n    // register user modules\n";
	
	foreach my $rLKey (sort keys %rL) {	
		# get the respective list 3 or 4 for the channel
		my ($xl) = (grep { ($cnlType{$_}{'cnl'} == $rLKey) && ($cnlType{$_}{'lst'} > 1) && ($cnlType{$_}{'lst'} < 5) } keys %cnlType);
		my $xLine = "    $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'modIdx'}].regInHM($rLKey, $cnlType{$xl}{'lst'}, &hm);";
		print $xLine ." "x(72-length($xLine)) ."// register user module\n";

		foreach (@{$rL{$rLKey}{'config_modul'}}) {
			my $sLine = "    $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'modIdx'}].$_;";
			print $sLine ." "x(72-length($sLine)) ."// configure user module\n";
		}
		print "\n";
	}
	
	#print "    // don't forget to set somewhere the .config of the respective user class!\n";	
	
	#thsens.config(&initTH1, &measureTH1, &thVal);											// configure the user class and handover addresses to respective functions and variables
	#thsens.timing(0, 0, 0);																// mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change
	
	
	print "}\n\n";

	print "void firstTimeStart(void) {\n";
	print "    // place here everything which should be done on the first start or after a complete reset of the sketch\n";	
	print "    // typical usecase are default values which should be written into the register or peer database\n\n";	
	
	print "}\n\n";	
}

sub printDefaltTable {
	my %dT = %{shift()};

	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- eeprom defaults table ------------------------------------------------------------------------------------------------\n";
	print "uint16_t EEMEM eMagicByte;\n";
	print "uint8_t  EEMEM eHMID[3]  = {" .prnHexStr($dT{'hmID'},3) ."};\n";
	print "uint8_t  EEMEM eHMSR[10] = {" .prnASCIIStr($dT{'serial'}) ."};\n";
	print "uint8_t  EEMEM eHMKEY[16] = {" .prnHexStr($dT{'hmKEY'},16) ."};\n";

	print "\n";
	print "// if HMID and Serial are not set, then eeprom ones will be used\n";
	print "uint8_t HMID[3] = {" .prnHexStr($dT{'hmID'},3) ."};\n";
	print "uint8_t HMSR[10] = {" .prnASCIIStr($dT{'serial'}) ."};          // $dT{'serial'}\n";
	print "uint8_t HMKEY[16] = {" .prnHexStr($dT{'hmKEY'},16) ."};\n";
	print "\n";
	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- settings of HM device for AS class -----------------------------------------------------------------------------------\n";
	print "const uint8_t devIdnt[] PROGMEM = {\n";
	print "    /* Firmware version  1 byte */  " .prnHexStr($dT{'firmwareVer'},1) ." "x31 ."// don't know for what it is good for\n";
	print "    /* Model ID          2 byte */  " .prnHexStr($dT{'modelID'},2) ." "x26 ."// model ID, describes HM hardware. Own devices should use high values due to HM starts from 0\n";
	print "    /* Sub Type ID       1 byte */  " .prnHexStr($dT{'subtypeID'},1) ." "x31 ."// not needed for FHEM, it's something like a group ID\n";
	print "    /* Device Info       3 byte */  " .prnHexStr($dT{'deviceInfo'},3) ." "x21 ."// describes device, not completely clear yet. includes amount of channels\n";
	print "};  // 7 byte\n\n";
}

sub printChannelSliceTable {
	my %dT = %{shift()}; my $cnt = 0;

	print "//- ----------------------------------------------------------------------------------------------------------------------\n";
	print "//- channel slice address definition -------------------------------------------------------------------------------------\n";
	print "const uint8_t cnlAddr[] PROGMEM = {\n";
	
	foreach my $test (sort keys %dT) {
		next    if ( !$cnlType{$test}{'regSet'} );  
		next    if(!"@{$dT{$test}{'regSet'}}");	
		print "    " .sprintf( "0x%.2x," x @{$dT{$test}{'regSet'}}, @{$dT{$test}{'regSet'}} )."\n";
		$cnt += scalar(@{$dT{$test}{'regSet'}});
	}
	print "};  // $cnt byte\n\n"; 
}

sub printChannelDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;

	print "//- channel device list table --------------------------------------------------------------------------------------------\n";
	print "EE::s_cnlTbl cnlTbl[] = {\n";
	print "    // cnl, lst, sIdx, sLen, pAddr, hidden\n";
	foreach my $test (sort keys %dT) {
		print sprintf("    { %.1d, %.1d, 0x%.2x, %2d, 0x%.4x, %1d, },\n", $dT{$test}{'cnl'}, $dT{$test}{'lst'}, $dT{$test}{'slcIdx'}, $dT{$test}{'slcLen'}, $dT{$test}{'phyAddr'}, $dT{$test}{'hidden'} );
		$cnt += 7;
	}
	print "};  // $cnt byte\n\n";
}

sub printPeerDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;

	print "//- peer device list table -----------------------------------------------------------------------------------------------\n";
	print "EE::s_peerTbl peerTbl[] = {\n";
	print "    // cnl, pMax, pAddr;\n";
	foreach my $test (sort keys %dT) {
		next    if ( $dT{$test}{'lst'} != (3 || 4) );
		#	{1, 6, 0x001a}              //  6 * 4 =  24 (0x18)
		print sprintf("    { %.1d, %.1d, 0x%.4x, },\n", $dT{$test}{'cnl'}, $dT{$test}{'peers'}, $dT{$test}{'phyAddrPeers'} );
		$cnt += 4;
	}
	print "};  // $cnt byte\n\n";
}

sub printDevDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;

	my $nLsIt = scalar keys %dT;																	# get amount of list items

	my $nCnlC = 0;																					# get amount of user channels
	foreach my $test (sort keys %dT) {
		$nCnlC += 1    if ( $dT{$test}{'lst'} == (3 || 4) );
	}
	
	print "//- handover to AskSin lib -----------------------------------------------------------------------------------------------\n";
	print "EE::s_devDef devDef = {\n";
	print "    $nCnlC, $nLsIt, devIdnt, cnlAddr,\n";
	print "};  // 6 byte\n\n";
}

sub printModuleTable {
	my %dT = %{shift()}; my $cnt = 0;

	my $nCnlC = 0;																					# get amount of user channels
	foreach my $test (sort keys %dT) {
		$nCnlC += 1    if ( $dT{$test}{'lst'} == (3 || 4) );
	}
	print "//- module registrar -----------------------------------------------------------------------------------------------------\n";
	print "RG::s_modTable modTbl[$nCnlC];\n\n";
}














## ----------------------------------------------------------------------------------------------------------
## ----------------------------------------------------------------------------------------------------------
## -- checks a given string if its content is ASCII or HEX and checks also length ---------------------------
#  parameter needed by the function
#  checkString('string to check', 'ASCII or HEX', 'length to check')
#  returns 0 = ok, 1 = ASCII or HEX didn't fit, 2 = wrong length
sub checkString {
	my $chkStr = shift;	my $chkCnt = shift;	my $chkLen = shift;

	return 1        if ($chkCnt eq 'ASCII' && $chkStr !~ /[A-Za-z0-9]/);
	return 1        if ($chkCnt eq 'HEX' && $chkStr !~ /[A-F0-9]/);
	return 2        if (length($chkStr) != $chkLen);
	return 0;
}

## -- generates an DEC or HEX string with the given length --------------------------------------------------
#  parameter needed by the function
#  randString('DEC or HEX', 'length of string')
#  returns the generated string
sub randString {
	my $chkCnt = shift;	my $chkLen = shift;
	my $ret; my @chars;
	
	if      ($chkCnt eq 'H') {
		@chars = ('A'..'F', '0'..'9');	
	} elsif ($chkCnt eq 'D') {
		@chars = ('0'..'9');
		$ret = "TLU";
	}
	
	while($chkLen--){ $ret .= $chars[rand @chars] };
	return $ret;
}

## -- read paramset in XML by giving filehandle, sectionname, paramset name and id -----------------------------------------------------
sub getParamSet {
	my $xO = shift;																							# xml object
	my $sN = shift;																							# section name
	my $iT = shift;
	my $pN = shift;																							# parameter set name
	my $iD = shift;																							# parameter id
	my %retObj = ();
	
	#-- get the respective parameter 
	# here we take the whole block an the details in physical
	
	#<parameter id="LOW_BAT_LIMIT">
	#	<logical type="float" min="5.0" max="15.0" default="10.5" unit="V"/>
	#	<physical type="integer" interface="config" list="0" index="18" size="1"/>
	#	<conversion type="float_integer_scale" factor="10"/>
	#</parameter>

	my ($section) = $xO->findnodes('/xmlSet/'.$sN.'/paramset[@'.$iT.'="'.$pN.'"]/parameter[@id="'.$iD.'"]');	# set pointer to parameter
	$retObj{'id'} = $section->getAttribute('id');

	# get out the parameter
	my ($physical) = $section->findnodes('./physical');														# search for the physical part and copy whole section
	return %retObj = ()       if (!$physical); 
	
	# check for the point value, and take the right part, it indicates the bit start value
	# if no point is inside the value, then assume a bit start value of 0	
	my $index; my $startBit = 0;
	
	my $rawIndex = $physical->getAttribute('index');														# get out the raw index figure
	return %retObj = ()       if (!$rawIndex); 

	my $pos = index( $rawIndex, '.');																		# search for a . value
	$startBit = substr( $rawIndex, $pos+1 )   if ( $pos > 0 );												# if we found a . value
	$rawIndex = substr( $rawIndex, 0, $pos )  if ( $pos > 0 );												# clean up the raw index
	
	# now we are checking on the raw index figure if it is hex formated
	$pos = index( $rawIndex, 'x');																			# search for hex format
	$rawIndex = hex($rawIndex)	              if ( $pos > 0 );												# if we found a hex formated string
	$index    = int($rawIndex);																				# generate a valid number
	

	# get the size attribute and format it accordingly
	my $size = $physical->getAttribute('size');
	$pos = index( $size, '.');
	$size = substr( $size, $pos+1 )          if ($pos >= 0);
	$size = $size * 8                        if ($pos < 0);

	# put all variables in the return object	
	$retObj{'physical'}  = $physical;
	$retObj{'idx'}       = sprintf('0x%.2x.%s', $index, $startBit);
	$retObj{'type'}      = $physical->getAttribute('type');
	$retObj{'interface'} = $physical->getAttribute('interface');
	$retObj{'list'}      = $physical->getAttribute('list');
	$retObj{'index'}     = $index;
	$retObj{'bit'}       = $startBit;
	$retObj{'size'}      = $size;

	# some debug
	#foreach my $test (keys %retObj) {
	#	print "$test    $retObj{$test}  \n";
	#}
	
	return %retObj;
}


## -- steps through the defined directory, reads every single file untill it finds the given model id -------

sub searchXMLFiles {
	my @handover; my $dir = 'devicetypes';																	# directory with the HM device files
	my $hn = shift;
	
	print "parameter: $hn\n";
	
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

				#-- generating output ---------------------------------------------------------
				#print sprintf("0x%.4x   0x%.2x    %-25s   %-65s", $devCVal, $devFW, $devID, $devName) ."   $file\n";

				if (hex("0x$hn") == $devCVal) {
					push @handover, { modelID => "$devCVal", firmwareVer => "$devFW", file => "$dir/$file" };
				} 
				
				if ($hn) {																					# search is used from outside, therefore no output needed
					next;
				}

				#-- generating output ---------------------------------------------------------
				#print sprintf("0x%.4x   0x%.2x    %-25s   %-65s", $devCVal, $devFW, $devID, $devName) ."   $file\n";
			}
		}
	}

	closedir(DIR);																							# close the directory again
	return @handover;
}
## ----------------------------------------------------------------------------------------------------------
