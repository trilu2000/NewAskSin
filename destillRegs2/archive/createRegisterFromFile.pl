use strict;
use warnings;
use diagnostics;
use XML::LibXML;

my $dir = 'devicetypes';																					# directory with the HM device files

#-- some functions -------------------------------------------------------------------------------------
# @array=&del_double(@array);
sub del_double {
	my %all=();
	@all{@_}=1;
	return (sort { $a cmp $b } keys %all);
}


#-------------------------------------------------------------------------------------------------------
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
my $devFW = 1;

foreach my $sections ($xc->findnodes('/device/supported_types/type')) {										# set pointer to type

	print $sections->getAttribute('name') ."\n";	
	
	foreach my $param ($sections->findnodes('./parameter')) {												# set pointer to parameter
		my $devIdx = $param->getAttribute('index');															# get the device index	
		
		if ($devIdx ==  '9.0') {																			# reflects the firmware
				
			my $condOP = $param->getAttribute('cond_op');
			if (( $condOP eq "GE" ) || ( $condOP eq "LE" ) || ( $condOP eq "EQ" )) {
				$devFW = eval $param->getAttribute('const_value');
				#print "$devFW\n";
			}
		}
	
		if ($devIdx == '10.0') {																			# only 10.0 is interesting
			$devCVal = eval($param->getAttribute('const_value'));
			#print "$devCVal\n";
		}
	
	}

	if (eval( $devCVal) == eval($dev_id) ) {																# check const value against device id
		last;
	}

}
#exit(0);

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
		$cnlTypeA{sprintf('00 %.2x %s', $phyLst, $idxIdt)} = {'idt' => $idxIdt, 'cnl' => 0, 'lst' => $phyLst, 'id' => $prmID, 'reg' => $idxVal, 'bgn' => $idxBgn, 'sze' => $idxSze };		
		
		#print "$idxIdt Idx: $idxVal \t B: $idxBgn \t S: $idxSze \t \n\n";
	
	}

	#-- needed for master id ---------------------------------------------------------------------------
	if (!$cnlTypeA{'00 00 0x0a.0'}) {
		$cnlTypeA{'00 00 0x0a.0'} = {'idt' => '0x0a.0', 'cnl' => 0, 'lst' => 0, 'id' => 'Master_ID_A', 'reg' => 0x0a, 'bgn' => 0, 'sze' => 8};
	}
	if (!$cnlTypeA{'00 00 0x0b.0'}) {
		$cnlTypeA{'00 00 0x0b.0'} = {'idt' => '0x0b.0', 'cnl' => 0, 'lst' => 0, 'id' => 'Master_ID_B', 'reg' => 0x0b, 'bgn' => 0, 'sze' => 8};
	}
	if (!$cnlTypeA{'00 00 0x0c.0'}) {
		$cnlTypeA{'00 00 0x0c.0'} = {'idt' => '0x0c.0', 'cnl' => 0, 'lst' => 0, 'id' => 'Master_ID_C', 'reg' => 0x0c, 'bgn' => 0, 'sze' => 8};
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
	my $cntFromSys = $sections->getAttribute('count_from_sysinfo');
	
	if ($cnlIdx == '0') {																					# channel 0 already done
		next;
	}
	#print "$cnlIdx $cnlCnt \n"; 

	if (( !$cnlCnt ) && ( !$cntFromSys )) {$cnlCnt = 1;}													# count from sys not given, therefore it is 1

	if ( $cntFromSys ) {
		#  <channel index="1" type="SWITCH" count_from_sysinfo="23.0:0.4">
		#       0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
		#<- 1A 03 84 00 01 02 05 00 00 00 15 00 6C 74 6C 75 31 30 30 31 32 33 35 10 41 01 00 (30493)

		print "Please enter the amount of channels with the same configuration as channel $cnlIdx\n";
		print "# Channel: ";

		$cnlCnt = 1;
		chomp ($cnlCnt = <STDIN>);
		
		print "\n";
	}
	

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
#foreach my $item (sort{$a <=> $b}(keys %cnlConf)) {
#	my $h = $cnlConf{$item};
#	print "$item,  $h->{'idx'}  $h->{'cnt'} $h->{'type'} \t $h->{'id'} \t $h->{'ref'}\n";
#}


#-- now getting the registers according the channel config ---------------------------------------------
foreach my $item (sort{$a <=> $b}(keys %cnlConf)) {
	my $h = $cnlConf{$item};
	#print "$item,   $h->{'cnl'}  $h->{'idx'}  $h->{'cnt'} $h->{'type'} \t $h->{'id'} \t $h->{'ref'}\n";
	
	foreach my $sections ($xc->findnodes('/device/paramset_defs/paramset')) {								# set pointer to paramset
		my $parsetId = $sections->getAttribute('id');														# get the id

		if ($parsetId ne $h->{'ref'} ) {																	# next paramset while id didn't fit
			next;
		}
		
		#-- now stepping through the parameter ---------------------------------------------------------
		foreach my $parameter ($sections->findnodes('./parameter')) {										# set pointer to parameter
			my $paraId = $parameter->getAttribute('id');													# get the id
			
			foreach my $physical ($parameter->findnodes('./physical')) {									# set pointer to physical
				my $phyLst = $physical->getAttribute('list');												# get the list
				my $phyIdx = $physical->getAttribute('index');												# get the index
				my $phySze = $physical->getAttribute('size');												# get the size

				if (!$phyIdx) {																				# if no index given, skip
					next;
				}

				#-- clean up variables and write in hash -----------------------------------------------
				#print "$phyLst $phyIdx $phySze  \n";
				
				my $idxBgn = 0;																				# default is bit 0 

				if (index($phyIdx, '.') != -1) {															# check for starting bit position
					$idxBgn = substr $phyIdx, -1;															# bit position is the last number
					$phyIdx = substr($phyIdx, 0, -2);														# shorten the index to get it converted to a dec number
				} 
				my $idxVal = eval $phyIdx;																	# convert from hex to dec if necessary
				
				#-- getting the size in bit ------------------------------------------------------------
				my ($hiValue, $loValue) = (0, 0);
				my $i = index($phySze, '.');
				
				if ($i == -1) {																				# no dot in size
					$hiValue = $phySze;
		
				} else {																					# we have to split up 
					$hiValue = substr($phySze, $i-1, 1);													# get the byte in front of the dot
					$loValue = substr($phySze, $i+1, 1);													# and after the dot
				}
				my $idxSze = ($hiValue*8) + $loValue;														# recalculate the amount of bit
				my $idxIdt = sprintf('0x%.2x.%s', $idxVal, $idxBgn);
		
				#-- put details in the hash for further use --------------------------------------------
				$cnlTypeA{sprintf('%.2x %.2x %s', $h->{'cnl'}, $phyLst, $idxIdt)} = {'idt' => $idxIdt, 'cnl' => $h->{'cnl'}, 'lst' => $phyLst, 'id' => $paraId, 'reg' => $idxVal, 'bgn' => $idxBgn, 'sze' => $idxSze };		
		
				#print "$idxIdt Idx: $idxVal \t B: $idxBgn \t S: $idxSze \t \n\n";
				
				
			}
		
		}
	
	}
	
}

#-- some debug -----------------------------------------------------------------------------------------
#foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
#	my $h = $cnlTypeA{$item};
#	print "$item,  $h->{'reg'} \n";
#}
#-------------------------------------------------------------------------------------------------------



#-------------------------------------------------------------------------------------------------------
#-- creation of channel table --------------------------------------------------------------------------
my $tempPhyAddr = 15;																						# starts at 15 while in front of the magic number, the serial and the HMID
my %cnlAddr = ();																							# holds the slice string
my %cnlTbl = ();																							# the channel table
my %peerTbl = ();																							# the peer table

my @tempSlcArray = ();


#stepping through the table
foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
	my $h = $cnlTypeA{$item};

	my $cnlTblID = sprintf('%.2x %.2x', $h->{'cnl'}, $h->{'lst'} );											# generating id for the hash

	#check if cnl/lst combination still exist, if not add it and collect the different regs in an array
	push ( @{ $cnlAddr{$cnlTblID} }, sprintf('0x%.2x,', $h->{'reg'}) );										# writing all regs in the respective array	

	#prepare the channel table
	$cnlTbl{ $cnlTblID } = {'cnl' => $h->{'cnl'}, 'lst' => $h->{'lst'}, 'sIdx' => 0, 'sLen' => 0, 'pAddr' => 0, 'peer' => 0  };

}

