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


#	// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, - 15 bytes
#	// 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A, 0x1B,0x1C,0x1D,0x1E, - 15 bytes
#	// 0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, - 15 bytes
#	// 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E, - 15 bytes

	regDimmer => { transmitTryMax=>1, ovrTempLvl=>1, redTempLvl=>1, redLvl=>1, powerUpAction=>1, statusInfoMinDly=>1, statusInfoRandom=>1, characteristic=>1, logicCombination=>1, 

shCtRampOff=>1, shCtRampOn=>1, shCtDlyOn=>1, shCtDlyOff=>1, shCtOn=>1, shCtOff=>1, shCtValLo=>1, shCtValHi=>1, 
shOnDly=>1, shOnTime=>1, shOffDly=>1, shOffTime=>1, 

shActionTypeDim
shOffTimeMode
shOnTimeMode


			<parameter id="SHORT_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xD.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xD.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ONDELAY_MODE">
				<logical type="option">
					<option id="SET_TO_OFF" default="true"/>
					<option id="NO_CHANGE"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xE.7" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ON_LEVEL_PRIO">
				<logical type="option">
					<option id="HIGH" default="true"/>
					<option id="LOW"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xE.6" size="0.1"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_BLINK">
				<logical type="option">
					<option id="OFF"/>
					<option id="ON" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xE.5" size="0.1"/>
			</parameter>
			<parameter id="SHORT_OFF_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0xF" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_ON_MIN_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.1" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x10" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_ON_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%">
					<special_value id="OLD_LEVEL" value="1.005"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x11" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_RAMP_START_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.05" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x12" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_RAMPON_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x13" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_RAMPOFF_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x14" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_DIM_MIN_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x15" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_DIM_MAX_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x16" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_DIM_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x17" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.05" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x18" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_NEWTIME">
				<logical type="float" min="0.1" max="25.6" default="0.5" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x19" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_OLDTIME">
				<logical type="float" min="0.1" max="25.6" default="0.5" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x1A" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			</parameter>
			<parameter id="SHORT_ELSE_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x26.7" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ELSE_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x26.6" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ELSE_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE" default="true"/>
					<option id="JUMP_TO_TARGET"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INVERS_TO_COUNTER"/>
					<option id="UPDIM"/>
					<option id="DOWNDIM"/>
					<option id="TOGGLEDIM"/>
					<option id="TOGGLEDIM_TO_COUNTER"/>
					<option id="TOGGLEDIM_INVERS_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x26.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x27.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x27.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x28.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x28.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x29.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_ELSE_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x29.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_RAMPOFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x81.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_RAMPON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x81.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_OFFDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x82.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_ONDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x82.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_OFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x83.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_ON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x83.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_COND_VALUE_LO">
				<logical type="integer" min="0" max="255" default="50"/>
				<physical type="integer" interface="config" list="3" index="0x84" size="1"/>
			</parameter>
			<parameter id="LONG_COND_VALUE_HI">
				<logical type="integer" min="0" max="255" default="100"/>
				<physical type="integer" interface="config" list="3" index="0x85" size="1"/>
			</parameter>
			<parameter id="LONG_ONDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x86" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_ON_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x87" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_OFFDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x88" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_OFF_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x89" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.7" size="0.1"/>
			</parameter>
			<parameter id="LONG_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.6" size="0.1"/>
			</parameter>
			<parameter id="LONG_MULTIEXECUTE">
				<logical type="option">
					<option id="OFF" default="true"/>
					<option id="ON"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.5" size="0.1"/>
			</parameter>
			<parameter id="LONG_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE" default="true"/>
					<option id="JUMP_TO_TARGET"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INVERS_TO_COUNTER"/>
					<option id="UPDIM"/>
					<option id="DOWNDIM"/>
					<option id="TOGGLEDIM"/>
					<option id="TOGGLEDIM_TO_COUNTER"/>
					<option id="TOGGLEDIM_INVERS_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8D.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8D.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_ONDELAY_MODE">
				<logical type="option">
					<option id="SET_TO_OFF" default="true"/>
					<option id="NO_CHANGE"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8E.7" size="0.1"/>
			</parameter>
			<parameter id="LONG_ON_LEVEL_PRIO">
				<logical type="option">
					<option id="HIGH" default="true"/>
					<option id="LOW"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8E.6" size="0.1"/>
			</parameter>
			<parameter id="LONG_OFFDELAY_BLINK">
				<logical type="option">
					<option id="OFF"/>
					<option id="ON" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8E.5" size="0.1"/>
			</parameter>
			<parameter id="LONG_OFF_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x8F" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_ON_MIN_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.1" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x90" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_ON_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%">
					<special_value id="OLD_LEVEL" value="1.005"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x91" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_RAMP_START_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.05" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x92" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_RAMPON_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x93" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_RAMPOFF_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x94" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="LONG_DIM_MIN_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x95" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_DIM_MAX_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x96" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_DIM_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x97" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_OFFDELAY_STEP">
				<logical type="float" min="0.0" max="1.0" default="0.05" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x98" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_OFFDELAY_NEWTIME">
				<logical type="float" min="0.1" max="25.6" default="0.5" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x99" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			</parameter>
			<parameter id="LONG_OFFDELAY_OLDTIME">
				<logical type="float" min="0.1" max="25.6" default="0.5" unit="s"/>
				<physical type="integer" interface="config" list="3" index="0x9A" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			</parameter>
			<parameter id="LONG_ELSE_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA6.7" size="0.1"/>
			</parameter>
			<parameter id="LONG_ELSE_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA6.6" size="0.1"/>
			</parameter>
			<parameter id="LONG_ELSE_MULTIEXECUTE">
				<logical type="option">
					<option id="OFF" default="true"/>
					<option id="ON"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA6.5" size="0.1"/>
			</parameter>
			<parameter id="LONG_ELSE_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE" default="true"/>
					<option id="JUMP_TO_TARGET"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INVERS_TO_COUNTER"/>
					<option id="UPDIM"/>
					<option id="DOWNDIM"/>
					<option id="TOGGLEDIM"/>
					<option id="TOGGLEDIM_TO_COUNTER"/>
					<option id="TOGGLEDIM_INVERS_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA6.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA7.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA7.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA8.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA8.0" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA9.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_ELSE_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY" default="true"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="RAMPOFF"/>
					<option id="OFF"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA9.0" size="0.4"/>
			</parameter>
		</paramset>
	</paramset_defs>
</device>

	},

