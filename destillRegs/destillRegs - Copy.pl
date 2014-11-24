use strict;

use RegConfig;
# ========================import constants=====================================
my %culHmRegDefShLg       =HMConfig::HMConfig_getHash("culHmRegDefShLg");
my %culHmRegDefine        =HMConfig::HMConfig_getHash("culHmRegDefine");

use devDefinition;
my %regList       =usrRegs::usr_getHash("regList");
my %listTypes     =usrRegs::usr_getHash("listTypes");

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

################### definitions  -  - user range###################


################### definitions - system range###################
CUL_HM_initRegHash();
#++++++++++++++ compilation++++++++++++++++++++

my %regLfull;
my %regTypeDef;

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
	  $regLfull{$chn}{$lst}{"empty$cnt"}{$culHmRegDefine{$reg}{a}} =$reg;
	}
  }
}

my ($sAddr,$sEnd,$sSize,$nbrSlices);
my ($maxChn,$maxList,$maxPeer) = (0,0,0);
my ($bitLoc,$missBits) = (0,0);
my ($nbrSlicesTotal,$phyAddr,$phySize) = (0,0,0);

#------------------create slice table----------------
my $sliceStrTpl = "struct s_sliceStrTpl {\n".
                  "\tunsigned char regAddr;\n".
                  "\tunsigned char nbrBytes;\n".
                  "\tunsigned short phyAddr;\n".
                  "};\n";
my $sliceArray = "\n\n// regAddr,nbrBytes,phyAddr\n".
                 "const s_sliceStrTpl sliceStr[] = {\n";
my %regTypesStruct;

