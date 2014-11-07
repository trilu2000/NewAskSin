use strict;

use RegConfig;
# ========================import constants=====================================
my %culHmRegDefShLg       =HMConfig::HMConfig_getHash("culHmRegDefShLg");
my %culHmRegDefine        =HMConfig::HMConfig_getHash("culHmRegDefine");

use devDefinition;
my %regList               =usrRegs::usr_getHash("regList");
my %listTypes             =usrRegs::usr_getHash("listTypes");

# ========================procedural range=====================================

sub CUL_HM_initRegHash() { #duplicate short and long press register 
	foreach my $reg (keys %culHmRegDefShLg){ #update register list
		%{$culHmRegDefine{"sh".$reg}} = %{$culHmRegDefShLg{$reg}};
		%{$culHmRegDefine{"lg".$reg}} = %{$culHmRegDefShLg{$reg}};
		$culHmRegDefine{"lg".$reg}{a} +=0x80;
	}

	foreach my $type(sort(keys %listTypes)){ 
		foreach my $reg (sort(keys %{$listTypes{$type}} )){
			if ($culHmRegDefShLg{$reg}){
				delete $listTypes{$type}{$reg};
				$listTypes{$type}{"sh".$reg} = 1;
				$listTypes{$type}{"lg".$reg} = 1;
			}
		}
	}

}

################### definitions  -  - user range ###################


################### definitions - system range #####################
CUL_HM_initRegHash();
#++++++++++++++ compilation++++++++++++++++++++

my %regLfull;

foreach my $chn (sort(keys %regList)){
	my $maxPeers = $regList{$chn}{peers};
	foreach my $reg(sort(keys %{$listTypes{$regList{$chn}{type}}} )){   
		my $lst = $culHmRegDefine{$reg}{l} if ($culHmRegDefine{$reg});	
		if (!defined$culHmRegDefine{$reg}){
			print " undefined register $reg, please correct\n";
			exit 0;
		}
	
		my $cntEnd = ($lst == 3 || $lst == 4)?$maxPeers:1; # peers only for list 3 and list 4
		for (my $cnt = 1;$cnt<=$cntEnd;$cnt++){
			#print "x: $culHmRegDefine{$reg}{a} y:$reg\n";
			$regLfull{$chn}{$lst}{"empty$cnt"}{$culHmRegDefine{$reg}{a}} = $reg;
		}
	
	}
}


# == provide an array with channel device config =======================================================
my %cnlLstType;																							# some variables
my @cnlTypeA = ();

foreach my $cnt (sort{$a <=> $b}(keys %regList)) {														# get a list of all cnlTypes
	push(@cnlTypeA, $regList{$cnt}{type});
}

my %seen;																								# remove doubles
my @unique = grep { ! $seen{$_}++ } @cnlTypeA;
@cnlTypeA = @unique;
#print "cnlTypeA order: @unique\n";

# -- create the array ----------------------------------------------------------------------------------
my $cCnt = 0;
foreach my $i (keys @cnlTypeA) {																		# create the cnlType object
	
	my ($idxCnl) = grep { $regList{$_}{type} eq $cnlTypeA[$i] } (sort{$a <=> $b}(keys %regList));		# get the idx in $regList																						# get the first channel which fits to our cnlType

	foreach my $lst (sort{$a <=> $b}(keys %{$regLfull{$idxCnl}})) {										# go through the registry and search for a fitting channel	
		$cnlLstType{$cCnt}{'cnl'} = $idxCnl;
		$cnlLstType{$cCnt}{'lst'} = $lst;
		$cnlLstType{$cCnt}{'cnlType'} = "$cnlTypeA[$i]";
		$cnlLstType{$cCnt}{'cnlLstType'} = "$cnlTypeA[$i]L$lst";
		
		$cCnt++;																						# increase the index counter
	}
}
# -- some debug ----------------------------------------------------------------------------------------
#foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
#	my $h = $cnlLstType{$cnt};
#	print  "$cnt, cnl:$h->{'cnl'}, lst:$h->{'lst'}, cnlType:$h->{'cnlType'}, cnlLstType:$h->{'cnlLstType'} " ."\n";
#}
#print "\n";

