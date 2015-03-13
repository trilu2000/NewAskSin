use strict;
use warnings;
use XML::LibXML;



## --------------import constants---------------------
use devDefinition;
use destillRegsModules;

my %cType             =usrRegs::usr_getHash("configTypes");


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


#print "fw: $cType{'firmwareVer'}\n";

print "\n\n\n";
usrMods::printDefaltTable(\%cType);

#print "hmID dec: $cType{'hmID'}\n";
#print "hmID hex: " .sprintf("0x%.2x", $cType{'hmID'}) ."\n";


#print "x: " .usrMods::frmHexStr($cType{'hmID'}, 6) ." \n";









	
#	print "$type, $listTypes{$type}\n";

	#foreach my $reg(sort(keys %{$listTypes{$type}} )) {
		


	#	if ($culHmRegDefShLg{$reg}){
	#		delete $listTypes{$type}{$reg};
	#		$listTypes{$type}{"sh".$reg} = 1;
	#		$listTypes{$type}{"lg".$reg} = 1;
	#	}
	#}
#}

#foreach my $type(sort(keys %listTypes)) { 
#	my $value = $listTypes{$type};


#print "test\n";