#-- cleanout doubles in channel table arrays and further prepare of channel table
foreach my $item ( sort { $a cmp $b } keys %cnlAddr) {														# stepping through the hash of arrays
	my $h = $cnlAddr{$item};																				# setting pointer
	@{ $h } = del_double(@{ $h });																			# deleting doubles
	
	$cnlTbl{$item}{'sLen'} = scalar @{ $h };																# calculating and storing slice len

	# shorten the slice table while slices are doubled
	my $t = 1+index( "@tempSlcArray", "@{$h}" );															# find the index in the slice string
	if ($t > 0) {$t -= 1; $t /= 6;}																			# divide it by 6 because of length of one element
	#print "$t, \n";

	if ($t > 0) {																							# while already in the index
		$cnlTbl{$item}{'sIdx'} = $t;																		# remember the index
		delete $cnlAddr{$item};																				# item not needed any more
				
	} else {																								# not in the index
		$cnlTbl{$item}{'sIdx'} = scalar @tempSlcArray;														# remember the index
		push (@tempSlcArray, @{ $h });																		# add current state to slice array but only if cnl/lst slice was not found
	}
	
	# getting the amount of peers
	if (($cnlTbl{$item}{'lst'} == 3) || ($cnlTbl{$item}{'lst'} == 4)) {
		print "Please enter the amount of peers you wish for channel: $cnlTbl{$item}{'cnl'}, list: $cnlTbl{$item}{'lst'}\n";
		print "# Peers: ";

		my $peers = 6;
		chomp ($peers = <STDIN>);
		$cnlTbl{$item}{'peer'} = $peers;
		
		print "\n";
	} 

	# calculating the addresses in channel table
	$cnlTbl{$item}{'pAddr'} = $tempPhyAddr;																	# adding the new start address to the channel table item

	if ( ($cnlTbl{$item}{'peer'}) > 0) {																	# calculation is different when peer is 0
		$tempPhyAddr += ($cnlTbl{$item}{'sLen'} * $cnlTbl{$item}{'peer'});									# length of string multiplied by amount of peers
		#print ">0  $tempPhyAddr \n";

		# creating peer table, because only valid for list 3 or 4
		$peerTbl{$item} = { 'cnl' => $cnlTbl{$item}{'cnl'}, 'peer' => $cnlTbl{$item}{'peer'}, 'pAddr' => 0 };
		
	} else {
		$tempPhyAddr += $cnlTbl{$item}{'sLen'};																# adding only the length
		#print "=0  $tempPhyAddr \n";
		
	}
}


