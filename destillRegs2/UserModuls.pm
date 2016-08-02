use strict;
use warnings;
my %user_modul;


## Master ###################################################################################
$user_modul{'Master'}{'library'} = << 'END_LINE';
#include <AS.h> 
#include "hardware.h"
#include "hmkey.h"
END_LINE

$user_modul{'Master'}{'stage'} = << 'END_LINE';
AS hm;
END_LINE

$user_modul{'Master'}{'config'} = << 'END_LINE';
hm.ld.set(welcome); 
hm.confButton.config($conf_keymode);
hm.pw.setMode($conf_powermode);
hm.bt.set($conf_lowbat_limit, $conf_lowbat_timer);
END_LINE

$user_modul{'Master'}{'header'} = << 'END_LINE';
<?xml version="1.0" encoding="iso-8859-1"?>
<?xml-stylesheet type="text/xsl" href="device.xsl"?>
END_LINE

$user_modul{'Master'}{'device'} = << 'END_LINE';
<device version="1" rx_modes="CONFIG,$rx_mode_flags" peering_sysinfo_expect_channel="false" supports_aes="true">
END_LINE

$user_modul{'Master'}{'supported_types'} = << 'END_LINE';
	<supported_types>
		<type name="$type_name" id="$type_id" updatable="true" priority="2">
			<parameter index="10.0" size="2.0" const_value="$type_const"/>
		</type>
	</supported_types>
END_LINE

$user_modul{'Master'}{'paramset_dev_master'} = << 'END_LINE';
	<paramset type="MASTER" id="maint_dev_master">
		<parameter id="INTERNAL_KEYS_VISIBLE" ui_flags="internal">
			<logical type="boolean" default="true"/>
			<physical type="integer" interface="config" list="0" index="2.7" size="0.1"/>
		</parameter>
		<parameter id="LOCAL_RESET_DISABLE">
			<logical type="boolean" default="false"/>
			<physical type="integer" interface="config" list="0" index="24" size="0.1"/>
		</parameter>
$low_bat_limit	</paramset>
END_LINE

$user_modul{'Master'}{'lowbat_limit'} = << 'END_LINE';
		<parameter id="LOW_BAT_LIMIT">
			<logical type="float" min="$lowbat_min" max="$lowbat_max" default="$lowbat_def" unit="V"/>
			<physical type="integer" interface="config" list="0" index="18" size="1"/>
			<conversion type="float_integer_scale" factor="10"/>
		</parameter>
END_LINE


$user_modul{'Master'}{'param_default'} = << 'END_LINE';
	<channels>
		<channel index="0" type="MAINTENANCE" ui_flags="internal" class="maintenance" count="1">
			<paramset type="MASTER" id="maint_ch_master">
			</paramset>
			<paramset type="VALUES" id="maint_ch_values">
				<parameter id="UNREACH" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="UNREACH"/>
				</parameter>
				<parameter id="STICKY_UNREACH" operations="read,write,event" ui_flags="service,sticky">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="STICKY_UNREACH"/>
				</parameter>
				<parameter id="CONFIG_PENDING" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="CONFIG_PENDING"/>
				</parameter>
$lowbat				<parameter id="AES_KEY" operations="read" ui_flags="invisible">
					<logical type="integer" min="0" max="127"/>
					<physical type="integer" interface="internal" value_id="AES_KEY"/>
				</parameter>
$dutycycle				<parameter id="RSSI_DEVICE" operations="read,event">
					<logical type="integer"/>
					<physical type="integer" interface="internal" value_id="RSSI_DEVICE"/>
				</parameter>
				<parameter id="RSSI_PEER" operations="read,event">
					<logical type="integer"/>
					<physical type="integer" interface="internal" value_id="RSSI_PEER"/>
				</parameter>
				<parameter id="DEVICE_IN_BOOTLOADER" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="DEVICE_IN_BOOTLOADER"/>
				</parameter>
				<parameter id="UPDATE_PENDING" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="UPDATE_PENDING"/>
				</parameter>
			</paramset>
		</channel>
END_LINE

$user_modul{'Master'}{'param_lowbat'} = << 'END_LINE';
				<parameter id="LOWBAT" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="LOWBAT"/>
				</parameter>
END_LINE

$user_modul{'Master'}{'param_dutycycle'} = << 'END_LINE';
				<parameter id="DUTYCYCLE" operations="read,event" ui_flags="service">
					<logical type="boolean"/>
					<physical type="integer" interface="internal" value_id="DUTYCYCLE"/>
				</parameter>
END_LINE

## END Master ###############################################################################

## xmlDummy
$user_modul{'xmlDummy'}{'library'} = << 'END_LINE';
#include <cmDummy.h> 
END_LINE

