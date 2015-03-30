use strict;
use warnings;
use XML::LibXML;



## --------------import constants----------------------------------------------------------------------------
use devDefinition;
use destillRegsModules;

my %cType             =usrRegs::usr_getHash("configType");




## ---------- checking basic informations -------------------------------------------------------------------

## ---------- serial check -----------------------
# serial - check content, only A-Z, a-z, 0-9 allowed 
my $ret = usrMods::checkString($cType{'serial'}, 'A', 10);
if ($ret != 0) {
	print "generating new serial...\n";
	$cType{'serial'} = usrMods::randString('D',7);
}

## ---------- hm id check ------------------------
if ($cType{'hmID'} == 0) {
	print "generating new hm ID...\n";
	$cType{'hmID'} = int(rand(0xFFFFFF));
}

## ---------- model id check ---------------------
# model id is mandatory, if 0 then exit
if ($cType{'modelID'} == 0) {
	print "model ID empty, exit...\n";
	exit;
}

# check if we will find a version in the hm config directory
my @fileList          =usrMods::searchXMLFiles($cType{'modelID'});
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

if ($numArr > 0) {																					# get the information from an existing file




} else {																							# get the information from devDefinition.pm
	
	## ---------- channel 0, list 0 -----------------------

	# -- 0x0A - 0x0C mandatory for the master ID so add
	$cnlTypeA{'00 00 0x0a.0'}  = { 'idx' => '0x0a.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_1', 'type' => 'integer', 'interface' => 'config', 'index' => '10', 'bit' => '0', 'size' => '8' };
	$cnlTypeA{'00 00 0x0b.0'}  = { 'idx' => '0x0b.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_2', 'type' => 'integer', 'interface' => 'config', 'index' => '11', 'bit' => '0', 'size' => '8' };
	$cnlTypeA{'00 00 0x0c.0'}  = { 'idx' => '0x0c.0', 'cnl' => '0', 'list' => '0', 'id' => 'MASTER_ID_BYTE_3', 'type' => 'integer', 'interface' => 'config', 'index' => '12', 'bit' => '0', 'size' => '8' };

	# -- open file and store handle
	my $xmlParser = XML::LibXML->new();																# create the xml object
	my $xmlDoc    = $xmlParser->parse_file("linkset.xml");											# open the file
	my $xO        = XML::LibXML::XPathContext->new( $xmlDoc->documentElement() );					# create parser object
	my %rO;																							# return object for getParamSet function
	
	# -- check local reset disable
	if ($cType{'localResDis'} == 1) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = usrMods::getParamSet($xO, 'xmlMain', 'MASTER', 'LOCAL_RESET_DISABLE');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}

	# -- check battery value
	if ($cType{'battValue'} > 0) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = usrMods::getParamSet($xO, 'xmlMain', 'MASTER', 'LOW_BAT_LIMIT');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}

	# -- internal keys visible
	if ($cType{'intKeysVis'} > 0) {
		# get parameter from linkset.xml in <xmlMain>
		%rO = usrMods::getParamSet($xO, 'xmlMain', 'MASTER', 'INTERNAL_KEYS_VISIBLE');
		#foreach my $test (keys %rO) { print "$test    $rO{$test}  \n"; }							# some debug
		$cnlTypeA{"00 00 $rO{'idx'}"}  = { 'cnl' => '0', %rO };										# copy the hash
	}



	# -- step through the channel array
	my %rL = usrRegs::usr_getHash('regList');
	foreach my $rLKey (sort keys %rL) {	
		print "xx: $rLKey\n";
		
		# getting all keys for the respective device file
		#$rLKey{'type'}
		#<xmlDimmer> <channel index="1" type="DIMMER" count="1">
		#<paramset type="MASTER"	
		
	
	}
	
	
	
	# -- close file
	
		
}




# some debug
foreach my $test (sort keys %cnlTypeA) { 
	print "$test : ";#   $rO{$test}  \n"; 
	print "cnl: $cnlTypeA{$test}{'cnl'}  lst: $cnlTypeA{$test}{'list'}   type: $cnlTypeA{$test}{'type'}  interface: $cnlTypeA{$test}{'interface'}  index: $cnlTypeA{$test}{'index'}  bit: $cnlTypeA{$test}{'bit'}  size: $cnlTypeA{$test}{'size'}  id: $cnlTypeA{$test}{'id'}  \n"
}


## ---------- print register.h ------------------------------------------------------------------------------
print "\n\n\n";
usrMods::printDefaltTable(\%cType);
print "\n\n";

print $cType{'battValue'};


#//- ----------------------------------------------------------------------------------------------------------------------
#//- channel slice address definition -------------------------------------------------------------------------------------
#const uint8_t cnlAddr[] PROGMEM = {
#	0x01,0x02,0x0a,0x0b,0x0c,
#	0x01,
#}; // 6 byte
#
#//- channel device list table --------------------------------------------------------------------------------------------
#EE::s_cnlTbl cnlTbl[] = {
#	// cnl, lst, sIdx, sLen, pAddr;
#	{0, 0, 0x00,  5, 0x000f},
#	{1, 4, 0x05,  1, 0x0014},   //  1 *  6 =   6 (0x0006)
#}; // 12 byte
#
#//- peer device list table -----------------------------------------------------------------------------------------------
#EE::s_peerTbl peerTbl[] = {
#	// cnl, pMax, pAddr;
#	{1, 6, 0x001a}              //  6 * 4 =  24 (0x18)
#}; // 4 byte
#
#//- handover to AskSin lib -----------------------------------------------------------------------------------------------
#EE::s_devDef devDef = {
#	1, 2, devIdnt, cnlAddr,
#}; // 6 byte
#
#//- module registrar -----------------------------------------------------------------------------------------------------
#RG::s_modTable modTbl[1];



#print "test\n";