#-- calculating peer addresses in eeprom ---------------------------------------------------------------
foreach my $item ( sort { $a cmp $b } keys %peerTbl) {
	my $h = $peerTbl{$item};

	$h->{'pAddr'} = $tempPhyAddr;
	$tempPhyAddr += ($h->{'peer'} * 4);																		# every peer needs 4 bytes

}


#-------------------------------------------------------------------------------------------------------
#-- some debug -----------------------------------------------------------------------------------------
#foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
#	my $h = $cnlTypeA{$item};
#	print "$item,  r: $h->{'reg'} \t b: $h->{'bgn'}  \t s: $h->{'sze'} \n";
#}
#print "\n\n";
#
#foreach my $item ( sort { $a cmp $b } keys %cnlAddr) {
#	my $h = $cnlAddr{$item};
#	print "$item, @{$h}   \n";
#}
#print "\n\n";
#
#print          "cnl, lst, sIdx, sLen, pAddr  \n";
#foreach my $item ( sort { $a cmp $b } keys %cnlTbl) {
#	my $h = $cnlTbl{$item};
#	print sprintf( "{%.2d, %.2d, 0x%.2x, %.2d,  0x%.4x},", $h->{'cnl'}, $h->{'lst'}, $h->{'sIdx'}, $h->{'sLen'}, $h->{'pAddr'} ) ."\n";
#}
#print "\n\n";
#
#print          "cnl, pMax, pAddr;  \n";
#foreach my $item ( sort { $a cmp $b } keys %peerTbl) {
#	my $h = $peerTbl{$item};
#	print sprintf( "{%.2d, %.2d, 0x%.4x},", $h->{'cnl'}, $h->{'peer'}, $h->{'pAddr'} ) ."\n";
#}
#print "\n\n";
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#-- starting the print out -----------------------------------------------------------------------------
print "\n\n";
print "##- ----------------------------------------------------------------------------------------------------------------------\n";
print "##- generated by createRegisterFromFile.pl\n";
print "##- ID: " .sprintf("0x%.4x", $devCVal) .", File: $rf_file\n";
print "##- ----------------------------------------------------------------------------------------------------------------------\n\n";