my $lastReg;
foreach my $chn (sort{$a <=> $b}(keys %regLfull)){
  $maxChn = $chn;
  my $tSize = 0;
  
  # writing struct headers
  if ($regList{$chn}{peers} > 1) {
    $regTypesStruct{$regList{$chn}{type}}{xstr} = "struct s_$regList{$chn}{type} {\n";      # reset regType

    $regTypesStruct{$regList{$chn}{type}}{str} = "struct s_dev_$regList{$chn}{type} {\n";   # reset regType
  } else {
    $regTypesStruct{$regList{$chn}{type}}{str} = "struct s_$regList{$chn}{type} {\n";       # reset regType
  }
  my $regHash = $regTypesStruct{$regList{$chn}{type}};                          # in regHash we will store the channel device config
  
  # stepping through the different lists
  foreach my $lst (sort{$a <=> $b}(keys %{$regLfull{$chn}})){
    $maxList = $lst if($maxList < $lst);
    my $comment = "\t// chn:$chn lst:$lst";
    my ($nbrPeers,$peersDone,$peerIndent) = (0,0,"");
    no strict;
    $nbrPeers = scalar keys%{$regLfull{$chn}{$lst}};
    use strict;
	
    # if we have more then one peer in the channel list, we need a special type in regHash
    if ($nbrPeers > 1){
      my $tCmt = "struct s_peer_$regList{$chn}{type} {";
      $tCmt .= " "x(34-length($tCmt));
      $regHash->{peer} =  $tCmt ."\t// chn:$chn, lst:$lst \n";
      #$regHash->{peer} = "struct s_peer_$regList{$chn}{type} {\t\t/*chn: $chn/$lst*/\n";
      $regHash->{peerMax} = $nbrPeers;
      $peerIndent = 1;
    }
    
    # stepping through the peers in channel list
    foreach my $peer (sort(keys %{$regLfull{$chn}{$lst}})){
      ($sAddr,$nbrSlices) = ("",0);
      $maxPeer = scalar( keys %{$regLfull{$chn}{$lst}}) if($maxPeer < scalar( keys %{$regLfull{$chn}{$lst}}));
    
      foreach my $reg (sort{$a <=> $b}(keys %{$regLfull{$chn}{$lst}{$peer}})){
        my $regPrt = $culHmRegDefine{$regLfull{$chn}{$lst}{$peer}{$reg}};       #shortcut
        my $regHead;
        if (!$sAddr){                                                           # this is a new list

		      $sAddr = int($reg);
		      $sEnd = $sAddr + int($regPrt->{s}+.99)-1;
		      $regHead = "regs: ";

		    } else {
          
          # construct slice array
          if (int($sEnd+1) >= int($reg)){                                       #this is a new byte
		        $sEnd = int($reg) + int($regPrt->{s}+0.99)-1;
		        $regHead = "regc: ";
		      } else {                                                              #begin of slice
            $sliceArray .= sprintf("\t{0x%02x, 0x%02x, 0x%04x}," ." "x6 ."%s\n",$sAddr,$sSize,$phyAddr ,$comment);
            $comment = "";
            $nbrSlices++;
            $tSize += $sSize;
            $phyAddr += $sSize;
            $sAddr = int($reg);
            $sEnd = $sAddr + int($regPrt->{s}+0.99)-1;
            $regHead = "regx: ";
          }
        }
 
        # fill the structs for channel device config
        if (!$peersDone){
          # check the type of the registers 
          my ($regSize,$regType,$regVar,$mBitsBefore);
          $regSize = ($regPrt->{s}>0.99)?int($regPrt->{s}*8):int($regPrt->{s}*10);        # calculate the bits of the respective variable
          
          $regVar = $regLfull{$chn}{$lst}{$peer}{$reg};                         # copy the reg text to a variable
          if ($regSize > 16) {                                                  # should be only pairCentral
            $regType = "uint8_t ";
            $regVar .= "[".($regSize/8)."]";                                    # type byte array
          } elsif ($regSize > 8) {
            $regType = "uint16_t";
          } else {
            $regType = "uint8_t ";
          }

          # make strings similar in length; max. size of variable = 16, so lets try 20, tab size = 4
          $regVar .= ($regSize<8)?"":";";                                       #add a semicolon while no bits to add 
          $regVar .=  " "x(20-length($regVar));
					$regVar .= ($regSize<8)?"":"   ";
          
          my ($mBitB,$mBitA); 	 
          # calculate the bits before and after
          if (int($lastReg) == int($reg)) {                                     # we are in the same register
            $mBitsBefore = int(($reg - $lastReg)*10);                           # missing bits are the difference between last and current
            $mBitB = 0;
            $mBitA = 0;
          
          } else {
            $mBitB = int(int($reg*10) - int($regPrt->{a})*10);                  # calc the missing bits before a variable
            $mBitA = int(8-(int($lastReg*10)-(int($lastReg)*10)));              # calc last registers missing bits
            if ($mBitA == 8) {$mBitA = 0};
            $mBitsBefore = (($mBitB<8)&&($mBitB>0))?$mBitB+$mBitA:$mBitB;
          }


          # write the variables to the structs
          my $stpe = ($peerIndent)?"peer":"str";                                # str or peer variable
          #$regHash->{$stpe} .= "l:" .$lastReg .", r:" .(int($lastReg*10)-(int($lastReg)*10)) .", b:" .$mBitB .", a:" .$mBitA ."\n";
          $regHash->{$stpe} .= ($mBitsBefore)?"\tuint8_t " ." "x20 .":" .$mBitsBefore .";\n":"";
          $regHash->{$stpe} .= "\t" .$regType .$regVar;  
          $regHash->{$stpe} .= ($regSize < 8)?":" .$regSize .";":"";
          $regHash->{$stpe} .= "\t// reg:"  .sprintf("0x%02X",$reg);            #.", b:$mBitsBefore, u:$regSize, a:$mBitsAfter  " ;#."\n";
          $regHash->{$stpe} .= ", sReg:" .$reg ."\n";                           #"lastReg:" .$lastReg 

          $lastReg = ($regSize<8)?($regSize/10)+$reg:+$reg;                     # remember reg variable
          #$regHash->{$stpe} .= "l:" .$lastReg ."\n";  
        
        }
        $sSize = $sEnd - $sAddr + 1;
      }
      $lastReg = "";


      # preparing slice array
      $sliceArray .= sprintf("\t{0x%02x, 0x%02x, 0x%04x}," ." "x6 ."%s\n",$sAddr,$sSize,$phyAddr ,$comment);
      $comment = "";
      $nbrSlices++;
      $tSize += $sSize;
      $phyAddr += $sSize;
      $nbrSlicesTotal +=$nbrSlices;
      $regTypeDef{$regList{$chn}{type}}{lists}{$lst}{nbrSlice} = $nbrSlices;
      $regTypeDef{$regList{$chn}{type}}{lists}{$lst}{peers} = ($lst == 3 || $lst == 4)?$regList{$chn}{peers}:1;

      # close the peer struct
      $regHash->{peer} .= "};\n" if ($peerIndent && !$peersDone);

      $peersDone = 1;# print only once per peer
    }
  }

  # special type to make channel 1 to x available
  if ($regHash->{peerMax}){
    $regHash->{xstr} .= "\ts_dev_$regList{$chn}{type}  list1;\n";
    $regHash->{xstr} .= "\ts_peer_$regList{$chn}{type} peer[$regHash->{peerMax}];\n";
    $regHash->{xstr} .="};\n";
  }
  
  # close the channel x, list0 and list1
  $regHash->{str} .="};\n";

#  if ($regList{$chn}{peers} > 1) {
#    $regHash->{xstr} .="};\n";
#  }
  $regTypeDef{$regList{$chn}{type}}{phySize} = $tSize; # sice of the type
  $regTypeDef{$regList{$chn}{type}}{nbrLists} = scalar(keys %{$regLfull{$chn}});

  #build s_regChan struct
  #$regTypesStruct{$regList{$chn}{type}}{str} = "struct s_$regList{$chn}{type} {\n"; # reset regType
}
$sliceArray .= "};\n";