$user_modul{'xmlDummy'}{'stage'} = << 'END_LINE';
cmDummy cm_Dummy[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void switchDummy(uint8_t channel, uint8_t status);
END_LINE

$user_modul{'xmlDummy'}{'config'} = << 'END_LINE';
	cm_Dummy[$cm_index].regInHM($cm_reg_channel, 3, &hm);
	cm_Dummy[$cm_index].config(&initDim, &switchDim);
END_LINE

$user_modul{'xmlDummy'}{'channels'} = << 'END_LINE';
		<channel autoregister="false" index="$channel_index" type="KEY" count="$channel_count" paired="false" aes_default="false">
			<link_roles>
			</link_roles>
			<paramset>
			</paramset>
		</channel>
END_LINE

$user_modul{'xmlDummy'}{'frames'} = << 'END_LINE';
		<frame>
			<parameter/>
		</frame>
END_LINE

$user_modul{'xmlDummy'}{'paramsets'} = << 'END_LINE';
		<paramset>
			<parameter>
				<logical/>
				<physical/>
				<conversion/>
			</parameter>
		</paramset>
END_LINE

## END Dummy ##########################################################################


## xmlRemote
$user_modul{'xmlRemote'}{'library'} = << 'END_LINE';
#include <cmRemote.h> 
END_LINE

$user_modul{'xmlRemote'}{'stage'} = << 'END_LINE';
cmRemote cm_Remote[$cm_tot_index];
extern void initRemote(uint8_t channel);
END_LINE

$user_modul{'xmlRemote'}{'config'} = << 'END_LINE';
cm_Remote[$cm_index].regInHM($cm_reg_channel, 4, &hm);
cm_Remote[$cm_index].config(&initRemote);
END_LINE

$user_modul{'xmlRemote'}{'channels'} = << 'END_LINE';
		<channel autoregister="true" index="$channel_index" type="KEY" count="$channel_count" pair_function="BA" function="A" paired="true" aes_default="false">
			<link_roles>
				<source name="SWITCH"/>
				<source name="KEYMATIC"/>
				<source name="WINMATIC"/>
				<source name="REMOTECONTROL_RECEIVER"/>
			</link_roles>
			<paramset type="MASTER" id="remote_ch_master">
				<subset ref="key_paramset"/>
			</paramset>
			<paramset type="VALUES" id="remote_ch_values">
				<subset ref="key_valueset"/>
			</paramset>
			<paramset type="LINK" id="remote_ch_link">
				<subset ref="key_linkset"/>
			</paramset>
		</channel>
END_LINE

$user_modul{'xmlRemote'}{'frames'} = << 'END_LINE';
		<frame id="KEY_EVENT_SHORT" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
			<parameter type="integer" index="9.6" size="0.1" const_value="0"/>
			<parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
			<parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
		</frame>
		<frame id="KEY_EVENT_LONG" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
			<parameter type="integer" index="9.6" size="0.1" const_value="1"/>
			<parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
			<parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
		</frame>
		<frame id="KEY_EVENT_LONG_BIDI" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
			<parameter type="integer" index="1.5" size="0.1" const_value="1"/>
			<parameter type="integer" index="9.6" size="0.1" const_value="1"/>
			<parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
			<parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
		</frame>
		<frame id="KEY_SIM_SHORT" direction="from_device" type="0x40" channel_field="9:0.6">
			<parameter type="integer" index="9.6" size="0.1" const_value="0"/>
			<parameter type="integer" index="9.7" size="0.1" const_value="0"/>
			<parameter type="integer" index="10.0" size="1.0" param="SIM_COUNTER"/>
		</frame>
		<frame id="KEY_SIM_LONG" direction="from_device" type="0x40" channel_field="9:0.6">
			<parameter type="integer" index="9.6" size="0.1" const_value="1"/>
			<parameter type="integer" index="9.7" size="0.1" const_value="0"/>
			<parameter type="integer" index="10.0" size="1.0" param="SIM_COUNTER"/>
		</frame>
END_LINE

$user_modul{'xmlRemote'}{'paramsets'} = << 'END_LINE';
		<paramset id="key_paramset">
			<parameter id="LONG_PRESS_TIME">
				<logical type="float" min="0.3" max="1.8" default="0.4" unit="s"/>
				<physical type="integer" interface="config" list="1" index="4.4" size="0.4"/>
				<conversion type="float_integer_scale" factor="10" offset="-0.3"/>
			</parameter>
			<parameter id="DBL_PRESS_TIME">
				<logical type="float" min="0.0" max="1.5" default="0.0" unit="s"/>
				<physical type="integer" interface="config" list="1" index="9.0" size="0.4"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="AES_ACTIVE" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="boolean" interface="internal" value_id="AES"/>
			</parameter>
		</paramset>
		<paramset id="key_valueset">
			<parameter id="PRESS_SHORT" operations="write,event" loopback="true" control="BUTTON.SHORT" burst_suppression="0">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="COUNTER">
					<event frame="KEY_EVENT_SHORT" auth_violate_policy="reject"/>
					<set request="KEY_SIM_SHORT"/>
				</physical>
				<conversion type="action_key_counter" sim_counter="SIM_COUNTER"/>
			</parameter>
			<parameter id="PRESS_LONG" operations="write,event" loopback="true" control="BUTTON.LONG">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="COUNTER">
					<event frame="KEY_EVENT_LONG" auth_violate_policy="reject"/>
					<set request="KEY_SIM_LONG"/>
				</physical>
				<conversion type="action_key_counter" sim_counter="SIM_COUNTER"/>
			</parameter>
			<parameter id="PRESS_LONG_RELEASE" operations="event" ui_flags="internal" burst_suppression="0">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="COUNTER">
					<event frame="KEY_EVENT_LONG_BIDI" auth_violate_policy="reject"/>
				</physical>
			</parameter>
			<parameter id="PRESS_CONT" operations="event" ui_flags="internal" burst_suppression="0">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="COUNTER">
					<event frame="KEY_EVENT_LONG" auth_violate_policy="reject"/>
				</physical>
				<conversion type="action_key_same_counter" sim_counter="SIM_CONT_COUNTER"/>
			</parameter>
			<parameter id="INSTALL_TEST" operations="event" ui_flags="internal">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="TEST_COUNTER">
					<event frame="KEY_EVENT_SHORT"/>
					<event frame="KEY_EVENT_LONG"/>
				</physical>
			</parameter>
		</paramset>
		<paramset id="key_linkset">
			<parameter id="PEER_NEEDS_BURST">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="config" list="4" index="1.0" size="0.1"/>
			</parameter>
			<parameter id="EXPECT_AES">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="config" list="4" index="1.7" size="0.1"/>
			</parameter>
			<enforce id="EXPECT_AES" value="$PEER.AES"/>
		</paramset>
END_LINE

## END Remote #########################################################################


## xmlDimmer
$user_modul{'xmlDimmer'}{'library'} = << 'END_LINE';
#include <cmDimmer.h> 
END_LINE

$user_modul{'xmlDimmer'}{'stage'} = << 'END_LINE';
cmDimmer cm_Dimmer[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void initDim(uint8_t channel);
extern void switchDim(uint8_t channel, uint8_t status, uint8_t characteristic);
END_LINE

$user_modul{'xmlDimmer'}{'config'} = << 'END_LINE';
cm_Dimmer[$cm_index].regInHM($cm_reg_channel, 3, &hm);
cm_Dimmer[$cm_index].config(&initDim, &switchDim);
END_LINE

$user_modul{'xmlDimmer'}{'channels'} = << 'END_LINE';
		<channel index="$channel_index" type="DIMMER" count="$channel_count">
			<link_roles>
				<target name="SWITCH"/>
				<target name="WEATHER_CS"/>
				<target name="WCS_TIPTRONIC_SENSOR"/>
			</link_roles>
			<paramset type="MASTER" id="dimmer_ch_master">
				<subset ref="physical_parameters"/>
				<subset ref="general_parameters"/>
			</paramset>
			<paramset type="VALUES" id="dimmer_ch_values">
				<subset ref="dimmer_valueset"/>
			</paramset>
			<paramset type="LINK" id="dimmer_ch_link">
				<subset ref="dimmer_linkset"/>
			</paramset>
			<enforce_link>
				<value id="LCD_SYMBOL" value="1"/>
				<value id="LCD_LEVEL_INTERP" value="1"/>
			</enforce_link>
		</channel>
END_LINE

$user_modul{'xmlDimmer'}{'frames'} = << 'END_LINE';
		<frame id="LEVEL_SET" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
			<parameter type="integer" index="12.0" size="2.0" PARAM="RAMP_TIME"/>
			<parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
		</frame>
		<frame id="RAMP_STOP" direction="to_device" type="0x11" subtype="0x03" subtype_index="9" channel_field="10">
		</frame>
		<frame id="OLD_LEVEL" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" const_value="201"/>
			<parameter type="integer" index="12.0" size="2.0" PARAM="RAMP_TIME"/>
			<parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
		</frame>
		<frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
			<parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
		</frame>
		<frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
			<parameter type="integer" index="10.0" size="1.0" const_value="14"/>
		</frame>
		<frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
			<parameter type="integer" index="12.1" size="0.3" param="ERROR"/>
			<parameter type="integer" index="12.1" size="0.1" param="ERROR_OVERLOAD"/>
			<parameter type="integer" index="12.2" size="0.1" param="ERROR_OVERHEAT"/>
			<parameter type="integer" index="12.3" size="0.1" param="ERROR_REDUCED"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
			<parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
		</frame>
		<frame id="ACK_STATUS" direction="from_device" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
			<parameter type="integer" index="12.1" size="0.3" param="ERROR"/>
			<parameter type="integer" index="12.1" size="0.1" param="ERROR_OVERLOAD"/>
			<parameter type="integer" index="12.2" size="0.1" param="ERROR_OVERHEAT"/>
			<parameter type="integer" index="12.3" size="0.1" param="ERROR_REDUCED"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
			<parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
		</frame>
		<frame id="TOGGLE_INSTALL_TEST" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="TOGGLE_FLAG"/>
			<parameter type="integer" index="12.0" size="2.0" const_value="0"/>
		</frame>
		<frame id="INFO_POWERON" direction="from_device" allowed_receivers="CENTRAL" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
			<parameter type="integer" index="10.0" size="1.0" const_value="0"/>
			<parameter type="integer" const_value="0" param="LEVEL"/>
			<parameter type="integer" const_value="0" param="STATE_FLAGS"/>
			<parameter type="integer" const_value="0" param="INHIBIT"/>
		</frame>
END_LINE

$user_modul{'xmlDimmer'}{'paramsets'} = << 'END_LINE';
		<paramset id="physical_parameters">
			<parameter id="REDUCE_TEMP_LEVEL" operations="read,write"><logical type="integer" default="75" min="30" max="100" unit="&#176;C"/>
				<physical type="integer" interface="config" list="1" index="52" size="1"/>
			</parameter>
			<parameter id="REDUCE_LEVEL" operations="read,write"><logical type="float" min="0.0" max="1.0" default="0.4" unit="100%"/>
				<physical type="integer" interface="config" list="1" index="53" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="OVERTEMP_LEVEL" operations="read,write"><logical type="integer" default="80" min="30" max="100" unit="&#176;C"/>
				<physical type="integer" interface="config" list="1" index="50" size="1"/>
			</parameter>
			<parameter id="CHARACTERISTIC" operations="read,write"><logical type="option">
					<option id="CHARACTERISTIC_LINEAR"/>
					<option id="CHARACTERISTIC_SQUARE" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="1" index="88" size="0.1"/>
			</parameter>
		</paramset>
		<paramset id="general_parameters">
			<parameter id="TRANSMIT_TRY_MAX" operations="read,write"><logical type="integer" min="0" max="10" default="6"/>
				<physical type="integer" interface="config" list="1" index="48" size="1"/>
			</parameter>
			<parameter id="POWERUP_ACTION" operations="read,write"><logical type="option">
					<option id="POWERUP_OFF" default="true"/>
					<option id="POWERUP_ON"/>
				</logical>
				<physical type="integer" interface="config" list="1" index="86" size="0.1"/>
			</parameter>
			<parameter id="STATUSINFO_MINDELAY" operations="read,write"><logical type="float" min="0.5" max="15.5" default="2.0" unit="s">
					<special_value id="NOT_USED" value="0.0"/>
				</logical>
				<physical type="integer" interface="config" list="1" index="87.0" size="0.5"/>
				<conversion type="float_integer_scale" factor="2"/>
			</parameter>
			<parameter id="STATUSINFO_RANDOM" operations="read,write"><logical type="float" min="0.0" max="7.0" default="1.0" unit="s"/>
				<physical type="integer" interface="config" list="1" index="87.5" size="0.3"/>
				<conversion type="float_integer_scale" factor="1"/>
			</parameter>
			<parameter id="LOGIC_COMBINATION" operations="read,write"><logical type="option">
					<option id="LOGIC_INACTIVE"/>
					<option id="LOGIC_OR" default="true"/>
					<option id="LOGIC_AND"/>
					<option id="LOGIC_XOR"/>
					<option id="LOGIC_NOR"/>
					<option id="LOGIC_NAND"/>
					<option id="LOGIC_ORINVERS"/>
					<option id="LOGIC_ANDINVERS"/>
					<option id="LOGIC_PLUS"/>
					<option id="LOGIC_MINUS"/>
					<option id="LOGIC_MUL"/>
					<option id="LOGIC_PLUSINVERS"/>
					<option id="LOGIC_MINUSINVERS"/>
					<option id="LOGIC_MULINVERS"/>
					<option id="LOGIC_INVERSPLUS"/>
					<option id="LOGIC_INVERSMINUS"/>
					<option id="LOGIC_INVERSMUL"/>
				</logical>
				<physical type="integer" interface="config" list="1" index="89" size="0.5"/>
			</parameter>
		</paramset>
		<paramset id="dimmer_valueset">
			<parameter id="LEVEL" operations="read,write,event" control="DIMMER.LEVEL">
				<logical type="float" default="0.0" min="0.0" max="1.0" unit="100%"/>
				<physical type="integer" interface="command" value_id="LEVEL">
					<set request="LEVEL_SET"/>
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
					<event frame="INFO_POWERON"/>
					<reset_after_send param="RAMP_TIME"/>
					<reset_after_send param="ON_TIME"/>
				</physical>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="OLD_LEVEL" operations="write" control="DIMMER.OLD_LEVEL">
				<logical type="action"/>
				<physical type="integer" interface="command">
					<set request="OLD_LEVEL"/>
					<reset_after_send param="RAMP_TIME"/>
					<reset_after_send param="ON_TIME"/>
				</physical>
			</parameter>
			<parameter id="LEVEL_REAL" operations="read,event" control="DIMMER.LEVEL_REAL">
				<logical type="float" default="0.0" min="0.0" max="1.0" unit="100%"/>
				<physical type="integer" interface="command" value_id="LEVEL_REAL">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL_R"/>
					<event frame="ACK_STATUS_R"/>
					<event frame="INFO_POWERON"/>
					<reset_after_send param="RAMP_TIME"/>
					<reset_after_send param="ON_TIME"/>
				</physical>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="RAMP_TIME" operations="write" control="NONE">
				<logical type="float" min="0.0" max="85825945.6" default="0.5" unit="s"/>
				<physical type="integer" interface="store" id="RAMP_TIME" volatile="true"/>
				<conversion type="float_integer_scale" factor="10"/>
				<conversion type="integer_tinyfloat" mantissa_start="5" mantissa_size="11" exponent_start="0" exponent_size="5"/>
			</parameter>
			<parameter id="ON_TIME" operations="write" control="NONE">
				<logical type="float" min="0.0" max="85825945.6" default="0.0" unit="s"/>
				<physical type="integer" interface="store" id="ON_TIME" volatile="true"/>
				<conversion type="float_integer_scale" factor="10"/>
				<conversion type="integer_tinyfloat" mantissa_start="5" mantissa_size="11" exponent_start="0" exponent_size="5"/>
			</parameter>
			<parameter id="RAMP_STOP" operations="write" control="NONE">
				<logical type="action"/>
				<physical type="integer" interface="command">
					<set request="RAMP_STOP"/>
				</physical>
			</parameter>
			<parameter id="INHIBIT" operations="read,write,event" control="NONE" loopback="true">
				<logical type="boolean"/>
				<physical type="integer" interface="command" value_id="INHIBIT">
					<set request="SET_LOCK"/>
					<event frame="INFO_POWERON"/>
				</physical>
			</parameter>
			<parameter id="WORKING" operations="read,event" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="STATE_FLAGS">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
					<event frame="INFO_POWERON"/>
				</physical>
				<conversion type="boolean_integer"/>
				<conversion type="integer_integer_map">
					<value_map device_value="0x04" parameter_value="1" mask="0x04"/>
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="0"/>
				</conversion>
			</parameter>
			<parameter id="DIRECTION" operations="read,event" ui_flags="internal">
				<logical type="option">
					<option id="NONE" default="true"/>
					<option id="UP"/>
					<option id="DOWN"/>
					<option id="UNDEFINED"/>
				</logical>
				<physical type="integer" interface="command" value_id="DIRECTION_FLAGS">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
					<event frame="INFO_POWERON"/>
				</physical>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x03" parameter_value="3"/>
				</conversion>
			</parameter>
			<parameter id="ERROR_REDUCED" operations="read,event" ui_flags="service" control="NONE">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="ERROR_REDUCED">
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
				</physical>
			</parameter>
			<parameter id="ERROR_OVERHEAT" operations="read,event" ui_flags="service" control="NONE">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="ERROR_OVERHEAT">
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
				</physical>
			</parameter>
			<parameter id="INSTALL_TEST" operations="write" ui_flags="internal">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="TOGGLE_FLAG" no_init="true">
					<set request="TOGGLE_INSTALL_TEST"/>
				</physical>
				<conversion type="toggle" value="LEVEL" on="200" off="0"/>
			</parameter>
		</paramset>
		<paramset id="dimmer_linkset">
			<parameter id="UI_HINT">
				<logical type="string" default="" use_default_on_failure="true"/>
				<physical type="string" interface="store" id="UI_HINT" save_on_change="true"/>
			</parameter>
			<parameter id="SHORT_CT_RAMPOFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_RAMPON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_OFFDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ONDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_OFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_LO">
				<logical type="integer" min="0" max="255" default="50"/>
				<physical type="integer" interface="config" list="3" index="4" size="1"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_HI">
				<logical type="integer" min="0" max="255" default="100"/>
				<physical type="integer" interface="config" list="3" index="5" size="1"/>
			</parameter>
			<parameter id="SHORT_ONDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="6" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="7" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="8" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="9" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.7" size="0.1"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.6" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ACTION_TYPE">
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
				<physical type="integer" interface="config" list="3" index="0xA.0" size="0.4"/>
			</parameter>
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
END_LINE

## END Dimmer #########################################################################


## xmlSwitch
$user_modul{'xmlSwitch'}{'library'} = << 'END_LINE';
#include <cmSwitch.h> 
END_LINE

$user_modul{'xmlSwitch'}{'stage'} = << 'END_LINE';
cmSwitch cm_Switch[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void initSwitch(uint8_t channel);
extern void switchSwitch(uint8_t channel, uint8_t status);
END_LINE

$user_modul{'xmlSwitch'}{'config'} = << 'END_LINE';
cm_Switch[$cm_index].regInHM($cm_reg_channel, 3, &hm);
cm_Switch[$cm_index].config(&initSwitch, &switchSwitch);
END_LINE

$user_modul{'xmlSwitch'}{'channels'} = << 'END_LINE';
		<channel index="$channel_index" type="SWITCH" count="$channel_count">
			<link_roles>
				<target name="SWITCH"/>
				<target name="WEATHER_CS"/>
				<target name="WCS_TIPTRONIC_SENSOR"/>
			</link_roles>
			<paramset type="MASTER" id="switch_ch_master">
				<subset ref="switch_paramset"/>
			</paramset>
			<paramset type="VALUES" id="switch_ch_values">
				<subset ref="switch_valueset"/>
			</paramset>
			<paramset type="LINK" id="switch_ch_link">
				<subset ref="switch_linkset"/>
			</paramset>
			<enforce_link>
				<value id="LCD_SYMBOL" value="2"/>
				<value id="LCD_LEVEL_INTERP" value="1"/>
				<value id="PEER_NEEDS_BURST" value="true"/>
			</enforce_link>
		</channel>
END_LINE

$user_modul{'xmlSwitch'}{'frames'} = << 'END_LINE';
		<frame id="LEVEL_SET" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="STATE"/>
			<parameter type="integer" index="12.0" size="2.0" const_value="0"/>
			<parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
		</frame>

		<frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
			<parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
		</frame>

		<frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
			<parameter type="integer" index="10.0" size="1.0" const_value="14"/>
		</frame>

		<frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="STATE"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
		</frame>

		<frame id="ACK_STATUS" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="STATE"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
		</frame>

		<frame id="TOGGLE_INSTALL_TEST" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="TOGGLE_FLAG"/>
			<parameter type="integer" index="12.0" size="2.0" const_value="0"/>
		</frame>

		<frame id="INFO_POWERON" direction="from_device" allowed_receivers="CENTRAL" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
			<parameter type="integer" index="10.0" size="1.0" const_value="0"/>
			<parameter type="integer" const_value="0" param="STATE"/>
			<parameter type="integer" const_value="0" param="STATE_FLAGS"/>
			<parameter type="integer" const_value="0" param="INHIBIT"/>
		</frame>
END_LINE

$user_modul{'xmlSwitch'}{'paramsets'} = << 'END_LINE';
		<paramset id="switch_valueset">
			<parameter id="STATE" operations="read,write,event" control="SWITCH.STATE">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="STATE">
					<set request="LEVEL_SET"/>
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL" auth_violate_policy="get"/>
					<event frame="ACK_STATUS" auth_violate_policy="get"/>
					<event frame="INFO_POWERON" auth_violate_policy="get"/>
					<reset_after_send param="ON_TIME"/>
				</physical>
				<conversion type="boolean_integer" threshold="1" false="0" true="200"/>
			</parameter>
			<parameter id="ON_TIME" operations="write" control="NONE">
				<logical type="float" min="0.0" max="85825945.6" default="0.0" unit="s"/>
				<physical type="integer" interface="store" id="ON_TIME" volatile="true"/>
				<conversion type="float_integer_scale" factor="10"/>
				<conversion type="integer_tinyfloat" mantissa_start="5" mantissa_size="11" exponent_start="0" exponent_size="5"/>
			</parameter>
			<parameter id="INHIBIT" operations="read,write,event" control="NONE" loopback="true">
				<logical type="boolean"/>
				<physical type="integer" interface="command" value_id="INHIBIT">
					<set request="SET_LOCK"/>
				</physical>
			</parameter>
			<parameter id="WORKING" operations="read,event" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="STATE_FLAGS">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
					<event frame="INFO_POWERON"/>
				</physical>
				<conversion type="boolean_integer"/>
			</parameter>
			<parameter id="INSTALL_TEST" operations="write" ui_flags="internal">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="TOGGLE_FLAG" no_init="true">
					<set request="TOGGLE_INSTALL_TEST"/>
				</physical>
				<conversion type="toggle" value="STATE"/>
			</parameter>
		</paramset>
		<paramset id="switch_paramset">
			<parameter id="AES_ACTIVE" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="boolean" interface="internal" value_id="AES"/>
			</parameter>
		</paramset>
		<paramset id="switch_linkset">
			<parameter id="UI_HINT">
				<logical type="string" default="" use_default_on_failure="true"/>
				<physical type="string" interface="store" id="UI_HINT" save_on_change="true"/>
			</parameter>
			<parameter id="SHORT_CT_OFFDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ONDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_OFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_LO">
				<logical type="integer" min="0" max="255" default="50"/>
				<physical type="integer" interface="config" list="3" index="4" size="1"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_HI">
				<logical type="integer" min="0" max="255" default="100"/>
				<physical type="integer" interface="config" list="3" index="5" size="1"/>
			</parameter>
			<parameter id="SHORT_ONDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="6" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="7" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="8" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="9" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.7" size="0.1"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.6" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE"/>
					<option id="JUMP_TO_TARGET" default="true"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INV_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.0" size="0.2"/>
			</parameter>
			<parameter id="SHORT_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
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
					<option id="OFF"/>
					<option id="ON" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.5" size="0.1"/>
			</parameter>
			<parameter id="LONG_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE"/>
					<option id="JUMP_TO_TARGET" default="true"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INV_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.0" size="0.2"/>
			</parameter>
			<parameter id="LONG_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="2"/>
					<value_map device_value="0x04" parameter_value="3"/>
					<value_map device_value="0x06" parameter_value="4"/>
				</conversion>
			</parameter>
			<default_values function="A">
				<value id="LONG_JT_OFF" value="ONDELAY"/>
				<value id="LONG_JT_OFFDELAY" value="ON"/>
				<value id="LONG_JT_ON" value="ON"/>
				<value id="LONG_JT_ONDELAY" value="ON"/>
				<value id="SHORT_JT_OFF" value="ONDELAY"/>
				<value id="SHORT_JT_OFFDELAY" value="ON"/>
				<value id="SHORT_JT_ON" value="ON"/>
				<value id="SHORT_JT_ONDELAY" value="ON"/>
			</default_values>
			<default_values function="B">
				<value id="LONG_JT_ON" value="OFFDELAY"/>
				<value id="SHORT_JT_ON" value="OFFDELAY"/>
			</default_values>
			<default_values function="AB">
				<value id="LONG_JT_OFF" value="ONDELAY"/>
				<value id="LONG_JT_ON" value="OFFDELAY"/>
				<value id="LONG_JT_ONDELAY" value="ON"/>
				<value id="SHORT_JT_OFF" value="ONDELAY"/>
				<value id="SHORT_JT_ON" value="OFFDELAY"/>
				<value id="SHORT_JT_ONDELAY" value="ON"/>
			</default_values>
		</paramset>
END_LINE

## END Switch #########################################################################


## xmlBlind
$user_modul{'xmlBlind'}{'library'} = << 'END_LINE';
#include <cmBlind.h>
END_LINE

$user_modul{'xmlBlind'}{'stage'} = << 'END_LINE';
cmBlind cm_Blind[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void initBlind(uint8_t channel);
extern void switchBlind(uint8_t channel, uint8_t status);
END_LINE

$user_modul{'xmlBlind'}{'config'} = << 'END_LINE';
cm_Blind[$cm_index].regInHM($cm_reg_channel, 3, &hm);
cm_Blind[$cm_index].config(&initBlind, &switchBlind);
END_LINE

$user_modul{'xmlBlind'}{'channels'} = << 'END_LINE';
		<channel index="$channel_index" type="BLIND" count="$channel_count"">
			<link_roles>
				<target name="SWITCH"/>
				<target name="WEATHER_CS"/>
			</link_roles>
			<paramset type="MASTER" id="blind_ch_master">
				<subset ref="blind_paramset"/>
			</paramset>
			<paramset type="VALUES" id="blind_ch_values">
				<subset ref="blind_valueset"/>
			</paramset>
			<paramset type="LINK" id="blind_ch_link">
				<subset ref="blind_linkset"/>
			</paramset>
			<enforce_link>
				<value id="LCD_SYMBOL" value="2"/>
				<value id="LCD_LEVEL_INTERP" value="1"/>
				<value id="PEER_NEEDS_BURST" value="true"/>
			</enforce_link>
		</channel>
END_LINE

$user_modul{'xmlBlind'}{'frames'} = << 'END_LINE';
		<frame id="LEVEL_SET" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
		</frame>
		<frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
			<parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
		</frame>
		<frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
			<parameter type="integer" index="10.0" size="1.0" const_value="14"/>
		</frame>
		<frame id="INFO_LEVEL" direction="from_device" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
			<parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
		</frame>
		<frame id="ACK_STATUS" direction="from_device" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
			<parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
			<parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
		</frame>
		<frame id="STOP" direction="to_device" type="0x11" subtype="0x03" subtype_index="9" channel_field="10">
		</frame>
		<frame id="INSTALL_TEST" direction="to_device" type="0x11" channel_field="10">
			<parameter type="integer" index="9.0" size="1.0" param="IT_COMMAND"/>
			<parameter type="integer" index="11.0" size="1.0" param="IT_LEVEL" omit_if="255"/>
		</frame>
END_LINE

$user_modul{'xmlBlind'}{'paramsets'} = << 'END_LINE';
		<paramset id="blind_valueset">
			<parameter id="LEVEL" operations="read,write,event" control="BLIND.LEVEL">
				<logical type="float" default="0.0" min="0.0" max="1.0" unit="100%"/>
				<physical type="integer" interface="command" value_id="LEVEL">
					<set request="LEVEL_SET"/>
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL" auth_violate_policy="get"/>
					<event frame="ACK_STATUS" auth_violate_policy="get"/>
				</physical>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="STOP" operations="write" control="BLIND.STOP">
				<logical type="action"/>
				<physical type="integer" interface="command">
					<set request="STOP" process_as_event="true"/>
				</physical>
			</parameter>
			<parameter id="INHIBIT" operations="read,write,event" control="NONE" loopback="true">
				<logical type="boolean"/>
				<physical type="integer" interface="command" value_id="INHIBIT">
					<set request="SET_LOCK"/>
				</physical>
			</parameter>
			<parameter id="WORKING" operations="read,event" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="command" value_id="STATE_FLAGS">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
				</physical>
				<conversion type="boolean_integer"/>
				<conversion type="integer_integer_map">
					<value_map device_value="0x04" parameter_value="1" mask="0x04"/>
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="1"/>
					<value_map device_value="0x03" parameter_value="0"/>
				</conversion>
			</parameter>
			<parameter id="DIRECTION" operations="read,event" ui_flags="internal">
				<logical type="option">
					<option id="NONE" default="true"/>
					<option id="UP"/>
					<option id="DOWN"/>
					<option id="UNDEFINED"/>
				</logical>
				<physical type="integer" interface="command" value_id="DIRECTION_FLAGS">
					<get request="LEVEL_GET" response="INFO_LEVEL" process_as_event="true"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
				</physical>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x03" parameter_value="3"/>
				</conversion>
			</parameter>
			<parameter id="INSTALL_TEST" operations="write" ui_flags="internal">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="IT_LEVEL" no_init="true">
					<set request="INSTALL_TEST"/>
				</physical>
				<conversion type="blind_test" value="255"/>
			</parameter>
		</paramset>
		<paramset id="blind_paramset">
			<parameter id="AES_ACTIVE" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="boolean" interface="internal" value_id="AES"/>
			</parameter>
			<parameter id="REFERENCE_RUNNING_TIME_TOP_BOTTOM">
				<logical type="float" min="0.1" max="6000.0" default="50.0" unit="s"/>
				<physical type="integer" interface="config" list="1" index="11" size="2"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="REFERENCE_RUNNING_TIME_BOTTOM_TOP">
				<logical type="float" min="0.1" max="6000.0" default="50.0" unit="s"/>
				<physical type="integer" interface="config" list="1" index="13" size="2"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="CHANGE_OVER_DELAY">
				<logical type="float" min="0.5" max="25.5" default="0.5" unit="s"/>
				<physical type="integer" interface="config" list="1" index="15" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="REFERENCE_RUN_COUNTER">
				<logical type="integer" min="0" max="255" default="0"/>
				<physical type="integer" interface="config" list="1" index="16" size="1"/>
			</parameter>
		</paramset>
		<paramset id="blind_linkset">
			<parameter id="UI_HINT">
				<logical type="string" default="" use_default_on_failure="true"/>
				<physical type="string" interface="store" id="UI_HINT" save_on_change="true"/>
			</parameter>
			<parameter id="SHORT_CT_RAMPOFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_RAMPON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_OFFDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ONDELAY">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x2.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_OFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_ON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x3.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_REFOFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1C.4" size="0.4"/>
			</parameter>
			<parameter id="SHORT_CT_REFON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1C.0" size="0.4"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_LO">
				<logical type="integer" min="0" max="255" default="50"/>
				<physical type="integer" interface="config" list="3" index="4" size="1"/>
			</parameter>
			<parameter id="SHORT_COND_VALUE_HI">
				<logical type="integer" min="0" max="255" default="100"/>
				<physical type="integer" interface="config" list="3" index="5" size="1"/>
			</parameter>
			<parameter id="SHORT_ONDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="6" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="7" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFFDELAY_TIME">
				<logical type="float" min="0.0" max="111600.0" default="0" unit="s"/>
				<physical type="integer" interface="config" list="3" index="8" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME">
				<logical type="float" min="0.0" max="108000.0" default="111600.0" unit="s">
					<special_value id="NOT_USED" value="111600.0"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="9" size="1"/>
				<conversion type="float_configtime"/>
			</parameter>
			<parameter id="SHORT_ON_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.7" size="0.1"/>
			</parameter>
			<parameter id="SHORT_OFF_TIME_MODE">
				<logical type="option">
					<option id="ABSOLUTE" default="true"/>
					<option id="MINIMAL"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.6" size="0.1"/>
			</parameter>
			<parameter id="SHORT_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE"/>
					<option id="JUMP_TO_TARGET" default="true"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INV_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xA.0" size="0.2"/>
			</parameter>
			<parameter id="SHORT_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xB.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xC.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xD.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0xD.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_REFOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1E.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_JT_REFON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1E.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="SHORT_OFF_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0xF" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_ON_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x11" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="SHORT_MAX_TIME_FIRST_DIR">
				<logical type="float" min="0.0" max="25.4" default="25.5" unit="s">
					<special_value id="NOT_USED" value="25.5"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1D" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="SHORT_DRIVING_MODE">
				<logical type="option">
					<option id="DRIVE_DIRECTLY" default="true"/>
					<option id="DRIVE_VIA_UPPER_END_POSITION"/>
					<option id="DRIVE_VIA_LOWER_END_POSITION"/>
					<option id="DRIVE_VIA_NEXT_END_POSITION"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x1F" size="1.0"/>
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
			<parameter id="LONG_CT_REFOFF">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9C.4" size="0.4"/>
			</parameter>
			<parameter id="LONG_CT_REFON">
				<logical type="option">
					<option id="X GE COND_VALUE_LO" default="true"/>
					<option id="X GE COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO"/>
					<option id="X LT COND_VALUE_HI"/>
					<option id="COND_VALUE_LO LE X LT COND_VALUE_HI"/>
					<option id="X LT COND_VALUE_LO OR X GE COND_VALUE_HI"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9C.0" size="0.4"/>
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
					<option id="OFF"/>
					<option id="ON" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.5" size="0.1"/>
			</parameter>
			<parameter id="LONG_ACTION_TYPE">
				<logical type="option">
					<option id="INACTIVE"/>
					<option id="JUMP_TO_TARGET" default="true"/>
					<option id="TOGGLE_TO_COUNTER"/>
					<option id="TOGGLE_INV_TO_COUNTER"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8A.0" size="0.2"/>
			</parameter>
			<parameter id="LONG_JT_OFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_ON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8B.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_OFFDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_ONDELAY">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8C.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_RAMPOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8D.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_RAMPON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x8D.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_REFOFF">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9E.4" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_JT_REFON">
				<logical type="option">
					<option id="NO_JUMP_IGNORE_COMMAND"/>
					<option id="ONDELAY"/>
					<option id="REFON"/>
					<option id="RAMPON"/>
					<option id="ON"/>
					<option id="OFFDELAY"/>
					<option id="REFOFF"/>
					<option id="RAMPOFF"/>
					<option id="OFF" default="true"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9E.0" size="0x0.4"/>
				<conversion type="option_integer">
					<value_map device_value="0x00" parameter_value="0"/>
					<value_map device_value="0x01" parameter_value="1"/>
					<value_map device_value="0x02" parameter_value="2"/>
					<value_map device_value="0x08" parameter_value="3"/>
					<value_map device_value="0x03" parameter_value="4"/>
					<value_map device_value="0x04" parameter_value="5"/>
					<value_map device_value="0x05" parameter_value="6"/>
					<value_map device_value="0x09" parameter_value="7"/>
					<value_map device_value="0x06" parameter_value="8"/>
				</conversion>
			</parameter>
			<parameter id="LONG_OFF_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="0.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x8F" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_ON_LEVEL">
				<logical type="float" min="0.0" max="1.0" default="1.0" unit="100%"/>
				<physical type="integer" interface="config" list="3" index="0x91" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
			<parameter id="LONG_MAX_TIME_FIRST_DIR">
				<logical type="float" min="0.0" max="25.4" default="0.5" unit="s">
					<special_value id="NOT_USED" value="25.5"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9D" size="1"/>
				<conversion type="float_integer_scale" factor="10" offset="0.0"/>
			</parameter>
			<parameter id="LONG_DRIVING_MODE">
				<logical type="option">
					<option id="DRIVE_DIRECTLY" default="true"/>
					<option id="DRIVE_VIA_UPPER_END_POSITION"/>
					<option id="DRIVE_VIA_LOWER_END_POSITION"/>
					<option id="DRIVE_VIA_NEXT_END_POSITION"/>
				</logical>
				<physical type="integer" interface="config" list="3" index="0x9F" size="1.0"/>
			</parameter>
			<default_values function="A">
				<value id="LONG_JT_OFF" value="ONDELAY"/>
				<value id="LONG_JT_OFFDELAY" value="ONDELAY"/>
				<value id="LONG_JT_ON" value="ONDELAY"/>
				<value id="LONG_JT_ONDELAY" value="REFON"/>
				<value id="LONG_JT_RAMPON" value="RAMPON"/>
				<value id="LONG_JT_REFON" value="RAMPON"/>
				<value id="SHORT_JT_OFF" value="ONDELAY"/>
				<value id="SHORT_JT_OFFDELAY" value="ONDELAY"/>
				<value id="SHORT_JT_ON" value="ONDELAY"/>
				<value id="SHORT_JT_ONDELAY" value="REFON"/>
				<value id="SHORT_JT_RAMPON" value="RAMPON"/>
				<value id="SHORT_JT_REFON" value="RAMPON"/>
			</default_values>
			<default_values function="B">
				<value id="LONG_JT_OFF" value="OFFDELAY"/>
				<value id="LONG_JT_OFFDELAY" value="REFOFF"/>
				<value id="LONG_JT_ON" value="OFFDELAY"/>
				<value id="LONG_JT_ONDELAY" value="OFFDELAY"/>
				<value id="LONG_JT_RAMPOFF" value="RAMPOFF"/>
				<value id="LONG_JT_RAMPON" value="ON"/>
				<value id="LONG_JT_REFOFF" value="RAMPOFF"/>
				<value id="LONG_JT_REFON" value="ON"/>
				<value id="SHORT_JT_OFF" value="OFFDELAY"/>
				<value id="SHORT_JT_OFFDELAY" value="REFOFF"/>
				<value id="SHORT_JT_ON" value="OFFDELAY"/>
				<value id="SHORT_JT_ONDELAY" value="OFFDELAY"/>
				<value id="SHORT_JT_RAMPOFF" value="RAMPOFF"/>
				<value id="SHORT_JT_RAMPON" value="ON"/>
				<value id="SHORT_JT_REFOFF" value="RAMPOFF"/>
				<value id="SHORT_JT_REFON" value="ON"/>
			</default_values>
			<default_values function="AB">
				<value id="LONG_JT_OFF" value="ONDELAY"/>
				<value id="LONG_JT_OFFDELAY" value="REFOFF"/>
				<value id="LONG_JT_ON" value="OFFDELAY"/>
				<value id="LONG_JT_ONDELAY" value="REFON"/>
				<value id="LONG_JT_RAMPON" value="ON"/>
				<value id="LONG_JT_REFON" value="ON"/>
				<value id="SHORT_JT_OFF" value="ONDELAY"/>
				<value id="SHORT_JT_OFFDELAY" value="REFOFF"/>
				<value id="SHORT_JT_ON" value="OFFDELAY"/>
				<value id="SHORT_JT_ONDELAY" value="REFON"/>
				<value id="SHORT_JT_RAMPON" value="ON"/>
				<value id="SHORT_JT_REFON" value="ON"/>
			</default_values>
		</paramset>
END_LINE

## END Blind ##########################################################################


## xmlMotion
$user_modul{'xmlMotion'}{'library'} = << 'END_LINE';
#include <cmMotion.h>
END_LINE

$user_modul{'xmlMotion'}{'stage'} = << 'END_LINE';
cmMotion cm_Motion[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void initMotion(uint8_t channel);
END_LINE

$user_modul{'xmlMotion'}{'config'} = << 'END_LINE';
cm_Motion[$cm_index].regInHM($cm_reg_channel, 4, &hm);
cm_Motion[$cm_index].config(&initMotion);
END_LINE

$user_modul{'xmlMotion'}{'channels'} = << 'END_LINE';
		<channel autoregister="true" index="$channel_index" type="MOTION_DETECTOR" count="$channel_count" aes_default="false">
			<link_roles>
				<source name="SWITCH"/>
				<source name="KEYMATIC"/>
				<source name="WINMATIC"/>
			</link_roles>
			<paramset type="MASTER" id="md_ch_master">
				<subset ref="md_paramset"/>
			</paramset>
			<paramset type="VALUES" id="md_ch_values">
				<subset ref="md_valueset"/>
			</paramset>
			<paramset type="LINK" id="md_ch_link">
				<subset ref="md_linkset"/>
			</paramset>
			<enforce_link>
				<value id="SHORT_CT_RAMPOFF" value="2"/>
				<value id="SHORT_CT_RAMPON" value="2"/>
				<value id="SHORT_CT_OFFDELAY" value="2"/>
				<value id="SHORT_CT_ONDELAY" value="2"/>
				<value id="SHORT_CT_OFF" value="2"/>
				<value id="SHORT_CT_ON" value="2"/>
				<value id="SHORT_COND_VALUE_LO" value="255"/>
				<value id="SHORT_ONDELAY_TIME" value="0.0"/>
				<value id="SHORT_ON_TIME" value="300.0"/>
				<value id="SHORT_OFFDELAY_TIME" value="20.0"/>
				<value id="SHORT_OFF_TIME" value="111600.0"/>
				<value id="SHORT_ON_TIME_MODE" value="1"/>
				<value id="SHORT_ACTION_TYPE" value="1"/>
				<value id="SHORT_JT_OFF" value="1"/>
				<value id="SHORT_JT_ON" value="2"/>
				<value id="SHORT_JT_OFFDELAY" value="2"/>
				<value id="SHORT_JT_ONDELAY" value="2"/>
				<value id="SHORT_JT_RAMPOFF" value="2"/>
				<value id="SHORT_JT_RAMPON" value="0"/>
				<value id="SHORT_ON_LEVEL_PRIO" value="1"/>
				<value id="SHORT_OFFDELAY_BLINK" value="1"/>
				<value id="SHORT_ON_MIN_LEVEL" value="10"/>
				<value id="SHORT_ON_LEVEL" value="100"/>
				<value id="SHORT_RAMP_START_STEP" value="5"/>
				<value id="SHORT_RAMPON_TIME" value="0.5"/>
				<value id="SHORT_RAMPOFF_TIME" value="0.5"/>
				<value id="SHORT_DIM_MIN_LEVEL" value="0"/>
				<value id="SHORT_DIM_MAX_LEVEL" value="100"/>
				<value id="SHORT_DIM_STEP" value="5"/>
				<value id="SHORT_OFFDELAY_STEP" value="5"/>
				<value id="SHORT_OFFDELAY_NEWTIME" value="0.5"/>
				<value id="SHORT_OFFDELAY_OLDTIME" value="0.5"/>
				<value id="LONG_ACTION_TYPE" value="0"/>
			</enforce_link>
		</channel>
END_LINE

$user_modul{'xmlMotion'}{'frames'} = << 'END_LINE';
		<frame id="EVENT" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x41" channel_field="9:0.6">
			<parameter type="integer" param="MOTION" const_value="1"/>
			<parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
			<parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
			<parameter type="integer" index="11.0" size="1.0" param="BRIGHTNESS"/>
			<parameter type="integer" index="12.4" size="0.4" param="NEXT_TRANSMISSION"/>
		</frame>
		<frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="BRIGHTNESS"/>
		</frame>
		<frame id="ACK_STATUS" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
			<parameter type="integer" index="11.0" size="1.0" param="BRIGHTNESS"/>
		</frame>
		<frame id="ENTER_BOOTLOADER" direction="to_device" type="0x11" subtype="0xCA" subtype_index="9"/>
END_LINE

$user_modul{'xmlMotion'}{'paramsets'} = << 'END_LINE';
		<paramset id="md_paramset">
			<parameter id="TRANSMIT_TRY_MAX">
				<logical type="integer" min="1.0" max="10.0" default="3.0"/>
				<physical type="integer" interface="config" list="1" index="48" size="1"/>
			</parameter>
			<parameter id="EVENT_FILTER_PERIOD">
				<logical type="float" min="0.5" max="7.5" unit="s" default="0.5"/>
				<physical type="integer" interface="config" list="1" index="1.0" size="0.4"/>
				<conversion type="float_integer_scale" factor="2"/>
			</parameter>
			<parameter id="EVENT_FILTER_NUMBER">
				<logical type="integer" min="1" max="15" default="1"/>
				<physical type="integer" interface="config" list="1" index="1.4" size="0.4"/>
			</parameter>
			<parameter id="CAPTURE_WITHIN_INTERVAL">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="config" list="1" index="2.3" size="0.1"/>
			</parameter>
			<parameter id="MIN_INTERVAL">
				<logical type="integer" min="0" max="4" default="4"/>
				<physical type="integer" interface="config" list="1" index="2.0" size="0.3"/>
			</parameter>
			<parameter id="BRIGHTNESS_FILTER">
				<logical type="integer" min="0" max="7" default="7"/>
				<physical type="integer" interface="config" list="1" index="2.4" size="0.4"/>
			</parameter>
			<parameter id="AES_ACTIVE" ui_flags="internal">
				<logical type="boolean" default="false"/>
				<physical type="boolean" interface="internal" value_id="AES"/>
			</parameter>
			<parameter id="LED_ONTIME">
				<logical type="float" min="0.0" max="1.275" unit="s" default="0.5"/>
				<physical type="integer" interface="config" list="1" index="34" size="1"/>
				<conversion type="float_integer_scale" factor="200"/>
			</parameter>
		</paramset>
		<paramset id="md_valueset">
			<parameter id="BRIGHTNESS" operations="read,event">
				<logical type="integer" min="0" max="255"/>
				<physical type="integer" interface="command" value_id="BRIGHTNESS" no_init="true">
					<event frame="EVENT"/>
					<event frame="INFO_LEVEL"/>
					<event frame="ACK_STATUS"/>
				</physical>
			</parameter>
			<parameter id="NEXT_TRANSMISSION" hidden="true">
				<logical type="integer" default="0"/>
				<physical type="integer" interface="command" value_id="NEXT_TRANSMISSION">
					<event frame="EVENT"/>
				</physical>
				<conversion type="integer_integer_scale" mul="100" div="110"/>
				<conversion type="integer_tinyfloat" mantissa_size="0" exponent_size="4"/>
			</parameter>
			<parameter id="MOTION" operations="read,event">
				<logical type="boolean"/>
				<physical type="integer" interface="command" value_id="MOTION">
					<event frame="EVENT">
						<domino_event value="0" delay_id="NEXT_TRANSMISSION"/>
					</event>
				</physical>
			</parameter>
			<parameter id="INSTALL_TEST" operations="event" ui_flags="internal">
				<logical type="action"/>
				<physical type="integer" interface="command" value_id="TEST_COUNTER">
					<event frame="EVENT"/>
				</physical>
			</parameter>
		</paramset>
		<paramset id="md_linkset">
			<parameter id="PEER_NEEDS_BURST">
				<logical type="boolean" default="false"/>
				<physical type="integer" interface="config" list="4" index="1.0" size="0.1"/>
			</parameter>
		</paramset>
END_LINE

## END Motion ##########################################################################


## xmlWeather
$user_modul{'xmlWeather'}{'library'} = << 'END_LINE';
#include <cmWeather.h>
END_LINE

$user_modul{'xmlWeather'}{'stage'} = << 'END_LINE';
cmWeather cm_Weather[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
extern void initWeather(uint8_t channel);                                   // declare function to jump in
extern void readWeather(uint8_t channel);                                   // declare function to jump in
END_LINE

$user_modul{'xmlWeather'}{'config'} = << 'END_LINE';
cm_Weather[$cm_index].regInHM($cm_reg_channel, 3, &hm);
cm_Weather[$cm_index].config(&initWeather, &readWeather);
END_LINE

$user_modul{'xmlWeather'}{'channels'} = << 'END_LINE';
		<channel index="$channel_index" type="WEATHER" count="$channel_count">
			<paramset type="MASTER" id="wds30ot2_ch_master">
			</paramset>
			<paramset type="VALUES" id="wds30ot2_ch_values">
				<subset ref="weather_valueset"/>
			</paramset>
		</channel>
END_LINE

$user_modul{'xmlWeather'}{'frames'} = << 'END_LINE';
		<frame id="WEATHER_EVENT" direction="from_device" event="true" fixed_channel="5" type="0x70">
			<parameter type="integer" signed="true" index="9.0" size="1.7" param="TEMPERATURE"/>
		</frame>
		<frame id="MEASURE_EVENT" direction="from_device" event="true" type="0x53" channel_field="10.0:0.6">
			<parameter type="integer" signed="true" index="11.0" size="2.0" param="TEMPERATURE"/>
		</frame>
		<frame id="MEASURE_EVENT" direction="from_device" event="true" type="0x53" channel_field="13.0:0.6">
			<parameter type="integer" signed="true" index="14.0" size="2.0" param="TEMPERATURE"/>
		</frame>
		<frame id="MEASURE_EVENT" direction="from_device" event="true" type="0x53" channel_field="16.0:0.6">
			<parameter type="integer" signed="true" index="17.0" size="2.0" param="TEMPERATURE"/>
		</frame>
		<frame id="MEASURE_EVENT" direction="from_device" event="true" type="0x53" channel_field="19.0:0.6">
			<parameter type="integer" signed="true" index="20.0" size="2.0" param="TEMPERATURE"/>
		</frame>
		<frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
			<parameter type="integer" index="12.7" size="0.1" param="LOWBAT"/>
		</frame>
		<frame id="WEATHER_EV_BAT" direction="from_device" event="true" fixed_channel="*" type="0x70">
			<parameter type="integer" index="9.7" size="0.1" param="LOWBAT"/>
		</frame>
		<frame id="MEASURE_EV_BAT" direction="from_device" event="true" type="0x53" fixed_channel="*">
			<parameter type="integer" index="9.7" size="0.1" param="LOWBAT"/>
		</frame>
END_LINE

$user_modul{'xmlWeather'}{'paramsets'} = << 'END_LINE';
		<paramset id="weather_valueset">
			<parameter id="TEMPERATURE" operations="read,event">
				<logical type="float" min="-150.0" max="150.0" unit="&#176;C"/>
				<physical type="integer" interface="command" value_id="TEMPERATURE" no_init="true">
					<event frame="MEASURE_EVENT"/>
				</physical>
				<conversion type="float_integer_scale" factor="10.0"/>
				<description>
					<field id="AutoconfRoles" value="WEATHER"/>
				</description>
			</parameter>
			<parameter id="LOWBAT" operations="read,event" control="NONE">
				<logical type="boolean"/>
				<physical type="integer" interface="command" value_id="LOWBAT">
					<event frame="WEATHER_EV_BAT"/>
					<event frame="MEASURE_EV_BAT"/>
					<event frame="INFO_LEVEL"/>
				</physical>
			</parameter>
		</paramset>
END_LINE

## END Weather ##########################################################################

	
### Needed for new channels ###########################################################
## xmlDummy
#$user_modul{'xmlDummy'}{'library'} = << 'END_LINE';
#include <cmDummy.h>
#END_LINE

#$user_modul{'xmlDummy'}{'stage'} = << 'END_LINE';
#cmDummy cm_Dummy[$cm_tot_index];												    // create instances of channel module	extern void initDim(uint8_t channel);
#extern void initDummy(uint8_t channel);                                   // declare function to jump in
#extern void readDummy(uint8_t channel);                                   // declare function to jump in
#END_LINE

#$user_modul{'xmlDummy'}{'config'} = << 'END_LINE';
#cm_Dummy[$cm_index].regInHM($cm_reg_channel, 3, &hm);
#cm_Dummy[$cm_index].config(&initDummy, &readDummy);
#END_LINE

#$user_modul{'xmlDummy'}{'channels'} = << 'END_LINE';
#		<channel autoregister="false" index="$channel_index" type="KEY" count="$channel_count" paired="false" aes_default="false">
#END_LINE

#$user_modul{'xmlDummy'}{'frames'} = << 'END_LINE';
#		<frame>
#			<parameter/>
#		</frame>
#END_LINE

#$user_modul{'xmlDummy'}{'paramsets'} = << 'END_LINE';
#		<paramset>
#			<parameter>
#				<logical/>
#				<physical/>
#				<conversion/>
#			</parameter>
#		</paramset>
#END_LINE

## END Dummy ##########################################################################
#######################################################################################



## -- helpers -----------------------------------------------------------------------------------------------
package UserModuls;

sub get {
  my $type = shift;
  return $user_modul{$type};
}


