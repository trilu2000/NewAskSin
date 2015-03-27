use strict;
use warnings;
use XML::LibXML;



## --------------import constants----------------------------------------------
use devDefinition;
use destillRegsModules;

my %cType             =usrRegs::usr_getHash("configTypes");




## ---------- checking basic informations -------------------------------------

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
#for my $href ( @fileList ) {
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

## ---------- subtype id check ------------------------
if ($numArr > 0) {
# get subtypeID from original document
#	$cType{'subtypeID'} = int(rand(0xFFFFFF));
}

## ---------- deviceInfo check ------------------------
if ($numArr > 0) {
# get deviceInfo from original document
#	$cType{'deviceInfo'} = int(rand(0xFFFFFF));
}

## ----------------------------------------------------------------------------




print "\n\n\n";
usrMods::printDefaltTable(\%cType);
print "\n\n";

print $cType{'battery'};


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