print "##- ----------------------------------------------------------------------------------------------------------------------\n";
print "##- ----------------------------------------------------------------------------------------------------------------------\n";
print "##                                   FW  moID   serial                         ST  devInfo\n";
print "## <- 1A 01 A4 00 01 02 05 63 19 63  15  00 6C  74 6C 75 31 30 30 31 32 33 35  10  11 01 00 \n";
print "## FW   -> Firmware, sometimes given in xml files of hm config software\n";
print "## moID -> Model ID, important for identification in hm config software\n";
print "## ST   -> Subtype, identifier if device is a switch or a dimmer or a remote\n";
print "## devInfo -> Device Info -> sometimes hm config files are refering on byte 23 for the amount of channels, other bytes not known\n";
print "##                           23:0 0.4, means first four bit of byte 23 reflecting the amount of channels\n";
print "##\n";

print "##- settings of HM device for AS class -----------------------------------------------------------------------------------\n";
print "const uint8_t devIdnt[] PROGMEM = {\n";
print "    /* Firmware version 1 byte */  ".sprintf("0x%.2x,", $devFW) ."\n";
print "    /* Model ID         2 byte */  ".sprintf("0x%.2x, 0x%.2x,", ($devCVal & 0xff00) / 256, $devCVal & 0xff) ."\n";
print "    /* Sub Type ID      1 byte */  0x00,\n";
print "    /* Device Info      3 byte */  0x41, 0x01, 0x00, \n";
print "};\n\n";


print "##- ----------------------------------------------------------------------------------------------------------------------\n";
print "##- channel slice address definition -------------------------------------------------------------------------------------\n";
print "const uint8_t cnlAddr[] PROGMEM = {\n";
my $xSize = 0;
foreach my $item ( sort { $a cmp $b } keys %cnlAddr) {
	print "    @{$cnlAddr{$item}}\n";
	$xSize += scalar @{$cnlAddr{$item}};
}
print "};  ## $xSize byte\n\n";


print "##- channel device list table --------------------------------------------------------------------------------------------\n";
print "EE::s_cnlTbl cnlTbl[] = {\n";
print "    ## cnl, lst, sIdx, sLen, pAddr;\n";
$xSize = 0;
foreach my $item ( sort { $a cmp $b } keys %cnlTbl) {
	my $h = $cnlTbl{$item};
	print sprintf( "    { %2s, %2s, 0x%.2x, %2s,  0x%.4x },", $h->{'cnl'}, $h->{'lst'}, $h->{'sIdx'}, $h->{'sLen'}, $h->{'pAddr'} ) ."\n";
	$xSize += 6;
}
print "};  ## $xSize byte\n\n";


print "##- peer device list table -----------------------------------------------------------------------------------------------\n";
print "EE::s_peerTbl peerTbl[] = {\n";
print "    ## cnl, pMax, pAddr;\n";
$xSize = 0;
foreach my $item ( sort { $a cmp $b } keys %peerTbl) {
	my $h = $peerTbl{$item};
	print sprintf( "    { %2s, %2s, 0x%.4x },", $h->{'cnl'}, $h->{'peer'}, $h->{'pAddr'} ) ."\n";
	$xSize += 4;
}
print "};  ## $xSize byte\n\n";


