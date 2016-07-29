use strict;
use warnings;
use XML::LibXML;
use Data::Dumper::Simple;
use JSON::XS;
use Time::HiRes;
 
my $DEBUG=1;
my $CONFIG_FROM_PERL_FILE=1;
my $CONFIG_FROM_JSON_FILE=0;
my $CONFIG_FROM_JSON_HANDLE=0;

my $AUTO_FIRMWARE_FROM_XML_FILE=1;

use subs qw(DEBUG);
use Time::HiRes qw(time);

sub get_bits_from_size($);
sub get_number_from_index($);
sub get_startbit_from_index($);


## ------------- import constants ---------------------------------------------------------------------------
use devDefinition;
my %cType             =usrRegs::usr_getHash("configType");



## ------------- reading config and generating device header ------------------------------------------------
DEBUG "Debug enabled";
DEBUG "Config comes from " ,($CONFIG_FROM_PERL_FILE)?"CONFIG_FROM_PERL_FILE":"" ,($CONFIG_FROM_JSON_FILE)?"CONFIG_FROM_JSON_FILE":"",($CONFIG_FROM_JSON_HANDLE)?"CONFIG_FROM_JSON_HANDLE":"","\n";


my $deviceConfig;
if ($CONFIG_FROM_PERL_FILE) {
  
	$deviceConfig = usrRegs::get_perl_config();										# get content from perl config file
	
} elsif ($CONFIG_FROM_JSON_FILE) {
  
	my $deviceConfig = load_file_by_name("config.json");							# get content from a json config file
	$deviceConfig = decode_json($deviceConfig);										# decode a json config stored in a variable
	
} elsif ($CONFIG_FROM_JSON_HANDLE) {

	## todo: get content from handle
	
}

if (length($deviceConfig) == 0) {													# check if we have some config, otherwise exit
	DEBUG "read input - " ,length($deviceConfig), " bytes\n";
	exit 1;
}


## ---------- checking basic informations -------------------------------------------------------------------
# serial - check content, only A-Z, a-z, 0-9 allowed 
my $ret = checkString($deviceConfig->{'base_config'}{'serial'}, 'A', 10);
if ($ret != 0) {
	DEBUG "generating new serial...";
	$cType{'serial'} = randString('D',7);
}

# hm id check 
if (length($deviceConfig->{'base_config'}{'hmID'}) == 0) {
	DEBUG "generating new hm ID...";
	$deviceConfig->{'base_config'}{'hmID'} = sprintf("%04x%02x", rand(0xFFFF), rand(0xFF) );
}

# model id is mandatory, if 0 then exit
if (length($deviceConfig->{'base_config'}{'modelID'}) == 0) {
	DEBUG "model ID empty, exit...";
	exit 1;
}

# check if we will find a version in the hm config directory
my $startTime = time;
my @fileList = searchXMLFiles($deviceConfig->{'base_config'}{'modelID'});
for my $href ( @fileList ) {													# some debug
    DEBUG "Was searching for the modelID ", $deviceConfig->{'base_config'}{'modelID'}, " and found something in the hm config files";
    DEBUG sprintf("modelID: %.4x, FW: %.2x, File: %-25s", $href->{'modelID'}, $href->{'firmwareVer'},$href->{'file'});
	DEBUG "Search took ", sprintf("%.2f", time - $startTime), " seconds";
}

## -- firmware version check 
# if we found something we have to check the firmware, in some cases hm uses more than one version in a file
# there are two options, automatic choose, which is default, or you can enable manual choose while enabled per flag

my $numArr= scalar @fileList;

if      (($numArr > 1) && (!$AUTO_FIRMWARE_FROM_XML_FILE)) {
	## get some info on console and let user choose for firmware version
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
		exit 1;
	}
	
	$deviceConfig->{'base_config'}{'firmwareVer'} = $fileList[$cnlCnt]{'firmwareVer'};	
	$deviceConfig->{'base_config'}{'configFile'} = $fileList[$cnlCnt]{'file'};


} elsif (($numArr > 1) && ($AUTO_FIRMWARE_FROM_XML_FILE)) {
	## choose the highest number of firmware version while no console available
	
	# step through the array and check if the firmware version number temp stored is higher than the current one
	$deviceConfig->{'base_config'}{'firmwareVer'} = $fileList[0]{'firmwareVer'};	
	$deviceConfig->{'base_config'}{'configFile'} = $fileList[0]{'file'};
	
	for(my $i=1; $i < $numArr; $i++) {
		if ($fileList[$i]{'firmwareVer'} > $deviceConfig->{'base_config'}{'firmwareVer'}) {
			$deviceConfig->{'base_config'}{'firmwareVer'} = $fileList[$i]{'firmwareVer'};	
			$deviceConfig->{'base_config'}{'configFile'} = $fileList[$i]{'file'};
		}
	}
	

} elsif ($numArr == 1) {

	$deviceConfig->{'base_config'}{'firmwareVer'} = $fileList[0]{'firmwareVer'};
	$deviceConfig->{'base_config'}{'configFile'} = $fileList[0]{'file'};
	
} 


## -- generating channel address table 
my %regTable;
# -- open file and store handle
my $xmlParser = XML::LibXML->new();													# create the xml object

