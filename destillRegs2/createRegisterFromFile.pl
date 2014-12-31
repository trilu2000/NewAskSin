use strict;
use warnings;
use diagnostics;
use XML::LibXML;

my $dir = 'devicetypes';																					# directory with the HM device files

#-- quit unless we have the correct number of command-line args ----------------------------------------
my $num_args = $#ARGV + 1;
if ($num_args != 2) {
    print "\nUsage: createRegisterFromFile.pl rf_file dev_id\n";
    exit;
}
 
#-- we got two command line args, so assume they are the rf_file and dev_id ----------------------------
my $rf_file=$ARGV[0];
my $dev_id=$ARGV[1];


#-- create parser object -------------------------------------------------------------------------------
my $parser = XML::LibXML->new();																			# create the xml object
my $doc    = $parser->parse_file("$dir/$rf_file");															# open the file
my $xc = XML::LibXML::XPathContext->new( $doc->documentElement()  );										# create parser object


#-- open the file and check if dev_id fits -------------------------------------------------------------
my $devCVal = 0;

foreach my $sections ($xc->findnodes('/device/supported_types/type')) {										# set pointer to type

	foreach my $param ($sections->findnodes('./parameter')) {												# set pointer to parameter
		my $devIdx = $param->getAttribute('index');															# get the device index	
		
		if ($devIdx != '10.0') {																			# only 10.0 is interesting
			next;
		}

		if (eval($param->getAttribute('const_value')) == eval($dev_id)) {									# check const value against device id
			$devCVal = eval($dev_id);																		# copy into the variable
		}
	}
}

if ($devCVal != eval($dev_id)) {																			# check the variable
	print "\nError: Device ID not found in file $rf_file\n";												# print error message while not fitting
	exit 0;																									# and exit
}


#-- get address definition from given file -------------------------------------------------------------
# step through the file and collect information and store in a hash
my %cnlTypeA = ();

foreach my $sections ($xc->findnodes('/device/paramset')) {													# set pointer to type
	my $prmType = $sections->getAttribute('type');															# get the type	
	
	if ($prmType ne 'MASTER') {																				# only MASTER is interesting
		next;
	}

	#-- now step through the single nodes of parameter
	my $phyLst;
	foreach my $parameter ($sections->findnodes('./parameter')) {											# set pointer to parameter
		my $prmID =  $parameter->getAttribute('id');														# get the parameter id	

		my($physical) = $parameter->findnodes('./physical');												# set pointer
		my $phyLst = $physical->getAttribute('list');														# get the list
		my $phyIdx = $physical->getAttribute('index');														# the index
		my $phySze = $physical->getAttribute('size');														# and size
		
		#print "$prmID \t l:$phyLst \t i:$phyIdx  \t s:$phySze\n";											# some debug

		#-- get the starting bit position if exits, otherwise assume it is 0 ---------------------------
		my $idxBgn = 0;																						# default is bit 0 

		if (index($phyIdx, '.') != -1) {																	# check for starting bit position
			$idxBgn = substr $phyIdx, -1;																	# bit position is the last number
			$phyIdx = substr($phyIdx, 0, -2);																# shorten the index to get it converted to a dec number
		} 
		my $idxVal = eval $phyIdx;																			# convert from hex to dec if necessary

		#-- getting the size in bit --------------------------------------------------------------------
		my ($hiValue, $loValue) = (0, 0);
		my $i = index($phySze, '.');
		
		if ($i == -1) {																						# no dot in size
			$hiValue = $phySze;

		} else {																							# we have to split up 
			$hiValue = substr($phySze, 0, $i);																# get the bytes in front of the dot
			$loValue = substr($phySze, $i+1, 1);															# and after the dot
		}
		my $idxSze = ($hiValue*8) + $loValue;																# recalculate the amount of bit
		my $idxIdt = sprintf('0x%.2x.%s', $idxVal, $idxBgn);

		#-- put details in the hash for further use ----------------------------------------------------
		$cnlTypeA{sprintf('00 %.2x %s', $phyLst, $idxIdt)} = {'idt' => $idxIdt, 'cnl' => 0, 'lst' => $phyLst, 'reg' => $idxVal, 'bgn' => $idxBgn, 'sze' => $idxSze };		
		
		#print "$idxIdt Idx: $idxVal \t B: $idxBgn \t S: $idxSze \t \n\n";
	
	}

	#-- needed for master id ---------------------------------------------------------------------------
	if (!$cnlTypeA{'00 00 0x0a.0'}) {
		$cnlTypeA{'00 00 0x0a.0'} = {'idt' => '0x0a.0', 'cnl' => 0, 'lst' => 0, 'reg' => 0x0a, 'bgn' => 0, 'sze' => 0};
	}
	if (!$cnlTypeA{'00 00 0x0b.0'}) {
		$cnlTypeA{'00 00 0x0b.0'} = {'idt' => '0x0b.0', 'cnl' => 0, 'lst' => 0, 'reg' => 0x0b, 'bgn' => 0, 'sze' => 0};
	}
	if (!$cnlTypeA{'00 00 0x0c.0'}) {
		$cnlTypeA{'00 00 0x0c.0'} = {'idt' => '0x0c.0', 'cnl' => 0, 'lst' => 0, 'reg' => 0x0c, 'bgn' => 0, 'sze' => 0};
	}
}	

