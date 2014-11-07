use strict;
#Beispiel 
# ========================switch =====================================
# 1 device
# 2 kanäle,  alle identisch
# 6 peers je kanal erlaubt
#----------------define reglist types-----------------
package usrRegs;
my %listTypes = (
      regDev =>{ intKeyVisib=>1,pairCentral=>1
               },
      regChan       =>{sign            =>1,
                       OnTime          =>1,OffTime         =>1,OnDly           =>1,OffDly          =>1,
                       SwJtOn          =>1,SwJtOff         =>1,SwJtDlyOn       =>1,SwJtDlyOff      =>1,
                       CtValLo         =>1,CtValHi         =>1,
                       CtOn            =>1,CtDlyOn         =>1,CtOff           =>1,CtDlyOff        =>1,
		               ActionType      =>1,OnTimeMode      =>1,OffTimeMode     =>1,
 		               lgMultiExec     =>1
		               },
     );
#      -----------assemble device -----------------
my %regList;
$regList{0}={type => "regDev",peers=>1};
$regList{1}={type => "regChan",peers=>6};
$regList{2}={type => "regChan",peers=>6};

sub usr_getHash($){
  my $hn = shift;
  return %regList       if($hn eq "regList"      );
  return %listTypes     if($hn eq "listTypes"       );
}