# -- create the slcStr in the array --------------------------------------------------------------------
my ($slcCnt, $slcIdx) = (0, 0);																			# declaration of some counters

foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {													# step through the lines
	my $h = $cnlLstType{$cnt};																			# some shorthand
	
	$cnlLstType{$cnt}{'slcIdx'} = $slcIdx;																# remember starting position in slice stream

	my $regL = 0;
	$slcCnt = 0;	
	foreach my $reg (sort{$a <=> $b}(keys %{$regLfull{$h->{'cnl'}}{$h->{'lst'}}{"empty1"}})) {			# step through the regs

		if (int($reg) == int($regL)) {next;};															# same as before

		my $regPrt = $culHmRegDefine{$regLfull{$h->{'cnl'}}{$h->{'lst'}}{"empty1"}{$reg}};				# pointer for short hand
		my $regSize = int($regPrt->{s}+.99);															# get the size of the reg variable
		
		for (my $i = 0 ; $i < $regSize ; $i++) {														# expand if more then one byte
			$cnlLstType{$cnt}{'slcStrItem'} .= sprintf("0x%.2x,", int($reg+$i));						# add the reg to the string
			$slcCnt++;																					# increase byte counter
			$slcIdx++;
		}  
	$regL = $reg;																						# remember reg for next time check
	$cnlLstType{$cnt}{'slcCnt'} = $slcCnt;																# remember starting position in slice stream
	}
}
# -- some debug ----------------------------------------------------------------------------------------
#foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
#	my $h = $cnlLstType{$cnt};
#	print  "$cnt, cnl:$h->{'cnl'}, lst:$h->{'lst'}, slcIdx:$h->{'slcIdx'}, slcCnt:$h->{'slcCnt'}, slcStr:$h->{'slcStrItem'} " ."\n";
#}
#print "\n";

# -- create the regdevs in the array -------------------------------------------------------------------
foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {													# step through the lines
	my $h = $cnlLstType{$cnt};																			# some shorthand

	my ($frntBits, $regL) = (0, 0);																		# size some variables for bit field calculation
	foreach my $reg (sort{$a <=> $b}(keys %{$regLfull{$h->{'cnl'}}{$h->{'lst'}}{"empty1"}})) {			# step through the regs
		my $regPrt = $culHmRegDefine{$regLfull{$h->{'cnl'}}{$h->{'lst'}}{"empty1"}{$reg}};				# pointer for short hand
		
		my $regByte = int($regPrt->{s}+.99);															# get the size of the reg variable
		my $regText = $regLfull{$h->{'cnl'}}{$h->{'lst'}}{"empty1"}{$reg};								# get the text of the reg variable

		my $regBits = ($regPrt->{s} - int($regPrt->{s}))*10;											# calculate the reg bits
		my $regSBit = ($reg*10) - (int($reg)*10);														# calculate the start bit
		my $regEBit = $regSBit + $regBits;																# calculate the end bit

		my $lastBits = 0;
		if ($regL) {																					# check if we have some bits from last try
			$lastBits = 8 - (($regL * 10) - (int($regL)*10));											# calculate the missing bits up to 8
		}
		
		my $fillText = "";
		if (($lastBits) && (int($regL) == int($reg))) {													# check if we are in the same byte
			$fillText = "    uint8_t" ." "x22 .":" .int(($reg - $regL)*10) .";";							# add the missing bits as a line 
			$fillText .= " "x5 ."//"; #." "x7 ."l:$regL, s:$reg";										# some debug
			$fillText .= "\n";
      if  ((($reg - $regL)*10) == 0) {$fillText = "";}

		} elsif (($lastBits) || ($regSBit)) {															# not in the same byte
			$fillText = "    uint8_t" ." "x22 .":" .($lastBits + $regSBit) .";";						# add the missing bits as a line 
			$fillText .= " "x5 ."//" ." "x7 ."l:$lastBits, s:$regSBit";									# some debug
			$fillText .= "\n";
		} 

		my $regVar = "    ";																			# fill the space in front of the reg variable
		if ($regByte == 1) {$regVar .= "uint8_t  ";}; 
		if ($regByte == 2) {$regVar .= "uint16_t ";};
		if ($regByte > 2)  {
			$regVar  .= "uint8_t  ";
			$regText .= "[$regByte]";
		}

		$regText .= ($regBits)?"":";"; 																	# if no bits to add then the semicolon could be added 
		$regText .= " "x(20 - length($regText)); 														# fill with blanks to have the same length
		$regText .= ($regBits)?":".$regBits.";":"   ";													# add the bit field 
		$regText .= " "x5 ."// " .sprintf("0x%.2x", int($reg)) .", s:$regSBit" .", e:$regEBit";			# some debug
		$regText .= "\n";

		if (($regEBit > 0) && ($regEBit < 8)) {															# remember missing bits for next run
			$regL = int($reg) .".$regEBit";
		} else {
			$regL = 0;
		}

		$cnlLstType{$cnt}{'devStrItem'} .= $fillText .$regVar .$regText;
	}
	
	if ($regL) {																						# add the missing bytes on the end
		my $lmissBits = 8 - int( ($regL - int($regL)) * 11);
		my $lmissText = "    uint8_t" ." "x22 .":" .$lmissBits .";";									# add the missing bits as a line 
		$lmissText .= " "x5 ."//";																		# some debug
		$lmissText .= "\n";

		$cnlLstType{$cnt}{'devStrItem'} .= $lmissText;
	}
}
# -- some debug ----------------------------------------------------------------------------------------
#foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
#	my $h = $cnlLstType{$cnt};
#	print  "$cnt, cnl:$h->{'cnl'}, lst:$h->{'lst'}, devStrItem: \n$h->{'devStrItem'} " ."\n";
#}
#print "\n";