#------------------create channel templates----------------
my @regLTypes;#collect different types and generate unique list
push @regLTypes,$regList{$_}{type} foreach (keys %regList);
  my %all;
  $all{$_}=0 foreach (grep !/^$/,@regLTypes);
  delete $all{""}; #remove empties if present
  @regLTypes = sort keys %all;
my $maxlstTml = 0;
foreach my $rlt (@regLTypes){ 
  my $x =scalar keys %{$regTypeDef{$rlt}{lists}};
  $maxlstTml = $x if ($x > $maxlstTml);
}

my $listDefSize = 0;
my $listStrTpl = "struct s_listTpl {\n".
                 "\tunsigned char ListNo;\n".
                 "\tunsigned char nbrOfSlice;\n".
                 "\tunsigned char nbrPeers;\n".
                 "};\n";
my $listTypArray = "struct {\n"
                  ."\tunsigned char nbrLists;    \t// number of lists for this channel\n"
                  ."\tstruct s_listTpl type[$maxlstTml];  \t// fill data with lists\n"
                  ."} const listTypeDef[" .scalar(@regLTypes) ."] = {\n";

foreach my $rlt (@regLTypes){ 
  $listTypArray .= "\t"
                  .($listDefSize?",":"") # not for first enrty
                  ."{ $regTypeDef{$rlt}{nbrLists}" ." "x23 ."\t// type $rlt\n"
                  ."\t\t,{\n";
  $listDefSize++;
  my $entry = 0;
  foreach my $listNo (sort (keys %{$regTypeDef{$rlt}{lists}})){
    my $nbrSlice = $regTypeDef{$rlt}{lists}{$listNo}{nbrSlice};
    $listTypArray .= "\t\t" .($entry++?",":" ") ."{$listNo,$nbrSlice"
	                .",".$regTypeDef{$rlt}{lists}{$listNo}{peers} ."}\n"; 
	$listDefSize += 3;
  }
  
  foreach (0..($maxlstTml - $entry)){
    $listTypArray .= "\t\t,{0,0,0}\n" 
	      if($_);
  }
  $listTypArray .= "\t\t}\n"
                  ."\t}\n";
}
$listTypArray .= "};\n";

#------------------channels in the device----------------
#- const devDef ----------------------------------------------------------------
my $devDefSize = 1;
my $devDef = "struct {\n"
            ."\tunsigned char  nbrChannels;\n"
            ."\ts_chDefType chDefType[".scalar (keys %regList)."];\n"
            ."} const devDef = {\n"
            ."\t".scalar (keys %regList) ." "x25 ."\t// number of channels\n"
            ."\t,{\n";

my $chnPhyAddr = 0;
my $sSlc = 0;
#my $regStr = "";
foreach my $chn (sort keys %regList){
  my( $index )= grep { $regLTypes[$_] eq $regList{$chn}{type} } 0..@regLTypes;
  $devDef .="\t\t" .(($chn)?",":" ")
          ."{$index,$chnPhyAddr,$sSlc}" ." "x13 ."\t// chn:$chn type:$regLTypes[$index]\n";

#  $regStr .= "\ts_$regLTypes[$index] ch_$chn;\n";
  $chnPhyAddr+=$regTypeDef{$regLTypes[$index]}{phySize};
  $devDefSize += 5;
  foreach my $listNo (sort (keys %{$regTypeDef{$regLTypes[$index]}{lists}})){
    $sSlc += $regTypeDef{$regLTypes[$index]}{lists}{$listNo}{nbrSlice} *
             $regTypeDef{$regLTypes[$index]}{lists}{$listNo}{peers};
  }
}
$devDef .= "\t}\n};\n";


#------------------print output----------------
$phySize = $phyAddr;



########## gen code##############
# Slice Table ##################################################################
print "\n";
print "//- -----------------------------------------------------------------------------------------------------------------------\n";
print "//- channel slice definition ----------------------------------------------------------------------------------------------\n";
#- s_chDefType -----------------------------------------------------------------
print "struct s_chDefType{\n"
      ."\tunsigned char  type; \n"
      ."\tunsigned short phyAddr;\n"
      ."\tunsigned short sliceIdx;\n"
      ."};\n";

