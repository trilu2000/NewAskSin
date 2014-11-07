use strict;
#Beispiel 
# ========================switch =====================================
# battery powered 1 channel temperature
#  "003D" => {name=>"HM-WDS10-TH-O"           ,st=>'THSensor'          ,cyc=>'00:10' ,rxt=>'c:w:f'  ,lst=>'p'            ,chn=>"",},
# 1 device
# 1 kanal
# 6 peers je kanal erlaubt
#----------------define reglist types-----------------
package usrRegs;
my %listTypes = (
      regDev =>{ intKeyVisib=>1, burstRx=>1, pairCentral=>1,
              },
      regChan       =>{  
                      characteristic          =>1, #  |     literal        |          |  options:linear,square
                      logicCombination        =>1, #  |     literal        |          |  options:inactive,or,nor,mul,mulinv,minusinv,invMinus,andinv,minus,invMul,orinv,plus,xor,plusinv,nand,invPlus,and
                      ovrTempLvl              =>1, #  |  30 to 100C        |          | overtemperatur level
                      powerUpAction           =>1, #  |     literal        |          | behavior on power up options:on,off
                      redLvl                  =>1, #  |   0 to 100%        |          | reduced power level
                      redTempLvl              =>1, #  |  30 to 100C        |          | reduced temperatur recover
                      statusInfoMinDly        =>1, #  | 0.5 to 15.5s       |          | status message min delay
                      statusInfoRandom        =>1, #  |   0 to 7s          |          | status message random delay
                      transmitTryMax          =>1, #  |   1 to 10          |          | max message re-transmit
                      lgActionTypeDim         =>1, #  |     literal        | required |  options:toggleToCntInv,downDim,off,toggelDimToCnt,jmpToTarget,toggelDim,upDim,toggleToCnt,toggelDimToCntInv
                      lgCtDlyOff              =>1, #  |     literal        | required | Jmp on condition from delayOff options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtDlyOn               =>1, #  |     literal        | required | Jmp on condition from delayOn options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtOff                 =>1, #  |     literal        | required | Jmp on condition from off options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtOn                  =>1, #  |     literal        | required | Jmp on condition from on options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtRampOff             =>1, #  |     literal        | required | Jmp on condition from rampOff options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtRampOn              =>1, #  |     literal        | required | Jmp on condition from rampOn options:geLo,between,outside,ltLo,geHi,ltHi
                      lgCtValHi               =>1, #  |   0 to 255         | required | Condition value high for CT table
                      lgCtValLo               =>1, #  |   0 to 255         | required | Condition value low for CT table
                      lgDimElsActionType      =>1, #  |     literal        | required |  options:toggleToCntInv,downDim,off,toggelDimToCnt,jmpToTarget,toggelDim,upDim,toggleToCnt,toggelDimToCntInv
                      lgDimElsJtDlyOff        =>1, #  |     literal        | required | else Jump from delayOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsJtDlyOn         =>1, #  |     literal        | required | else Jump from delayOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsJtOff           =>1, #  |     literal        | required | else Jump from off options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsJtOn            =>1, #  |     literal        | required | else Jump from on options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsJtRampOff       =>1, #  |     literal        | required | else Jump from rampOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsJtRampOn        =>1, #  |     literal        | required | else Jump from rampOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimElsOffTimeMd       =>1, #  |     literal        | required |  options:minimal,absolut
                      lgDimElsOnTimeMd        =>1, #  |     literal        | required |  options:minimal,absolut
                      lgDimJtDlyOff           =>1, #  |     literal        | required | Jump from delayOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimJtDlyOn            =>1, #  |     literal        | required | Jump from delayOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimJtOff              =>1, #  |     literal        | required | Jump from off options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimJtOn               =>1, #  |     literal        | required | Jump from on options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimJtRampOff          =>1, #  |     literal        | required | Jump from rampOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimJtRampOn           =>1, #  |     literal        | required | Jump from rampOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      lgDimMaxLvl             =>1, #  |   0 to 100%        | required | dimMaxLevel
                      lgDimMinLvl             =>1, #  |   0 to 100%        | required | dimMinLevel
                      lgDimStep               =>1, #  |   0 to 100%        | required | dimStep
                      lgMultiExec             =>1, #  |     literal        | required | multiple execution per repeat of long trigger options:on,off
                      lgOffDly                =>1, #  |   0 to 111600s     | required | off delay
                      lgOffDlyBlink           =>1, #  |     literal        | required |  options:on,off
                      lgOffDlyNewTime         =>1, #  | 0.1 to 25.6s       | required | off delay new time
                      lgOffDlyOldTime         =>1, #  | 0.1 to 25.6s       | required | off delay old time
                      lgOffLevel              =>1, #  |   0 to 100%        | required | PowerLevel off
                      lgOffTime               =>1, #  |   0 to 111600s     | required | off time, 111600 = infinite
                      lgOffTimeMode           =>1, #  |     literal        | required | off time mode options:minimal,absolut
                      lgOnDly                 =>1, #  |   0 to 111600s     | required | on delay
                      lgOnDlyMode             =>1, #  |     literal        | required |  options:setToOff,NoChange
                      lgOnLevel               =>1, #  |   0 to 100%        | required | PowerLevel on
                      lgOnLvlPrio             =>1, #  |     literal        | required |  options:high,low
                      lgOnMinLevel            =>1, #  |   0 to 100%        | required | minimum PowerLevel
                      lgOnTime                =>1, #  |   0 to 111600s     | required | on time, 111600 = infinite
                      lgOnTimeMode            =>1, #  |     literal        | required | on time mode options:minimal,absolut
                      lgRampOffTime           =>1, #  |   0 to 111600s     | required | rampOffTime
                      lgRampOnTime            =>1, #  |   0 to 111600s     | required | rampOnTime
                      lgRampSstep             =>1, #  |   0 to 100%        | required | rampStartStep
                      shActionTypeDim         =>1, #  |     literal        | required |  options:toggleToCntInv,downDim,off,toggelDimToCnt,jmpToTarget,toggelDim,upDim,toggleToCnt,toggelDimToCntInv
                      shCtDlyOff              =>1, #  |     literal        | required | Jmp on condition from delayOff options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtDlyOn               =>1, #  |     literal        | required | Jmp on condition from delayOn options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtOff                 =>1, #  |     literal        | required | Jmp on condition from off options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtOn                  =>1, #  |     literal        | required | Jmp on condition from on options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtRampOff             =>1, #  |     literal        | required | Jmp on condition from rampOff options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtRampOn              =>1, #  |     literal        | required | Jmp on condition from rampOn options:geLo,between,outside,ltLo,geHi,ltHi
                      shCtValHi               =>1, #  |   0 to 255         | required | Condition value high for CT table
                      shCtValLo               =>1, #  |   0 to 255         | required | Condition value low for CT table
                      shDimElsActionType      =>1, #  |     literal        | required |  options:toggleToCntInv,downDim,off,toggelDimToCnt,jmpToTarget,toggelDim,upDim,toggleToCnt,toggelDimToCntInv
                      shDimElsJtDlyOff        =>1, #  |     literal        | required | else Jump from delayOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsJtDlyOn         =>1, #  |     literal        | required | else Jump from delayOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsJtOff           =>1, #  |     literal        | required | else Jump from off options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsJtOn            =>1, #  |     literal        | required | else Jump from on options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsJtRampOff       =>1, #  |     literal        | required | else Jump from rampOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsJtRampOn        =>1, #  |     literal        | required | else Jump from rampOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimElsOffTimeMd       =>1, #  |     literal        | required |  options:minimal,absolut
                      shDimElsOnTimeMd        =>1, #  |     literal        | required |  options:minimal,absolut
                      shDimJtDlyOff           =>1, #  |     literal        | required | Jump from delayOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimJtDlyOn            =>1, #  |     literal        | required | Jump from delayOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimJtOff              =>1, #  |     literal        | required | Jump from off options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimJtOn               =>1, #  |     literal        | required | Jump from on options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimJtRampOff          =>1, #  |     literal        | required | Jump from rampOff options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimJtRampOn           =>1, #  |     literal        | required | Jump from rampOn options:on,rampOn,off,dlyOn,no,dlyOff,rampOff
                      shDimMaxLvl             =>1, #  |   0 to 100%        | required | dimMaxLevel
                      shDimMinLvl             =>1, #  |   0 to 100%        | required | dimMinLevel
                      shDimStep               =>1, #  |   0 to 100%        | required | dimStep
                      shOffDly                =>1, #  |   0 to 111600s     | required | off delay
                      shOffDlyBlink           =>1, #  |     literal        | required |  options:on,off
                      shOffDlyNewTime         =>1, #  | 0.1 to 25.6s       | required | off delay new time
                      shOffDlyOldTime         =>1, #  | 0.1 to 25.6s       | required | off delay old time
                      shOffLevel              =>1, #  |   0 to 100%        | required | PowerLevel off
                      shOffTime               =>1, #  |   0 to 111600s     | required | off time, 111600 = infinite
                      shOffTimeMode           =>1, #  |     literal        | required | off time mode options:minimal,absolut
                      shOnDly                 =>1, #  |   0 to 111600s     | required | on delay
                      shOnDlyMode             =>1, #  |     literal        | required |  options:setToOff,NoChange
                      shOnLevel               =>1, #  |   0 to 100%        | required | PowerLevel on
                      shOnLvlPrio             =>1, #  |     literal        | required |  options:high,low
                      shOnMinLevel            =>1, #  |   0 to 100%        | required | minimum PowerLevel
                      shOnTime                =>1, #  |   0 to 111600s     | required | on time, 111600 = infinite
                      shOnTimeMode            =>1, #  |     literal        | required | on time mode options:minimal,absolut
                      shRampOffTime           =>1, #  |   0 to 111600s     | required | rampOffTime
                      shRampOnTime            =>1, #  |   0 to 111600s     | required | rampOnTime
                      shRampSstep             =>1, #  |   0 to 100%        | required | rampStartStep 
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