# == providing an array with channel/list items and the respective channel device ======================
my %cnlDefIndex;
my ($cnt, $pAddr, $pPeer) = (0, 0, 0);

foreach my $chn (sort{$a <=> $b}(keys %regLfull)) {														# per channel
	foreach my $lst (sort{$a <=> $b}(keys %{$regLfull{$chn}})) {										# per list

		$cnlDefIndex{$cnt}{'cnl'} = $chn;																# get the channel
		$cnlDefIndex{$cnt}{'lst'} = $lst;																# get the list
		$cnlDefIndex{$cnt}{'cnlType'} = "$regList{$chn}{type}";											# get the name for the base struct
		$cnlDefIndex{$cnt}{'cnlLstType'} = "$regList{$chn}{type}L$lst";									# get the name for the sub struct
		
		my $maxPeer = scalar( keys %{$regLfull{$chn}{$lst}});											# get the number of peers
		if (($lst == 3) || ($lst == 4)) {																# we need the real number only for list3 and list4
			$cnlDefIndex{$cnt}{'pPeer'} = $pPeer;														# add the eeprom peer db address
			$cnlDefIndex{$cnt}{'pMax'} = $maxPeer;
			$pPeer += $maxPeer * 4;																		# calculate the eeprom peer db address
		} else {
			$cnlDefIndex{$cnt}{'pPeer'} = 0;															# add the eeprom peer db address
			$cnlDefIndex{$cnt}{'pMax'} = 0;
		}
		
		my ($x) = grep { $cnlLstType{$_}{cnlLstType} eq "$regList{$chn}{type}L$lst" } sort(keys %cnlLstType);	# get the idx in $regList																						# get the first channel which fits to our cnlType
		my $h = $cnlLstType{$x};																		# some shorthand
		#print "cnl:$chn, lst:$lst, x:$x, $h->{slcIdx} \n";
		
		$cnlDefIndex{$cnt}{'slcIdx'} = $h->{'slcIdx'}; 													# add the position in the slice string
		$cnlDefIndex{$cnt}{'slcLen'} = $h->{'slcCnt'};													# add the length of the respective slice string

		$cnlDefIndex{$cnt}{'pAddr'} = $pAddr;															# add the eeprom address, different while slcLen is depending on amount of peers
		$pAddr += $h->{'slcCnt'} * int($maxPeer);														# adjust the peer db address
				
		$cnt++;
	}
}