print $devDef;
print $sliceStrTpl;
print $sliceArray;

print $listStrTpl;
print $listTypArray;



# Peer Database ################################################################
print "\n";
print "//- -----------------------------------------------------------------------------------------------------------------------\n";
print "// - peer db config -------------------------------------------------------------------------------------------------------\n";
print "#define maxChannel $maxChn\n";
print "#define maxPeer    ".($maxPeer)."\n";
print "static uint32_t peerdb[maxChannel][maxPeer];\n";

my @mp;
foreach my $p (sort keys %regList) {
  if (!$p) {next;}
  push @mp,$regList{$p}{peers};
}

print "const uint8_t peermax[] = {"
       .join(",",@mp)
	   ."};\n";
#	   %regList;



# Channel device ###############################################################
print "\n";
print "//- -----------------------------------------------------------------------------------------------------------------------\n";
print "// - Channel device config ------------------------------------------------------------------------------------------------\n";

foreach (keys %regTypesStruct){
  print $regTypesStruct{$_}{peer} if ($regTypesStruct{$_}{peer});
  print $regTypesStruct{$_}{str};
  print $regTypesStruct{$_}{xstr};
}


print "\nstruct s_regs {\n";
foreach my $chn (sort{$a <=> $b}(keys %regLfull)){
  print "\ts_$regList{$chn}{type} ch_$chn;\n";
}
print "};\n";

print "\nstruct s_EEPROM {\n"
      ."\tunsigned short magNbr;\n"
		  ."\tuint32_t peerdb[maxChannel][maxPeer];\n"
		  ."\ts_regs regs;\n"
		  ."};\n";



# Register user space ##########################################################
print "\n";
print "//- -----------------------------------------------------------------------------------------------------------------------\n";
print "//- struct to provide register settings to user sketch --------------------------------------------------------------------\n";

my $lChn = "";                                                                  # remember last channel to avoid same channel names in one struct
foreach my $chn (sort{$a <=> $b}(keys %regLfull)){                              # step through the whole channel list
  if ($chn == 0) {next;}                                                        # if channel is device channel then skip
  if ($lChn eq $regList{$chn}{type}) {next;}                                    # if current channel has the name of the last channel then skip
  $lChn = $regList{$chn}{type};                                                 # remember the last channel
  print "\nstruct s_cpy_$regList{$chn}{type} {\n";                              # print header line of struct
  
  foreach my $lst (sort{$a <=> $b}(keys %{$regLfull{$chn}})){                   # now we are stepping through the lists
    if (($lst == 3) || ($lst == 4)) {
      print "\ts_peer_$regList{$chn}{type} l$lst;\n";
    } else {
      print "\ts_dev_$regList{$chn}{type}  l$lst;\n";
    }
  }
}
print "};\n";

print "\nstruct s_regCpy {\n";
foreach my $chn (sort{$a <=> $b}(keys %regLfull)){
  if ($chn == 0) {
    print "\ts_$regList{$chn}{type}    ch$chn;\n";
  } else {
    print "\ts_cpy_$regList{$chn}{type} ch$chn;\n";
  }
}
print "} static regMC;\n";

print "\nstatic uint16_t regMcPtr[] = { \n";
foreach my $chn (sort{$a <=> $b}(keys %regLfull)){
  foreach my $lst (sort{$a <=> $b}(keys %{$regLfull{$chn}})){
    if ($lst == 0) {
      print "\t(uint16_t)&regMC.ch$chn,\n";
    } else {
      print "\t(uint16_t)&regMC.ch$chn.l$lst,\n";
    }
  }
}
print "};\n";



# Notes on config ##############################################################
my $constSize = $nbrSlicesTotal * 4;
$constSize += $listDefSize;
$constSize += $devDefSize;
$constSize += ($maxChn+ 1);#peerMaxTable

$phySize += 4*($maxChn+ 1)*$maxPeer;# add peer table space
$phySize += 2;# add magic number
print "\n";
print "//- -----------------------------------------------------------------------------------------------------------------------\n";
print "//- Device definition -----------------------------------------------------------------------------------------------------\n";
print "//\t  Channels:      ".($maxChn+1)
       ."\n//\t  highest List:  $maxList"
	   ."\n//\t  possible peers:$maxPeer"
       ."\n//- Memory usage"
	   ."\n//\t  Slices:$nbrSlicesTotal"
	   ."\n//\t  EEPROM size:$phySize"
	      .($phySize > 1024?" need external EEPROM":" fits internal")
	   ."\n//\t  const size: $constSize"
	   ."\n";