#	regDimmer => { transmitTryMax=>1, ovrTempLvl=>1, redTempLvl=>1, redLvl=>1, powerUpAction=>1, statusInfoMinDly=>1, statusInfoRandom=>1, characteristic=>1, logicCombination=>1, 
#
#		           shCtRampOn=>1, shCtRampOff=>1, shCtDlyOn=>1, shCtDlyOff=>1, shCtOn=>1, shCtOff=>1, shCtValLo=>1, shCtValHi=>1, shOnDly=>1, shOnTime=>1, shOffDly=>1, 
#	               shOffTime=>1, shActionTypeDim=>1, shOffTimeMode=>1, shOnTimeMode=>1, shDimJtOn=>1, shDimJtOff=>1, shDimJtDlyOn=>1, shDimJtDlyOff=>1, shDimJtRampOn=>1, 
#	               shDimJtRampOff=>1, shOffDlyBlink=>1, shOnLvlPrio=>1, shOnDlyMode=>1, shOffLevel=>1, shOnMinLevel=>1, shOnLevel=>1, shRampSstep=>1, shRampOnTime=>1, 
#	               shRampOffTime=>1, shDimMinLvl=>1, shDimMaxLvl=>1, shDimStep=>1, shOffDlyNewTime=>1, shOffDlyOldTime=>1, shDimElsActionType=>1, shDimElsOffTimeMd=>1, 
#	               shDimElsOnTimeMd=>1, shDimElsJtOn=>1, shDimElsJtOff=>1, shDimElsJtDlyOn=>1, shDimElsJtDlyOff=>1, shDimElsJtRampOn=>1, shDimElsJtRampOff=>1,
#	               lgCtRampOn=>1, lgCtRampOff=>1, lgCtDlyOn=>1, lgCtDlyOff=>1, lgCtOn=>1, lgCtOff=>1, lgCtValLo=>1, lgCtValHi=>1, lgOnDly=>1, lgOnTime=>1, lgOffDly=>1, 
#	               lgOffTime=>1, lgActionTypeDim=>1, lgMultiExec=>1, lgOffTimeMode=>1, lgOnTimeMode=>1, lgDimJtOn=>1, lgDimJtOff=>1, lgDimJtDlyOn=>1, lgDimJtDlyOff=>1, 
#	               lgDimJtRampOn=>1, lgDimJtRampOff=>1, lgOffDlyBlink=>1, lgOnLvlPrio=>1, lgOnDlyMode=>1, lgOffLevel=>1, lgOnMinLevel=>1, lgOnLevel=>1, lgRampSstep=>1, 
#	               lgRampOnTime=>1, lgRampOffTime=>1, lgDimMinLvl=>1, lgDimMaxLvl=>1, lgDimStep=>1, lgOffDlyNewTime=>1, lgOffDlyOldTime=>1, lgDimElsActionType=>1, 
#	               lgDimElsOffTimeMd=>1, lgDimElsOnTimeMd=>1, lgDimElsJtOn=>1, lgDimElsJtOff=>1, lgDimElsJtDlyOn=>1, lgDimElsJtDlyOff=>1, lgDimElsJtRampOn=>1, lgDimElsJtRampOff=>1, 
#	},

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