# == print out =========================================================================================

print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- channel slice definition ---------------------------------------------------------------------------------------------\n";
print "uint8_t sliceStr[] = {\n";

my $slcLenCnt = 0;
foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
	my $h = $cnlLstType{$cnt};
	$slcLenCnt += $h->{'slcCnt'};
	print "    $h->{'slcStrItem'}\n";
}
print "}; // $slcLenCnt byte \n\n\n";


print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- Channel device config ------------------------------------------------------------------------------------------------\n";

foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
	my $h = $cnlLstType{$cnt};
	print "struct s_$h->{'cnlLstType'} {\n";
	print "    // $h->{'slcStrItem'}\n";
	print "$h->{'devStrItem'}};\n\n";
}

foreach my $i (keys @cnlTypeA) {
	print "struct s_$cnlTypeA[$i] {\n";
	foreach my $cnt (sort{$a <=> $b}(keys %cnlLstType)) {
		if ($cnlLstType{$cnt}{'cnlType'} eq $cnlTypeA[$i]) {
			print "    s_$cnlLstType{$cnt}{'cnlLstType'} l$cnlLstType{$cnt}{'lst'};\n";
		}
	}
	print "};\n\n";
}

print "struct s_regs {\n";
foreach my $cnl (sort{$a <=> $b}(keys %regList)) {
	print "    s_$regList{$cnl}{'type'} ch$cnl;\n";
}
print "} regs; // "  .$pAddr ." byte\n\n\n";


print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- channel device list table --------------------------------------------------------------------------------------------\n";
print "s_cnlDefType cnlDefType[] PROGMEM = {\n";
print "    // cnl, lst, pMax, sIdx, sLen, pAddr, pPeer, *pRegs;																	// pointer to regs structure\n\n";

foreach my $cnt (sort{$a <=> $b}(keys %cnlDefIndex)) {
	my $h = $cnlDefIndex{$cnt};
	print "    {$h->{cnl}, $h->{lst}, $h->{pMax}, " .sprintf("0x%.2x", $h->{slcIdx}) .", $h->{slcLen}, ";
	print sprintf("0x%.4x", $h->{pAddr}) .", " .sprintf("0x%.4x", $h->{pPeer})  .", (void*)&regs.ch$h->{cnl}.l$h->{lst}},\n"; #.", $h->{slcStrItem},    \n";	
}
print "}; // " .(scalar( keys %cnlDefIndex)*11) ." byte \n\n\n";


print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- handover to AskSin lib -----------------------------------------------------------------------------------------------\n";
print "HM::s_devDef dDef = {\n";
print "    " .(scalar( keys %regLfull)-1) .", " .scalar( keys %cnlDefIndex) .", sliceStr, cnlDefType,\n";
print "}; // 6 byte\n\n\n";


print "//- ----------------------------------------------------------------------------------------------------------------------\n";
print "//- eeprom definition ----------------------------------------------------------------------------------------------------\n";
print "// define start address  and size in eeprom for magicNumber, peerDB, regsDB, userSpace  \n";
print "HM::s_eeprom ee[] = {\n";
print "    {" .sprintf("0x%.4x", 0) .", " .sprintf("0x%.4x", 2)      .", " .sprintf("0x%.4x", 2+$pPeer) .", " .sprintf("0x%.4x", 2+$pPeer+$pAddr) .",},\n";  
print "    {" .sprintf("0x%.4x", 2) .", " .sprintf("0x%.4x", $pPeer) .", " .sprintf("0x%.4x", $pAddr)   .", " .sprintf("0x%.4x", 0)               .",},\n";  
print "}; // 16 byte\n\n\n";

# something for default settings....