#-- some debug -----------------------------------------------------------------------------------------
#foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
#	my $h = $cnlTypeA{$item};
#    print "$item,  $h->{'reg'} \n";
#}

#-- now getting the channel config ---------------------------------------------------------------------
my %cnlConf = ();
my $xCnt = 0;

foreach my $sections ($xc->findnodes('/device/channels/channel')) {											# set pointer to channel
	my $cnlIdx = $sections->getAttribute('index');															# get the channel index	
	my $cnlCnt = $sections->getAttribute('count');															# get the channel count	

	if ($cnlIdx == '0') {																					# channel 0 already done
		next;
	}
	#print "$cnlIdx $cnlCnt \n"; 

	#-- stepping through the paramsets per channel -----------------------------------------------------
	my $cnt = $cnlCnt;
	
	while ($cnt) {																							# if we have more then one channel in the paramset
		my $cnl = $cnlIdx + $cnlCnt - $cnt;
		
		
		foreach my $param ($sections->findnodes('./paramset')) {											# set pointer to paramsets
			my $paraType = $param->getAttribute('type');													# get the type
			my $paraID = $param->getAttribute('id');														# get the id	
	
			#-- stepping through the subsets ---------------------------------------------------------------
			
			foreach my $subset ($param->findnodes('./subset')) {											# set pointer to paramsets
				my $paraRef = $subset->getAttribute('ref');
		
				$cnlConf{$xCnt++} = {'cnl' => $cnl, 'idx' => $cnlIdx, 'cnt' => $cnlCnt, 'type' => $paraType, 'id' => $paraID, 'ref' => $paraRef };
				#print "$cnl   $cnlIdx $cnlCnt $paraType $paraID \t $paraRef\n"; 

			}
		}
		$cnt--;	

	}
}

#-- some debug -----------------------------------------------------------------------------------------
foreach my $item (sort{$a <=> $b}(keys %cnlConf)) {
	my $h = $cnlConf{$item};
    print "$item,  $h->{'idx'}  $h->{'cnt'} $h->{'type'} \t $h->{'id'} \t $h->{'ref'}\n";
}


#1 1 MASTER dimmer_ch_master 	 physical_parameters
#1 1 MASTER dimmer_ch_master 	 general_parameters
#1 1 VALUES dimmer_ch_values 	 dimmer_valueset
#1 1 LINK dimmer_ch_link 	 dimmer_linkset
#2 2 MASTER dimmer_virt_ch_master 	 general_parameters
#2 2 VALUES dimmer_ch_values 	 dimmer_valueset
#2 2 LINK dimmer_ch_link 	 dimmer_linkset

#-- some debug -----------------------------------------------------------------------------------------
#foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
#	my $h = $cnlTypeA{$item};
#    print "$item,  $h->{'reg'} \n";
#}


#-- starting the print out -----------------------------------------------------------------------------
print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- generated by createRegisterFromFile.pl\n";
print "//- ID: " .sprintf("0x%.4x", $devCVal) .", File: $rf_file\n";
print "//- ----------------------------------------------------------------------------------------------------------------------\n\n";

print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- settings of HM device for AS class -----------------------------------------------------------------------------------\n";
print "const uint8_t devIdnt[] PROGMEM = {\n";
print "    /* Firmware version 1 byte */  0x01,                                     // don't know for what it is good for\n";
print "    /* Model ID         2 byte */  ".sprintf("0x%.2x, 0x%.2x,", ($devCVal & 0xff00) / 256, $devCVal & 0xff);
print                                               "                               // model ID, describes HM hardware. Own devices should use high values due to HM starts from 0\n";
print "    /* Sub Type ID      1 byte */  0x00,                                     // not needed for FHEM, it's something like a group ID\n";
print "    /* Device Info      3 byte */  0x41, 0x01, 0x00                          // describes device, not completely clear yet. includes amount of channels\n";
print "};\n\n\n";


#//- ----------------------------------------------------------------------------------------------------------------------
#//- channel slice address definition -------------------------------------------------------------------------------------
#const uint8_t cnlAddr[] PROGMEM = {
#    0x02,0x0a,0x0b,0x0c,0x15,0x18,
#    0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
#    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
#};  // 72 byte