print "##- handover to AskSin lib -----------------------------------------------------------------------------------------------\n";
print "EE::s_devDef devDef = {\n";
print sprintf("    %d, %d, devIdnt, cnlAddr,", scalar(keys %peerTbl), scalar(keys %cnlTbl)) ."\n";
print "};  ## 10 byte\n\n";

print "##- module registrar -----------------------------------------------------------------------------------------------------\n";
print "RG::s_modTable modTbl[" .scalar(keys %peerTbl) ."];\n\n";


print "\n\n##- ----------------------------------------------------------------------------------------------------------------------\n";
print "##- only if needed -------------------------------------------------------------------------------------------------------\n";
print "##- ----------------------------------------------------------------------------------------------------------------------\n";

my $lastCnl = 255; my $lastLst = 255;
my $lastReg = 0;   my $lastBte = 0;
my $cnt = 0; my $end = scalar(keys %cnlTypeA);

my $cnlLstCnt;

foreach my $item ( sort { $a cmp $b } keys %cnlTypeA) {
	my $h = $cnlTypeA{$item};

	if (( $lastReg != $h->{'reg'} ) && ( $lastBte )) {														# reg change but former reg was not filled to 8
		print sprintf("    uint8_t %-25s :%s;       ## 0x%.2x, s:%s, l:%s ", '', 8-$lastBte, $lastReg, $lastBte, 8,  ) ."\n";
		$lastBte = 0;																						# new start, therefore last byte has to be full

	}

	if ( ($lastCnl != $h->{'cnl'}) || ($lastLst != $h->{'lst'}) ) {											# check if we are in a new struct set
		if ($lastCnl != 255) {																				# close the former structure, will not work for the very last struct
			print "};  ## $cnlLstCnt byte\n";	
		}
		
		print "\nstruct s_lst$h->{'lst'}Cnl$h->{'cnl'} {\n";												# print the struct header
		#print "## @{$cnlAddr{ sprintf('%.2x %.2x', $h->{'cnl'}, $h->{'lst'} ) }}\n";
		$lastReg = 0;																						# no last reg available yet
		$lastBte = 0;																						# no former byte to fill
		$cnlLstCnt = 0;

		my $x = $cnlTbl{ sprintf('%.2x %.2x', $h->{'cnl'}, $h->{'lst'} ) };									# generating id for the hash
		my $sStr = substr("@tempSlcArray", $x->{'sIdx'} * 6, $x->{'sLen'} * 6 );
		print "## $sStr\n";

	}
	$lastCnl = $h->{'cnl'};																					# remember the current channel and list
	$lastLst = $h->{'lst'};

	
	# add one empty line with the missing bits
	if ( $h->{'bgn'} > $lastBte ) {																			# identify a gap
		print sprintf("    uint8_t %-25s :%s;       ## 0x%.2x, s:%s, e:%s ", '', $h->{'bgn'} - $lastBte, $h->{'reg'}, $lastBte, $h->{'bgn'},  ) ."\n";
		
	}

	# add the real line
	print sprintf("    uint8_t %-25s :%s;       ## 0x%.2x, s:%s, e:%s ", $h->{'id'}, $h->{'sze'}, $h->{'reg'}, $h->{'bgn'}, $h->{'bgn'} + $h->{'sze'},  ) ."\n";

	if ( $lastReg != $h->{'reg'} ) {$cnlLstCnt++;}															# increase struct byte counter

	$lastReg = $h->{'reg'};																					# remember the last reg value
	$lastBte = $h->{'bgn'} + $h->{'sze'};																	# remember last bit position
	if ( $lastBte >= 8 ) {$lastBte = 0;}																	# if last bit gets above 8, then reset


	# check if we are on the end of the hash table
	$cnt++;
	if ($cnt == $end) {																						# definately the last key in table
		if ( $lastBte ) {																					# former reg was not filled to 8
			print sprintf("v   uint8_t %-25s :%s;       ## 0x%.2x, s:%s, l:%s ", '', 8-$lastBte, $lastReg, $lastBte, 8-$lastBte,  ) ."\n";
		}

		print "};  ## $cnlLstCnt byte\n";	
		
	}
}

