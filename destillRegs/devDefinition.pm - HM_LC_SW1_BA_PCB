use strict;
#Beispiel 
# ========================switch =====================================
# 
#   "006C" => {name=>"HM-LC-SW1-BA-PCB"        ,st=>'switch'            ,cyc=>''      ,rxt=>'b'      ,lst=>'3'            ,chn=>"",},
# 1 device
# 1 kanal
# 6 peers je kanal erlaubt
#----------------define reglist types-----------------
package usrRegs;
my %listTypes = (
      regDev =>{ intKeyVisib=>1, ledMode=>1, lowBatLimitBA=>1, pairCentral=>1,
              },
      regChan       =>{ sign          =>1,    #|     literal        |          | signature (AES) options:on,off
                        lgActionType  =>1,    #|     literal        | required |  options:toggleToCntInv,off,toggleToCnt,jmpToTarget
                        lgCtDlyOff    =>1,    #|     literal        | required | Jmp on condition from delayOff options:geLo,between,outside,ltLo,geHi,ltHi
                        lgCtDlyOn     =>1,    #|     literal        | required | Jmp on condition from delayOn options:geLo,between,outside,ltLo,geHi,ltHi
												lgCtOff       =>1,    #|     literal        | required | Jmp on condition from off options:geLo,between,outside,ltLo,geHi,ltHi
                        lgCtOn        =>1,    #|     literal        | required | Jmp on condition from on options:geLo,between,outside,ltLo,geHi,ltHi
												lgCtValHi     =>1,    #|   0 to 255         | required | Condition value high for CT table
                        lgCtValLo     =>1,    #|   0 to 255         | required | Condition value low for CT table
												lgMultiExec   =>1,    #|     literal        | required | multiple execution per repeat of long trigger options:on,off
												lgOffDly      =>1,    #|   0 to 111600s     | required | off delay
												lgOffTime     =>1,    #|   0 to 111600s     | required | off time, 111600 = infinite
												lgOffTimeMode =>1,    #|     literal        | required | off time mode options:minimal,absolut
												lgOnDly       =>1,    #|   0 to 111600s     | required | on delay
												lgOnTime      =>1,    #|   0 to 111600s     | required | on time, 111600 = infinite
												lgOnTimeMode  =>1,    #|     literal        | required | on time mode options:minimal,absolut
												lgSwJtDlyOff  =>1,    #|     literal        | required | Jump from delayOff options:on,off,dlyOn,no,dlyOff
												lgSwJtDlyOn   =>1,    #|     literal        | required | Jump from delayOn options:on,off,dlyOn,no,dlyOff
												lgSwJtOff     =>1,    #|     literal        | required | Jump from off options:on,off,dlyOn,no,dlyOff
												lgSwJtOn      =>1,    #|     literal        | required | Jump from on options:on,off,dlyOn,no,dlyOff
												shActionType  =>1,    #|     literal        | required |  options:toggleToCntInv,off,toggleToCnt,jmpToTarget
												shCtDlyOff    =>1,    #|     literal        | required | Jmp on condition from delayOff options:geLo,between,outside,ltLo,geHi,ltHi
												shCtDlyOn     =>1,    #|     literal        | required | Jmp on condition from delayOn options:geLo,between,outside,ltLo,geHi,ltHi
												shCtOff       =>1,    #|     literal        | required | Jmp on condition from off options:geLo,between,outside,ltLo,geHi,ltHi
												shCtOn        =>1,    #|     literal        | required | Jmp on condition from on options:geLo,between,outside,ltLo,geHi,ltHi
												shCtValHi     =>1,    #|   0 to 255         | required | Condition value high for CT table
												shCtValLo     =>1,    #|   0 to 255         | required | Condition value low for CT table
												shOffDly      =>1,    #|   0 to 111600s     | required | off delay
												shOffTime     =>1,    #|   0 to 111600s     | required | off time, 111600 = infinite
												shOffTimeMode =>1,    #|     literal        | required | off time mode options:minimal,absolut
												shOnDly       =>1,    #|   0 to 111600s     | required | on delay
												shOnTime      =>1,    #|   0 to 111600s     | required | on time, 111600 = infinite
												shOnTimeMode  =>1,    #|     literal        | required | on time mode options:minimal,absolut
												shSwJtDlyOff  =>1,    #|     literal        | required | Jump from delayOff options:on,off,dlyOn,no,dlyOff
												shSwJtDlyOn   =>1,    #|     literal        | required | Jump from delayOn options:on,off,dlyOn,no,dlyOff
												shSwJtOff     =>1,    #|     literal        | required | Jump from off options:on,off,dlyOn,no,dlyOff
												shSwJtOn      =>1,    #|     
		          },
     );
#      -----------assemble device -----------------
my %regList;
$regList{0}={type => "regDev",peers=>1};
$regList{1}={type => "regChan",peers=>6};

sub usr_getHash($){
  my $hn = shift;
  return %regList       if($hn eq "regList"      );
  return %listTypes     if($hn eq "listTypes"       );
}
