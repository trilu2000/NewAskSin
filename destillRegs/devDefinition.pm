use strict;
#Beispiel 

#----------------define reglist types-----------------
package usrRegs;

# sub types - needed in register.h
#  "01" => { st => "AlarmControl",
#  "10" => { st => "switch",
#  "12" => { st => "outputUnit",
#  "20" => { st => "dimmer",
#  "30" => { st => "blindActuator",
#  "39" => { st => "ClimateControl",
#  "40" => { st => "remote",
#  "41" => { st => "sensor",
#  "42" => { st => "swi",
#  "43" => { st => "pushButton",
#  "44" => { st => "singleButton",
#  "51" => { st => "powerMeter",
#  "58" => { st => "thermostat",
#  "60" => { st => "KFM100",
#  "70" => { st => "THSensor",
#  "80" => { st => "threeStateSensor"
#  "81" => { st => "motionDetector",
#  "C0" => { st => "keyMatic",
#  "C1" => { st => "winMatic",
#  "C3" => { st => "tipTronic",
#  "CD" => { st => "smokeDetector",

my %listTypes = (
	#regDev    => { burstRx=>1, intKeyVisib=>1, pairCentral=>1, localResDis=>1,
	regDev    => { burstRx=>1, intKeyVisib=>1, pairCentral=>1, 
	},

	regSwitch => { sign=>1, longPress=>1, dblPress=>1,
	               peerNeedsBurst=>1, expectAES=>1,
	},

	regRelay  => { sign=>1, 
	               shCtDlyOn=>1, shCtDlyOff=>1, shCtOn=>1, shCtOff=>1, shCtValLo=>1, shCtValHi=>1, shOnDly=>1, shOnTime=>1, shOffDly=>1, shOffTime=>1, shActionType=>1, 
	               shOffTimeMode=>1, shOnTimeMode=>1, shSwJtOn=>1, shSwJtOff=>1, shSwJtDlyOn=>1, shSwJtDlyOff=>1, 
	               lgCtDlyOn=>1, lgCtDlyOff=>1, lgCtOn=>1, lgCtOff=>1, lgCtValLo=>1, lgCtValHi=>1, lgOnDly=>1, lgOnTime=>1, lgOffDly=>1, lgOffTime=>1, lgActionType=>1, lgMultiExec=>1, 
	               lgOffTimeMode=>1, lgOnTimeMode=>1, lgSwJtOn=>1, lgSwJtOff=>1, lgSwJtDlyOn=>1, lgSwJtDlyOff=>1, 
	},


	regDimmer => { transmitTryMax=>1, ovrTempLvl=>1, redTempLvl=>1, redLvl=>1, powerUpAction=>1, statusInfoMinDly=>1, statusInfoRandom=>1, characteristic=>1, logicCombination=>1, 

		           shCtRampOn=>1, shCtRampOff=>1, shCtDlyOn=>1, shCtDlyOff=>1, shCtOn=>1, shCtOff=>1, shCtValLo=>1, shCtValHi=>1, shOnDly=>1, shOnTime=>1, shOffDly=>1, 
	               shOffTime=>1, shActionTypeDim=>1, shOffTimeMode=>1, shOnTimeMode=>1, shDimJtOn=>1, shDimJtOff=>1, shDimJtDlyOn=>1, shDimJtDlyOff=>1, shDimJtRampOn=>1, 
	               shDimJtRampOff=>1, shOffDlyBlink=>1, shOnLvlPrio=>1, shOnDlyMode=>1, shOffLevel=>1, shOnMinLevel=>1, shOnLevel=>1, shRampSstep=>1, shRampOnTime=>1, 
	               shRampOffTime=>1, shDimMinLvl=>1, shDimMaxLvl=>1, shDimStep=>1, shOffDlyNewTime=>1, shOffDlyOldTime=>1, shDimElsActionType=>1, shDimElsOffTimeMd=>1, 
	               shDimElsOnTimeMd=>1, shDimElsJtOn=>1, shDimElsJtOff=>1, shDimElsJtDlyOn=>1, shDimElsJtDlyOff=>1, shDimElsJtRampOn=>1, shDimElsJtRampOff=>1,
	               lgCtRampOn=>1, lgCtRampOff=>1, lgCtDlyOn=>1, lgCtDlyOff=>1, lgCtOn=>1, lgCtOff=>1, lgCtValLo=>1, lgCtValHi=>1, lgOnDly=>1, lgOnTime=>1, lgOffDly=>1, 
	               lgOffTime=>1, lgActionTypeDim=>1, lgMultiExec=>1, lgOffTimeMode=>1, lgOnTimeMode=>1, lgDimJtOn=>1, lgDimJtOff=>1, lgDimJtDlyOn=>1, lgDimJtDlyOff=>1, 
	               lgDimJtRampOn=>1, lgDimJtRampOff=>1, lgOffDlyBlink=>1, lgOnLvlPrio=>1, lgOnDlyMode=>1, lgOffLevel=>1, lgOnMinLevel=>1, lgOnLevel=>1, lgRampSstep=>1, 
	               lgRampOnTime=>1, lgRampOffTime=>1, lgDimMinLvl=>1, lgDimMaxLvl=>1, lgDimStep=>1, lgOffDlyNewTime=>1, lgOffDlyOldTime=>1, lgDimElsActionType=>1, 
	               lgDimElsOffTimeMd=>1, lgDimElsOnTimeMd=>1, lgDimElsJtOn=>1, lgDimElsJtOff=>1, lgDimElsJtDlyOn=>1, lgDimElsJtDlyOff=>1, lgDimElsJtRampOn=>1, lgDimElsJtRampOff=>1, 
	},

	regTHSensor => { peerNeedsBurst=>1, expectAES=>1,
	},

);

#----------------assemble device -----------------
my %regList;
$regList{0}={type => "regDev",peers=>1};

$regList{1}={type => "regDimmer",peers=>6};
#$regList{1}={type => "regDimmer",peers=>6};
#$regList{2}={type => "regDimmer",peers=>1};
#$regList{3}={type => "regDimmer",peers=>1};
#$regList{2}={type => "regSwitch",peers=>6};
#$regList{3}={type => "regSwitch",peers=>6};
#$regList{4}={type => "regSwitch",peers=>6};
#$regList{5}={type => "regSwitch",peers=>6};
#$regList{6}={type => "regSwitch",peers=>6};


sub usr_getHash($){
  my $hn = shift;
  return %regList       if($hn eq "regList"      );
  return %listTypes     if($hn eq "listTypes"       );
}
