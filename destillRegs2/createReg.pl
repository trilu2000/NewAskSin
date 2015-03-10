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
# check content, only A-F, 0-9 allowed 
$ret = usrMods::checkString($cType{'hmID'}, 'H', 6);
if ($ret != 0) {
	print "generating new hm ID...\n";
	$cType{'hmID'} = usrMods::randString('H',6);
}

## ---------- model id check ---------------------
# check content, only A-F, 0-9 allowed 
$ret = usrMods::checkString($cType{'modelID'}, 'H', 4);
if ($ret != 0) {
	print "model ID in wrong format, exit...\n";
	exit;
}

# check if we will find a version in the hm config directory
my @fileList          =usrMods::searchXMLFiles($cType{'modelID'});
for my $href ( @fileList ) {
    print sprintf("modelID: %.4x, FW: %.2x, File: %-25s\n", $href->{'modelID'}, $href->{'firmwareVer'},$href->{'file'});
}


## ---------- firmware version check -------------
# found more than one file, lets choose
# if we found one file take over the firmware
# firmware is zero, write something like 10
# firmware is > 0, take it
my $numArr= scalar @fileList;
if      ($numArr > 1) {
	# lets choose one line

} elsif ($numArr == 1) {
	$cType{'firmwareVer'} = $fileList[0]{'firmwareVer'};

} 


print "fw: $cType{'firmwareVer'}\n";









	
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
