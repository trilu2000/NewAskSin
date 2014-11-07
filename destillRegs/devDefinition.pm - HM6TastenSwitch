use strict;
#Beispiel 
# ========================switch =====================================
# battery powered 6 channel switch
# "00A9" => {name=>"HM-PB-6-WM55"            ,st=>'remote'            ,cyc=>''      ,rxt=>'c'      ,lst=>'1,4'          ,chn=>"Btn:1:6",},
# 1 device
# 6 kanäle,  alle identisch
# 6 peers je kanal erlaubt
#----------------define reglist types-----------------
package usrRegs;
my %listTypes = (
      regDev =>{ burstRx=>1,intKeyVisib=>1,pairCentral=>1,localResDis=>1,
              },
      regChan =>{sign=>1, longPress=>1, dblPress=>1,
                      peerNeedsBurst=>1, expectAES=>1,
		          },
     );
#      -----------assemble device -----------------
my %regList;
$regList{0}={type => "regDev",peers=>1};
$regList{1}={type => "regChan",peers=>6};
$regList{2}={type => "regChan",peers=>6};
$regList{3}={type => "regChan",peers=>6};
$regList{4}={type => "regChan",peers=>6};
$regList{5}={type => "regChan",peers=>6};
$regList{6}={type => "regChan",peers=>6};

sub usr_getHash($){
  my $hn = shift;
  return %regList       if($hn eq "regList"      );
  return %listTypes     if($hn eq "listTypes"       );
}