# -- 0x0A - 0x0C mandatory for the master ID so add
$regTable{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0' };
$regTable{'00 00 0x0b.0'}  = { 'idx' => '0x0b.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_2', 'type' => 'integer', 'interface' => 'config', 'index' => '11', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0'  };
$regTable{'00 00 0x0c.0'}  = { 'idx' => '0x0c.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_3', 'type' => 'integer', 'interface' => 'config', 'index' => '12', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0'  };

if ($deviceConfig->{'base_config'}{'configFile'}) {
	## different ways on getting information, if the modelID refers to a xml config file, we will get the required information out there
	DEBUG "reading config from: ", $deviceConfig->{'base_config'}{'configFile'};

	# - we start to fill the extended config information
	
	# - now getting the master channel information
	my $xmlDoc    = $xmlParser->parse_file($deviceConfig->{'base_config'}{'configFile'});# open the file
	my $xmlObj    = XML::LibXML::XPathContext->new( $xmlDoc->documentElement() );	# create parser object
	
	# - finde the respective node and list the paramsets
	foreach my $xmlNode ( $xmlObj->findnodes('/device/paramset') ) {	
		my $xml_type  = $xmlNode->getAttribute('type');
		my $xml_cnl=0;
		if ( $xml_type ne 'MASTER') {next;};

		foreach my $xmlParam ( $xmlNode->findnodes('./parameter')) {
			my $xml_id = $xmlParam->getAttribute('id');
			DEBUG "   $xml_id";
			
			my ($xml_phy) = $xmlParam->findnodes('./physical');						# lets go for the physical node
			# <physical type="integer" interface="config" list="0" index="24" size="0.1"/>
			my $xml_phy_type = $xml_phy->getAttribute('type');
			my $xml_phy_list = $xml_phy->getAttribute('list');
			my $xml_phy_index = $xml_phy->getAttribute('index');
			my $xml_phy_size = $xml_phy->getAttribute('size');
			my $xml_phy_interface = $xml_phy->getAttribute('config');
			
			my ($xml_log) = $xmlParam->findnodes('./logical');						# lets go for the logical node
			# <logical type="boolean" default="false"/>
			# <logical type="float" min="0.3" max="1.8" default="0.4" unit="s"/>
			my $xml_log_type = $xml_log->getAttribute('type');
			my $xml_log_default = $xml_log->getAttribute('default');

			my ($xml_con) = $xmlParam->findnodes('./conversion');					# lets go for the conversation node
			# <conversion type="float_integer_scale" factor="10" offset="-0.3"/>
			my $xml_con_type = ($xml_con)?$xml_log->getAttribute('type'):"";
			my $xml_con_factor = ($xml_con)?$xml_log->getAttribute('factor'):"";
			my $xml_con_offset = ($xml_con)?$xml_log->getAttribute('offset'):"";

			#$regTable{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0' };
 			my $idx = sprintf( "0x%.2x.%d" , get_number_from_index($xml_phy_index), get_startbit_from_index($xml_phy_index) );
 			my $idIdx = sprintf( "%.2d %.2d %s" , $xml_cnl, $xml_phy_list, $idx ); 

 			$regTable{$idIdx}{'orig'}    = $xmlParam;

 			$regTable{$idIdx}{'idx'}     = $idx;
 			$regTable{$idIdx}{'cnl'}     = $xml_cnl;
 			$regTable{$idIdx}{'lst'}     = $xml_phy_list;
 			$regTable{$idIdx}{'type'}    = $xml_phy_type;
 			$regTable{$idIdx}{'index'}   = get_number_from_index($xml_phy_index);
 			$regTable{$idIdx}{'bit'}     = get_startbit_from_index($xml_phy_index);
 			$regTable{$idIdx}{'size'}    = get_bits_from_size($xml_phy_size);
 			$regTable{$idIdx}{'id'}      = $xml_id;
			$regTable{$idIdx}{'log_type'}= $xml_log_type;

 			## check the type of the default value in logical section, if it is int then copy it, 
 			## boolean, float and other special types need a conversation
 			if ($xml_log_type eq "boolean") {
	 			$regTable{$idIdx}{'log_def'} = 1 if (lc($xml_log_default) eq "true");
	 			$regTable{$idIdx}{'log_def'} = 0 if (lc($xml_log_default) eq "false");

 			} elsif ($xml_log_type eq "integer") {
	 			$regTable{$idIdx}{'log_def'} = $xml_log_default;

 			}
		}

	}
	
	# - now the channel specific ones
	
} else {
	## otherwise we take the extended config and the linkset.xml file
	
}



DEBUG Dumper(%regTable);

exit;


sub get_bits_from_size($) {#####################
	my $size = shift;
	my ( $int, $rest ) = split /\./, $size, 2;
	$int  = 0 if( !defined($int) || length($int) == 0 );
	$rest = 0 if( !defined($rest) || length($rest) == 0);
	return ($int*8)+$rest;
}

sub get_number_from_index($) {#####################
	my $index = shift;
	my ( $int, $rest ) = split /\./, $index, 2;
	$int  = 0 if( !defined($int) || length($int) == 0 );
	return $int;
}

sub get_startbit_from_index($) {#####################
	my $index = shift;
	my ( $int, $rest ) = split /\./, $index, 2;
	$rest = 0 if( !defined($rest) || length($rest) == 0);
	return $rest;
}




my %deviceProperties ;

## some checkups on config file



##	/*
##	* HMID, Serial number, HM-Default-Key, Key-Index
##	*/
##	const uint8_t HMSerialData[] PROGMEM = {
##		/* HMID */            0x5d,0xa8,0x79,
##		/* Serial number */   'H','B','r','e','m','o','t','e','0','1',   ## HBremote01
##		/* Default-Key */     HM_DEVICE_AES_KEY,
##		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX
##	};

##	/*
##	* Settings of HM device
##	* firmwareVersion: The firmware version reported by the device
##	*                  Sometimes this value is important for select the related device-XML-File
##	*
##	* modelID:         Important for identification of the device.
##	*                  @See Device-XML-File /device/supported_types/type/parameter/const_value
##	*
##	* subType:         Identifier if device is a switch or a blind or a remote
##	* DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
##	*                  Other bytes not known.
##	*                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
##	*/
##	const uint8_t devIdnt[] PROGMEM = {
##		/* firmwareVersion 1 byte */  0x11,
##		/* modelID         2 byte */  0x00,0xa9,
##		/* subTypeID       1 byte */  0x40,
##		/* deviceInfo      3 byte */  0x06, 0x00, 0x00,
##	};


## ----------------------------------------------------------------------------------------------------------
## ---------- checking basic informations -------------------------------------------------------------------

## ---------- serial check -----------------------
# serial - check content, only A-Z, a-z, 0-9 allowed 
$ret = checkString($cType{'serial'}, 'A', 10);
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
 @fileList          =searchXMLFiles($cType{'modelID'});
for my $href ( @fileList ) {													# some debug
    print sprintf("modelID: %.4x, FW: %.2x, File: %-25s\n", $href->{'modelID'}, $href->{'firmwareVer'},$href->{'file'});
}


## ---------- firmware version check -------------
# found more than one file, lets choose
# if we found one file take over the firmware
 $numArr= scalar @fileList;

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
$cnlTypeA{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0' };
$cnlTypeA{'00 00 0x0b.0'}  = { 'idx' => '0x0b.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_2', 'type' => 'integer', 'interface' => 'config', 'index' => '11', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0'  };
$cnlTypeA{'00 00 0x0c.0'}  = { 'idx' => '0x0c.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_3', 'type' => 'integer', 'interface' => 'config', 'index' => '12', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0'  };

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
		#print "$rLKey  x: $rL{$rLKey}{'type'}  \n";															# some debug

		# AES flag per channel
		$cnlTypeA{sprintf("%.2d %.2d %s", $rLKey, 1, '0x08.0')}  = { 'idx' => '0x08.0', 'cnl' => $rLKey, 'lst' => '1', 'id' => 'AES_FLAG', 'type' => 'integer', 'interface' => 'config', 'index' => '8', 'bit' => '0', 'size' => '1', 'log_type' => 'integer', 'log_def' => '0'  };

		# step through the referer list
		foreach my $xPrms ($xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/channel/paramset/subset')) {	
			my $secName = $xPrms->getAttribute('ref');
			#print "$rLKey:   $secName\n";															# some debug
			
			# step through the single items
			my @xa = $xO->findnodes('/xmlSet/'.$rL{$rLKey}{'type'}.'/paramset[@id="'.$secName.'"]/parameter/@id');
			#print "  @xa\n";																		# some debug

			## now, as we have a list in the array, step through the array and get the registers
			foreach my $xName (@xa) {
				$xName =~ s/id="|\"|\s//g;															# filter the string
				#print "$xName\n";																	# some debug
				
				# now it is about populating the $cnlTypeA
				%rO = getParamSet($xO, $rL{$rLKey}{'type'}, 'id', $secName, $xName);
				next if (!%rO);
				
				# size > 8 checken - start with highest value and step down by 8 bit
				for (my $i = $rO{'size'}; $i > 0; $i -= 8 ) {
					#print " $i  $rO{'size'} $rO{'idx'} hab dich \n";
					$cnlTypeA{sprintf("%.2d %.2d %s", $rLKey, $rO{'lst'}, $rO{'idx'})}  = { 'cnl' => $rLKey, %rO };

					
					if ($rO{'size'} > 8 ) {
						#print "rO: $rO{'log_def'}  rX: " .int($rO{'log_def'} & 0xff) ."  rN: " .int($rO{'log_def'} >> 8) ."\n";

						$cnlTypeA{sprintf("%.2d %.2d %s", $rLKey, $rO{'lst'}, $rO{'idx'})}{'size'} = 8;
						$cnlTypeA{sprintf("%.2d %.2d %s", $rLKey, $rO{'lst'}, $rO{'idx'})}{'log_def'} = int($rO{'log_def'} & 0xff);

						$rO{'size'} -= 8;
						$rO{'index'} += 1;
						$rO{'idx'} = sprintf("0x%.2x.%d", $rO{'index'}, $rO{'bit'});

						$rO{'log_def'} = int($rO{'log_def'} >> 8); 
					}
				}
				
			}

		}
	}

}


# -- register should be completed per channel, now sort out the slice address and channel device list table
my $lastIdx = 0; my $lastCnl = -1; 


foreach my $test (sort keys %cnlTypeA) {														# some debug
	#$lastIdx = 0    if($lastCnl != $cnlTypeA{$test}{'cnl'});
	


	#print "idx: $test: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'lst'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}  \n";
	#print "idx: $test: $cnlTypeA{$test}{'cnl'}  type: $cnlTypeA{$test}{'type'} index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'} lT: $cnlTypeA{$test}{'log_type'}  lD: $cnlTypeA{$test}{'log_def'} id: $cnlTypeA{$test}{'id'}  \n";
	#print "cnl: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'lst'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}   logType: $cnlTypeA{$test}{'log_type'}  logDef: $cnlTypeA{$test}{'log_def'}  \n"
	

	# push the defaults in an array
	if($lastIdx == $cnlTypeA{$test}{'index'}) {
		my $lastX = @{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{ 'defSet'} } - 1;
		my $lastV = @{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{ 'defSet'} }[$lastX];
		my $newV = $lastV + ($cnlTypeA{$test}{'log_def'} << $cnlTypeA{$test}{'bit'});
		#print "length: $lastX  content: $lastV new: $newV \n";	
		@{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{ 'defSet'} }[$lastX] = $newV;
		
	} else {
		#print "push " .int($cnlTypeA{$test}{'log_def'} << $cnlTypeA{$test}{'bit'}) ."\n";
		push @{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{ 'defSet'} }, $cnlTypeA{$test}{'log_def'} << $cnlTypeA{$test}{'bit'};
		
	}
	#print "idx: $test: $cnlTypeA{$test}{'cnl'}  type: $cnlTypeA{$test}{'type'} index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'} lT: $cnlTypeA{$test}{'log_type'}  lD: $cnlTypeA{$test}{'log_def'} id: $cnlTypeA{$test}{'id'}  \n\n";
	


	# push the register content into an array
	next              if($lastIdx == $cnlTypeA{$test}{'index'});
	push @{ $cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{ 'regSet'} }, $cnlTypeA{$test}{'index'};
	$cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{'cnl'} = $cnlTypeA{$test}{'cnl'};
	$cnlType{ sprintf("%.2d %.2d", $cnlTypeA{$test}{'cnl'}, $cnlTypeA{$test}{'lst'} ) }{'lst'} = $cnlTypeA{$test}{'lst'};

	$lastIdx = $cnlTypeA{$test}{'index'};
	$lastCnl = $cnlTypeA{$test}{'cnl'};


}

#foreach my $test (sort keys %cnlType) { 														# some debug
	#print "$test @{$cnlType{$test}{'regSet'}} \n"; 
	#print "cnl: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'lst'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}   logType: $cnlTypeA{$test}{'log_type'}  logDef: $cnlTypeA{$test}{'log_def'}  \n"
#}

	
foreach my $test (sort keys %cnlTypeA) { 														# some debug
	#print "$test: $rO{$test}  \n"; 
	#print "cnl: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'lst'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}   logType: $cnlTypeA{$test}{'log_type'}  logDef: $cnlTypeA{$test}{'log_def'}  \n"
}


# prefil cnlType with list1's - not needed any more, while list1 gets prefilled with 0x08 for AES communication
#foreach my $rLKey (sort keys %rL) {	
#	my $test = sprintf("%.2d %.2d", $rLKey, 1);
	#print "$test \n";
	
#	if (!$cnlType{$test}) {
#		$cnlType{$test}{'cnl'} = $rLKey;
#		$cnlType{$test}{'lst'} = 1;
#		$cnlType{$test}{'slcIdx'} = 0;
#		$cnlType{$test}{'slcLen'} = 0;
#		$cnlType{$test}{'phyAddr'} = 0;
#		$cnlType{$test}{'hidden'} = $rL{$rLKey}{'hidden'};	
		
		#my @x=();
		#$cnlType{$test}{'regSet'} = @x;
		#print "nicht vorhanden\n";

#	}

	

		
	#print "x:  $rL{$rLKey}{'type'}     $rLKey    $test\n";
	
	#$peers = $rL{$cnlType{$test}{'cnl'}}{'peers'}    if( ( $cnlType{$test}{'cnl'} > 0 ) && ( $cnlType{$test}{'lst'} >= 3 ) && ( $cnlType{$test}{'lst'} <= 4 ) );
	
	#$cnlType{$test}{'slcIdx'}  = $slcIdx;
	#$cnlType{$test}{'slcLen'}  = $slcLen;
	#$cnlType{$test}{'phyAddr'} = $phyAddr;
	#$cnlType{$test}{'peers'}   = $peers;

#}


# -- cleanup the array and remember start address and length
my $slcIdx = 0; my $slcLen = 0; my $phyAddr = 32;
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


# -- cleanup the channel array, find dublicates and reshape the addressing - fix the peers physical address, while phyAddr is at max from earlier function
foreach my $test (sort keys %cnlType) {
	if ( $cnlType{$test}{'lst'} == 3 || $cnlType{$test}{'lst'} == 4 ) {
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

#print Dumper( %cnlType);

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
	$xmlParser = XML::LibXML->new();																# create the xml object
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
##print "\n\n\n";
##print "#ifndef _REGISTER_h\n";
##print "   #define _REGISTER_h\n\n";

##printLoadLibs();
##printDefaltTable(\%cType);
##printChannelSliceTable(\%cnlType);
##printChannelDeviceListTable(\%cnlType);
##printPeerDeviceListTable(\%cnlType);
##printDevDeviceListTable(\%cnlType);
##printModuleTable(\%cnlType);
##printStartFunctions(\%cnlTypeA);

##print "#endif\n";

##printInfo(\%cnlType);


#print $cType{'battValue'};


## generate sub routine which could be called for first time start
## to setup peer connections, power savings, etc




## ----------------------------------------------------------------------------------------------------------

sub printLoadLibs {
	#print "   ##- load libraries -------------------------------------------------------------------------------------------------------\n";
	print "   #include <AS.h>                                                       ## the asksin framework\n";
	print "   #include \"hardware.h\"                                                 ## hardware definition\n";

	my $oldLibName ="";
	foreach my $rLKey (sort { $rL{$a}{'libName'} cmp $rL{$b}{'libName'} }  keys %rL) {	
		if ($rL{$rLKey}{'libName'} eq $oldLibName) {next;};
		print "   #include <" .$rL{$rLKey}{'libName'} .">\n";
		$oldLibName = $rL{$rLKey}{'libName'};
	}
	print "   #include \"hmkey.h\"\n";
	print "\n";


	#print "   ##- stage modules --------------------------------------------------------------------------------------------------------\n";
	print "   AS hm;                                                               ## asksin framework\n";

	$oldRLKey = "";	
	foreach my $rLKey (sort { $rL{$a}{'type'} cmp $rL{$b}{'type'} } keys %rL) {	
		if ($rL{$rLKey}{'type'} eq $oldRLKey) {next;};

		##print "\n";
		my $xLine = "   $rL{$rLKey}{'modName'} $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'maxIdxSize'}];";
		print $xLine ." "x(72-length($xLine)) ."## create instances of channel module\n";
		foreach (@{$rL{$rLKey}{'stage_modul'}}) {
			my $sLine = "   " .$_ .";";
			print $sLine ." "x(72-length($sLine)) ."## declare function to jump in\n";
		}
		$oldRLKey = $rL{$rLKey}{'type'};
	}

	print "\n";
}

sub printDefaltTable {
	my %dT = %{shift()};
	
	#print "   ##- eeprom defaults table ------------------------------------------------------------------------------------------------\n";
	print "   /*\n";
	print "   * HMID, Serial number, HM-Default-Key, Key-Index\n";
	print "   */\n";

	print "   const uint8_t HMSerialData[] PROGMEM = {\n";
	print "      /* HMID */            " .prnHexStr($dT{'hmID'},3) ."\n";
	print "      /* Serial number */   " .prnASCIIStr($dT{'serial'}) ."    ## $dT{'serial'}\n";
	print "      /* Default-Key */     HM_DEVICE_AES_KEY,\n";
	print "      /* Key-Index */       HM_DEVICE_AES_KEY_INDEX\n";
	print "   };\n\n";

	print "   /*\n";
	print "   * Settings of HM device\n";
	print "   * firmwareVersion: The firmware version reported by the device\n";
	print "   *                  Sometimes this value is important for select the related device-XML-File\n";
	print "   *\n";
	print "   * modelID:         Important for identification of the device.\n";
	print "   *                  \@See Device-XML-File /device/supported_types/type/parameter/const_value\n";
	print "   *\n";
	print "   * subType:         Identifier if device is a switch or a blind or a remote\n";
	print "   * DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.\n";
	print "   *                  Other bytes not known.\n";
	print "   *                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.\n";
	print "   */\n";
	
	print "   const uint8_t devIdnt[] PROGMEM = {\n";
	print "      /* firmwareVersion 1 byte */  " .prnHexStr($dT{'firmwareVer'},1) ."\n";
	print "      /* modelID         2 byte */  " .prnHexStr($dT{'modelID'},2) ."\n";
	print "      /* subTypeID       1 byte */  " .prnHexStr($dT{'subtypeID'},1) ."\n";
	print "      /* deviceInfo      3 byte */  " .prnHexStr($dT{'deviceInfo'},3) ."\n";
	print "   };\n\n";

}

sub printChannelSliceTable {
	my %dT = %{shift()}; my $cnt = 0;

	#print "   ##- channel slice address definition -------------------------------------------------------------------------------------\n";

	print "   /* \n";
	print "   * Register definitions\n";
	print "   * The values are offset adresses in relation to the start adress defines in cnlTbl\n";
	print "   * Register values can found in related Device-XML-File.\n";
	print "      \n";
	print "   * Spechial register list 0: 0x0A, 0x0B, 0x0C\n";
	print "   * Spechial register list 1: 0x08\n";
	print "   *  \n";
	print "   * \@See Defines.h\n";
	print "   *  \n";
	print "   * \@See: cnlTbl\n";
	print "   */ \n";
	print "   const uint8_t cnlAddr[] PROGMEM = {\n";

	foreach my $test (sort keys %dT) {
		next    if ( !$cnlType{$test}{'regSet'} );  
		next    if(!"@{$dT{$test}{'regSet'}}");	
		print "      ## channel: $dT{$test}{'cnl'}, list: $dT{$test}{'lst'} \n";
		print "      " .sprintf( "0x%.2x," x @{$dT{$test}{'regSet'}}, @{$dT{$test}{'regSet'}} )."\n";
		$cnt += scalar(@{$dT{$test}{'regSet'}});
	}
	print "   }; ## $cnt byte\n\n"; 

}

sub printChannelDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;

	#print "   ##- channel device list table --------------------------------------------------------------------------------------------\n";
	print "   /* \n";
	print "   * Channel - List translation table\n";
	print "   * channel, list, startIndex, start address in EEprom, hidden\n";
	print "   */\n";

	print "   EE::s_cnlTbl cnlTbl[] = {\n";
	print "      ## cnl, lst, sIdx, sLen, pAddr,  hidden\n";

	foreach my $test (sort keys %dT) {
		print sprintf("      {  %.1d,   %.1d,   0x%.2x, %2d,   0x%.4x, %1d, },\n", $dT{$test}{'cnl'}, $dT{$test}{'lst'}, $dT{$test}{'slcIdx'}, $dT{$test}{'slcLen'}, $dT{$test}{'phyAddr'}, $dT{$test}{'hidden'} );
		$cnt += 7;
	}
	print "   }; ## $cnt byte\n\n";
}

sub printPeerDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;
	
	#print Dumper(%dT);

	#print "##- peer device list table -----------------------------------------------------------------------------------------------\n";
	print "   /* \n";
	print "   * Peer-Device-List-Table \n";
	print "   * channel, maximum allowed peers, start address in EEprom \n";
	print "   */ \n";

	print "   EE::s_peerTbl peerTbl[] = {\n";
	print "      ## cnl, pMax, pAddr;\n";
	foreach my $test (sort keys %dT) {
		next    if ( $dT{$test}{'lst'} != 3 && $dT{$test}{'lst'} != 4 );
		#	{1, 6, 0x001a}              ##  6 * 4 =  24 (0x18)
		print sprintf("      { %.1d, %.1d, 0x%.4x, },\n", $dT{$test}{'cnl'}, $dT{$test}{'peers'}, $dT{$test}{'phyAddrPeers'} );
		$cnt += 4;
	}
	print "   }; ## $cnt byte\n\n";
}


sub printDevDeviceListTable {
	my %dT = %{shift()}; my $cnt = 0;

	my $nLsIt = scalar keys %dT;																	# get amount of list items

	my $nCnlC = 0;																					# get amount of user channels
	foreach my $test (sort keys %dT) {
		$nCnlC += 1    if ( $dT{$test}{'lst'} == 3 || $dT{$test}{'lst'} == 4 );
	}
	
	#print "##- handover to AskSin lib -----------------------------------------------------------------------------------------------\n";
	print "   /* \n";
	print "   * Device definition table \n";
	print "   * Parameter: amount of user channel\(s\), amount of lists, \n";
	print "   * pointer to device identification string and pointer to the channel table \n";
	print "   */ \n";


	print "   EE::s_devDef devDef = {\n";
	print "      $nCnlC, $nLsIt, devIdnt, cnlAddr,\n";
	print "   }; ## 6 byte\n\n";
}

sub printModuleTable {
	my %dT = %{shift()}; my $cnt = 0;

	my $nCnlC = 0;																					# get amount of user channels
	foreach my $test (sort keys %dT) {
		$nCnlC += 1    if ( $dT{$test}{'lst'} == 3 || $dT{$test}{'lst'} == 4 );
	}
	#print "##- module registrar -----------------------------------------------------------------------------------------------------\n";
	print "   /* \n";
	print "   * module registrar \n";
	print "   * size table to register and access channel modules \n";
	print "   */ \n";

	print "   RG::s_modTable modTbl[$nCnlC];\n\n";
}




sub printStartFunctions {
	my %dT = %{shift()};
	#print "##- ----------------------------------------------------------------------------------------------------------------------\n";
	#print "##- first time and regular start functions -------------------------------------------------------------------------------\n\n";
	
	print "   /** \n";
	print "   * \@brief First time and regular start functions \n";
	print "   */ \n";
	
	
	print "   void everyTimeStart(void) {\n";
	print "      /* \n";
	print "      * Place here everything which should be done on each start or reset of the device. \n";
	print "      * Typical use case are loading default values or user class configurations. \n";
	print "      */ \n\n";

	
	print "      ## init the homematic framework\n";

	print "      hm.confButton.config($cType{'confKeyMode'}, CONFIG_KEY_PCIE, CONFIG_KEY_INT);"  ." "x11  ."## configure the config button, mode, pci byte and pci bit\n";
	print "      hm.ld.init($cType{'statusLED'}, &hm);"  ." "x49  ."## set the led\n";
	print "      hm.ld.set(welcome);"  ." "x49  ."## show something\n";
	
	if ($cType{'battValue'} > 0 ) {
		print "      hm.bt.set($cType{'battValue'}, $cType{'battChkDura'});"  ." "x(52-length($cType{'battChkDura'}))  ."## set battery check, internal, 2.7 reference, measurement each hour\n";
	}

	#if ($cType{'powerMode'} > 0 ) {
		print "      hm.pw.setMode($cType{'powerMode'});"  ." "x51  ."## set power management mode\n";
	#}
	
	print "\n      ## register user modules\n";
	
	foreach my $rLKey (sort keys %rL) {	
		# get the respective list 3 or 4 for the channel
		my ($xl) = (grep { ($cnlType{$_}{'cnl'} == $rLKey) && ($cnlType{$_}{'lst'} > 1) && ($cnlType{$_}{'lst'} < 5) } keys %cnlType);
		my $xLine = "      $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'modIdx'}].regInHM($rLKey, $cnlType{$xl}{'lst'}, &hm);";
		print $xLine ." "x(72-length($xLine)) ."## register user module\n";

		foreach (@{$rL{$rLKey}{'config_modul'}}) {
			my $sLine = "      $rL{$rLKey}{'modName'}\[$rL{$rLKey}{'modIdx'}].$_;";
			print $sLine ." "x(72-length($sLine)) ."## configure user module\n";
		}
		print "\n";
	}
	
	#print "    ## don't forget to set somewhere the .config of the respective user class!\n";	
	
	#thsens.config(&initTH1, &measureTH1, &thVal);											## configure the user class and handover addresses to respective functions and variables
	#thsens.timing(0, 0, 0);																## mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change
	
	
	print "   }\n\n";

	print "   void firstTimeStart(void) {\n";
	print "      /* \n";
	print "      * place here everything which should be done on the first start or after a complete reset of the sketch\n";	
	print "      * typical usecase are default values which should be written into the register or peer database\n";	
	print "      */ \n\n";

	foreach my $test (sort keys %cnlType) {
		#print "@{$cnlType{$test}{'defSet'}}\n";
		next    if ( !$cnlType{$test}{'defSet'} );  
		next    if(!"@{$cnlType{$test}{'defSet'}}");	
		print "      const uint8_t cnl$cnlType{$test}{'cnl'}lst$cnlType{$test}{'lst'}\[\] = { \n";
		print "         " .sprintf( "0x%.2x," x @{$cnlType{$test}{'defSet'}}, @{$cnlType{$test}{'defSet'}} )."\n";
		print "      };\n\n";
	}

	print "   }\n\n";	
}


sub printInfo {
	my $xCnl = 255; my $xLst = 255; my $xCng = 0; my $i = 0;
	my $lastIndex = 0; my $lastBitEnd = 8;
	#print Dumper(%cnlTypeA);	
	
	#$cnlTypeA{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'lst' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8', 'log_type' => 'integer', 'log_def' => '0' };

	foreach my $test (sort keys %cnlTypeA) {
		$xCng = ( ($cnlTypeA{$test}{'cnl'} != $xCnl) || ($cnlTypeA{$test}{'lst'} != $xLst) )?1:0;
		
		# check if we need to fill some bits upfront of the new record, if we are still in the same struct
		if ( ($lastIndex == $cnlTypeA{$test}{'index'}) && ($lastBitEnd < $cnlTypeA{$test}{'bit'}) ) {
			print " "x6 ."uint8_t" ." "x38 .":" .($cnlTypeA{$test}{'bit'} - $lastBitEnd) .";";
			print " "x4 ."## " .sprintf("0x%.2x.%d", $cnlTypeA{$test}{'index'}, $lastBitEnd) ."\n";
		} 		

		# check if we need to fill some bits after the last record
		if ( ($lastIndex != $cnlTypeA{$test}{'index'}) && ($lastBitEnd < 8) ) {
			print " "x6 ."uint8_t" ." "x38 .":" .(8 - $lastBitEnd) .";";
			print " "x4 ."## " .sprintf("0x%.2x.%d", $lastIndex, $lastBitEnd) ."\n";
		} 		

		# channel/list has changed, double check if an open struct exists
		print " "x3 ."}; \n\n"					if ( $xCng && $i > 0);

		# channel or list has changed, print the header and fill the first bits upfront if neccasary
		if ( $xCng ) {
			print " "x3 ."struct s_cnl$cnlTypeA{$test}{'cnl'}lst$cnlTypeA{$test}{'lst'} { \n";
			#print Dumper($cnlTypeA{$test});

			print " "x6 ."uint8_t" ." "x38 .":" .(8 - $cnlTypeA{$test}{bit}) .";" if ($cnlTypeA{$test}{bit} > 0);
			print " "x4 ."## " .sprintf("0x%.2x.%d", $cnlTypeA{$test}{index}, 0) ."\n" if ($cnlTypeA{$test}{bit} > 0);
		}

		# print the single line content
		my $lineText = "uint8_t $cnlTypeA{$test}{'id'}";
		print " "x6 .$lineText ." "x(45-length($lineText)) .":" .$cnlTypeA{$test}{'size'} .";";
		print " "x4 ."## $cnlTypeA{$test}{'idx'}, " .sprintf("0x%.2x", $cnlTypeA{$test}{'log_def'}) ."\n";

		
		
		# check for last key to close the struct
		#print "$i, " .keys(%cnlTypeA) ."\n";
		if ( keys(%cnlTypeA) == $i+1 ) {
			if ( ($cnlTypeA{$test}{'bit'} + $cnlTypeA{$test}{'size'}) < 8 ) {
				print " "x6 ."uint8_t" ." "x38 .":" .(8 - ($cnlTypeA{$test}{'bit'} + $cnlTypeA{$test}{'size'}) ) .";";
				print " "x4 ."## " .sprintf("0x%.2x.%d", $cnlTypeA{$test}{'index'}, $cnlTypeA{$test}{'bit'}) ."\n";
			}
			
			print " "x3 ."}; \n\n\n";

		}
		

		# some sanity
		$lastIndex = $cnlTypeA{$test}{'index'};
		$lastBitEnd = $cnlTypeA{$test}{'bit'} + $cnlTypeA{$test}{'size'};

		$xCnl = $cnlTypeA{$test}{'cnl'};
		$xLst = $cnlTypeA{$test}{'lst'};
		$i += 1;

		#print "         " .sprintf( "0x%.2x," x @{$dT{$test}{'defSet'}}, @{$dT{$test}{'defSet'}} )."\n";
	}

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
	my $xFileHandle = shift;																							# xml object
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

	my ($section) = $xFileHandle->findnodes('/xmlSet/'.$sN.'/paramset[@'.$iT.'="'.$pN.'"]/parameter[@id="'.$iD.'"]');	# set pointer to parameter
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
	$retObj{'lst'}       = $physical->getAttribute('list');
	$retObj{'index'}     = $index;
	$retObj{'bit'}       = $startBit;
	$retObj{'size'}      = $size;


	# get out the default parameter
	# <logical type="integer" default="75" min="30" max="100" unit="&#176;C"/>
	# <logical type="float" min="0.0" max="1.0" default="0.4" unit="100%"/>

	# <logical type="option">
	#    <option id="CHARACTERISTIC_LINEAR"/>
	#    <option id="CHARACTERISTIC_SQUARE" default="true"/>
	# </logical>

	my ($logical) = $section->findnodes('./logical');														# search for the physical part and copy whole section
	my $log_type = $logical->getAttribute('type');
	my $log_def = $logical->getAttribute('default') ? $logical->getAttribute('default') : 0;

	my $log_min = $logical->getAttribute('min');
	my $log_max = $logical->getAttribute('max');
	my $newValue = 0;
	
	if ($log_type eq 'boolean') {
		$newValue = ($log_def eq 'true') ? 1:0;
		#print "boolean: default: $log_def, new: $newValue \n";

	} elsif ($log_type eq 'integer') {
		$newValue = $log_def;
		#print "$log_def \n";

	} elsif ($log_type eq 'float') {
		my ($conversation) = $section->findnodes('./conversion');														# search for the physical part and copy whole section
		my $conv = $conversation->getAttribute('type');

		if ($conv eq 'float_integer_scale') {
			my $log_factor = $conversation->getAttribute('factor') ? $conversation->getAttribute('factor'):0;
			my $log_offset = $conversation->getAttribute('offset') ? $conversation->getAttribute('offset'):0;
			
			$newValue = ($log_def + $log_offset) * $log_factor;
			#print "min: $log_min, max: $log_max, fact: $log_factor, offset: $log_offset, default: $log_def, new: $newValue \n";

		} elsif ($conv eq 'float_configtime') {
			$newValue = int(HM_encodeTime8($log_def));

			#print "$retObj{'size'}  default: $log_def, new: $newValue \n";
			#print "$logical \n\n";
		}

	} elsif ($log_type eq 'option') {
		my $i = 0;
		for my $item ( $logical->findnodes('./option') ) {
			#print $item ."\n";
			$newValue = $i    if ($item->getAttribute('default'));
			$i++;
		}
		#print "$newValue\n";
		
	} else {
		print "unknown: $log_type\n";

	}

	$retObj{'log_type'} = $log_type;
	$retObj{'log_def'} = $newValue;



	#print "id: $retObj{'id'}  default: $retObj{'ldefault'}   type: $retObj{'ltype'} \n ";




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

sub HM_encodeTime8($) {#####################
  my @culHmTimes8 = ( 0.1, 1, 5, 10, 60, 300, 600, 3600 );
  my $v = shift;
  return 0 if($v < 0.1);
  for(my $i = 0; $i < @culHmTimes8; $i++) {
    if($culHmTimes8[$i] * 32 > $v) {
      for(my $j = 0; $j < 32; $j++) {
        if($j*$culHmTimes8[$i] >= $v) {
          return sprintf("%f", $i*32+$j);
        }
      }
    }
  }

  return 255;
}

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

## -- file functions ----------------------------------------------------------------------------------------
sub load_file_by_name {
    my $file  = shift;

    open( my $fh, $file ) or die $!;
    my $content = do { local $/; <$fh> };
    close $fh;
	return $content;
}

## -- debug function ----------------------------------------------------------------------------------------
sub DEBUG {
	#my $content  = shift;
	if (!$DEBUG) {return;}
	print join("",@_) ."\n";
}
