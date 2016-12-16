
#ifndef _REGISTER_h
	#define _REGISTER_h
	
	/**
	 * @brief Libraries needed to run AskSin library
	 */
	#include <AS.h> 
	#include "hardware.h"
	#include "hmkey.h"
	#include <cmMaintenance.h> 
	#include <cmDimmer.h> 

	/**
	 * @brief Stage the modules and declare external functions.
	 *
	 * This functions are the call backs for user modules,
	 * declaration is in the register.h but the functions needs
	 * to be defined in the user sketch.
	 */
	AS hm;
	AES *aes = new NO_AES;

	/*
	* cmSwitch requires this functions in the user sketch:
	* void cmSwitch::initSwitch(uint8_t channel);
	* void cmSwitch::switchSwitch(uint8_t channel, uint8_t status);
	*/
	const uint8_t cmMaintenance_ChnlReg[] PROGMEM = { 0x02,0x05,0x08,0x0a,0x0b,0x0c,0x12, };
	const uint8_t cmMaintenance_ChnlDef[] PROGMEM = { 0x80,0x00,0x10,0x00,0x00,0x00,0x69, };
	const uint8_t cmMaintenance_ChnlLen = 7;

	cmMaster *ptr_CM[4] = {
		new cmMaintenance(0),
		new cmDimmer(10),
		new cmDimmer(2),
		new cmDimmer(2),
	};


	/*
	 * @brief HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x33,0x11,0x23,
		/* Serial number */   'H','B','d','i','m','m','e','r','0','1',		// HBswitch01 
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX,
		/* Default-Key */     HM_DEVICE_AES_KEY,
	};


	/**
	 * @brief Settings of HM device
	 * firmwareVersion: The firmware version reported by the device
	 *                  Sometimes this value is important for select the related device-XML-File
	 *
	 * modelID:         Important for identification of the device.
	 *                  @See Device-XML-File /device/supported_types/type/parameter/const_value
	 *
	 * subType:         Identifier if device is a switch or a blind or a remote
	 * DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
	 *                  Other bytes not known.
	 *                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
	 */
	const uint8_t dev_static[] PROGMEM = {             // testID 
		/* firmwareVersion 1 byte */  0x25,           // or GE 
		/* modelID         2 byte */  0x00,0x67,
		/* subTypeID       1 byte */  0x00,           // replace __ by a valid type id 
		/* deviceInfo      3 byte */  0x41,0x01,0x00, // device info not found, replace by valid values 
	};



	/**
	 * @brief Regular start function
	 * This function is called by the main function every time when the device starts,
	 * here we can setup everything which is needed for a proper device operation
	 */
	void everyTimeStart(void) {
		dbg << F("\nevery time start\n");

		// channel 0 section 
		led.set(welcome);
		btn.config(2);
		pom.setMode(POWER_MODE_NO_SLEEP);
		bat.set(30, 3600000);
		// channel 1 section 
	}

	/**
	 * @brief First time start function
	 * This function is called by the main function on the first boot of a device.
	 * First boot is indicated by a magic byte in the eeprom.
	 * Here we can setup everything which is needed for a proper device operation, like cleaning
	 * of eeprom variables, or setting a default link in the peer table for 2 channels
	 */
	void firstTimeStart(void) {
		dbg << F("\n\nnew magic!\n\n");


	}





#endif

/**
 * @brief Channel structs (for developers)
 * Within the channel struct you will find the definition of the respective registers per channel and list.
 * These information is only needed if you want to develop your own channel module, for pre defined
 * channel modules all this definitions enclosed in the pre defined module.
 */

struct s_cnl0_lst0 {
	uint8_t                          : 7;  // 0x02.0, s:7   d:   
	uint8_t INTERNAL_KEYS_VISIBLE    : 1;  // 0x02.7, s:1   d: true  
	uint8_t MASTER_ID                : 24; // 0x0a.0, s:24  d:   
	uint8_t LOW_BAT_LIMIT            : 8;  // 0x12.0, s:8   d: 3 V 
	uint8_t LOCAL_RESET_DISABLE      : 1;  // 0x18.0, s:1   d: false  
	uint8_t                          : 7;  // 0x18.1, s:7   d:   
}; // 6 byte

struct s_cnl1_lst1 {
	uint8_t TRANSMIT_TRY_MAX         : 8;  // 0x30.0, s:8   d: 6  
	uint8_t OVERTEMP_LEVEL           : 8;  // 0x32.0, s:8   d: 80 °C 
	uint8_t REDUCE_TEMP_LEVEL        : 8;  // 0x34.0, s:8   d: 75 °C 
	uint8_t REDUCE_LEVEL             : 8;  // 0x35.0, s:8   d: 0.4 % 
	uint8_t POWERUP_ACTION           : 1;  // 0x56.0, s:1   d: POWERUP_OFF  
	uint8_t                          : 7;  // 0x56.1, s:7   d:   
	uint8_t STATUSINFO_MINDELAY      : 5;  // 0x57.0, s:5   d: 2.0 s 
	uint8_t STATUSINFO_RANDOM        : 3;  // 0x57.5, s:3   d: 1.0 s 
	uint8_t CHARACTERISTIC           : 1;  // 0x58.0, s:1   d: CHARACTERISTIC_SQUARE  
	uint8_t                          : 7;  // 0x58.1, s:7   d:   
	uint8_t LOGIC_COMBINATION        : 5;  // 0x59.0, s:5   d: LOGIC_OR  
	uint8_t                          : 3;  // 0x59.5, s:3   d:   
}; // 8 byte

struct s_cnl1_lst3 {
	uint8_t SHORT_CT_RAMPON          : 4;  // 0x01.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_RAMPOFF         : 4;  // 0x01.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_ONDELAY         : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_OFFDELAY        : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_ON              : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_OFF             : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_COND_VALUE_LO      : 8;  // 0x04.0, s:8   d: 50  
	uint8_t SHORT_COND_VALUE_HI      : 8;  // 0x05.0, s:8   d: 100  
	uint8_t SHORT_ONDELAY_TIME       : 8;  // 0x06.0, s:8   d: 0 s 
	uint8_t SHORT_ON_TIME            : 8;  // 0x07.0, s:8   d: 111600.0 s 
	uint8_t SHORT_OFFDELAY_TIME      : 8;  // 0x08.0, s:8   d: 0 s 
	uint8_t SHORT_OFF_TIME           : 8;  // 0x09.0, s:8   d: 111600.0 s 
	uint8_t SHORT_ACTION_TYPE        : 4;  // 0x0a.0, s:4   d: INACTIVE  
	uint8_t                          : 2;  // 0x0a.4, s:2   d:   
	uint8_t SHORT_OFF_TIME_MODE      : 1;  // 0x0a.6, s:1   d: ABSOLUTE  
	uint8_t SHORT_ON_TIME_MODE       : 1;  // 0x0a.7, s:1   d: ABSOLUTE  
	uint8_t SHORT_JT_ON              : 4;  // 0x0b.0, s:4   d: ONDELAY  
	uint8_t SHORT_JT_OFF             : 4;  // 0x0b.4, s:4   d: ONDELAY  
	uint8_t SHORT_JT_ONDELAY         : 4;  // 0x0c.0, s:4   d: ONDELAY  
	uint8_t SHORT_JT_OFFDELAY        : 4;  // 0x0c.4, s:4   d: ONDELAY  
	uint8_t SHORT_JT_RAMPON          : 4;  // 0x0d.0, s:4   d: ONDELAY  
	uint8_t SHORT_JT_RAMPOFF         : 4;  // 0x0d.4, s:4   d: ONDELAY  
	uint8_t SHORT_OFFDELAY_BLINK     : 1;  // 0x0e.5, s:1   d: ON  
	uint8_t SHORT_ON_LEVEL_PRIO      : 1;  // 0x0e.6, s:1   d: HIGH  
	uint8_t SHORT_ONDELAY_MODE       : 1;  // 0x0e.7, s:1   d: SET_TO_OFF  
	uint8_t SHORT_OFF_LEVEL          : 8;  // 0x0f.0, s:8   d: 0.0 % 
	uint8_t SHORT_ON_MIN_LEVEL       : 8;  // 0x10.0, s:8   d: 0.1 % 
	uint8_t SHORT_ON_LEVEL           : 8;  // 0x11.0, s:8   d: 1.0 % 
	uint8_t SHORT_RAMP_START_STEP    : 8;  // 0x12.0, s:8   d: 0.05 % 
	uint8_t SHORT_RAMPON_TIME        : 8;  // 0x13.0, s:8   d: 0 s 
	uint8_t SHORT_RAMPOFF_TIME       : 8;  // 0x14.0, s:8   d: 0 s 
	uint8_t SHORT_DIM_MIN_LEVEL      : 8;  // 0x15.0, s:8   d: 0.0 % 
	uint8_t SHORT_DIM_MAX_LEVEL      : 8;  // 0x16.0, s:8   d: 1.0 % 
	uint8_t SHORT_DIM_STEP           : 8;  // 0x17.0, s:8   d: 0.0 % 
	uint8_t SHORT_OFFDELAY_STEP      : 8;  // 0x18.0, s:8   d: 0.05 % 
	uint8_t SHORT_OFFDELAY_NEWTIME   : 8;  // 0x19.0, s:8   d: 0.5 s 
	uint8_t SHORT_OFFDELAY_OLDTIME   : 8;  // 0x1a.0, s:8   d: 0.5 s 
	uint8_t SHORT_ELSE_ACTION_TYPE   : 4;  // 0x26.0, s:4   d: INACTIVE  
	uint8_t                          : 2;  // 0x26.4, s:2   d:   
	uint8_t SHORT_ELSE_OFF_TIME_MODE : 1;  // 0x26.6, s:1   d: ABSOLUTE  
	uint8_t SHORT_ELSE_ON_TIME_MODE  : 1;  // 0x26.7, s:1   d: ABSOLUTE  
	uint8_t SHORT_ELSE_JT_ON         : 4;  // 0x27.0, s:4   d: ONDELAY  
	uint8_t SHORT_ELSE_JT_OFF        : 4;  // 0x27.4, s:4   d: ONDELAY  
	uint8_t SHORT_ELSE_JT_ONDELAY    : 4;  // 0x28.0, s:4   d: ONDELAY  
	uint8_t SHORT_ELSE_JT_OFFDELAY   : 4;  // 0x28.4, s:4   d: ONDELAY  
	uint8_t SHORT_ELSE_JT_RAMPON     : 4;  // 0x29.0, s:4   d: ONDELAY  
	uint8_t SHORT_ELSE_JT_RAMPOFF    : 4;  // 0x29.4, s:4   d: ONDELAY  
	uint8_t LONG_CT_RAMPON           : 4;  // 0x81.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_RAMPOFF          : 4;  // 0x81.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_ONDELAY          : 4;  // 0x82.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_OFFDELAY         : 4;  // 0x82.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_ON               : 4;  // 0x83.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_OFF              : 4;  // 0x83.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_COND_VALUE_LO       : 8;  // 0x84.0, s:8   d: 50  
	uint8_t LONG_COND_VALUE_HI       : 8;  // 0x85.0, s:8   d: 100  
	uint8_t LONG_ONDELAY_TIME        : 8;  // 0x86.0, s:8   d: 0 s 
	uint8_t LONG_ON_TIME             : 8;  // 0x87.0, s:8   d: 111600.0 s 
	uint8_t LONG_OFFDELAY_TIME       : 8;  // 0x88.0, s:8   d: 0 s 
	uint8_t LONG_OFF_TIME            : 8;  // 0x89.0, s:8   d: 111600.0 s 
	uint8_t LONG_ACTION_TYPE         : 4;  // 0x8a.0, s:4   d: INACTIVE  
	uint8_t                          : 1;  // 0x8a.4, s:1   d:   
	uint8_t LONG_MULTIEXECUTE        : 1;  // 0x8a.5, s:1   d: OFF  
	uint8_t LONG_OFF_TIME_MODE       : 1;  // 0x8a.6, s:1   d: ABSOLUTE  
	uint8_t LONG_ON_TIME_MODE        : 1;  // 0x8a.7, s:1   d: ABSOLUTE  
	uint8_t LONG_JT_ON               : 4;  // 0x8b.0, s:4   d: ONDELAY  
	uint8_t LONG_JT_OFF              : 4;  // 0x8b.4, s:4   d: ONDELAY  
	uint8_t LONG_JT_ONDELAY          : 4;  // 0x8c.0, s:4   d: ONDELAY  
	uint8_t LONG_JT_OFFDELAY         : 4;  // 0x8c.4, s:4   d: ONDELAY  
	uint8_t LONG_JT_RAMPON           : 4;  // 0x8d.0, s:4   d: ONDELAY  
	uint8_t LONG_JT_RAMPOFF          : 4;  // 0x8d.4, s:4   d: ONDELAY  
	uint8_t LONG_OFFDELAY_BLINK      : 1;  // 0x8e.5, s:1   d: ON  
	uint8_t LONG_ON_LEVEL_PRIO       : 1;  // 0x8e.6, s:1   d: HIGH  
	uint8_t LONG_ONDELAY_MODE        : 1;  // 0x8e.7, s:1   d: SET_TO_OFF  
	uint8_t LONG_OFF_LEVEL           : 8;  // 0x8f.0, s:8   d: 0.0 % 
	uint8_t LONG_ON_MIN_LEVEL        : 8;  // 0x90.0, s:8   d: 0.1 % 
	uint8_t LONG_ON_LEVEL            : 8;  // 0x91.0, s:8   d: 1.0 % 
	uint8_t LONG_RAMP_START_STEP     : 8;  // 0x92.0, s:8   d: 0.05 % 
	uint8_t LONG_RAMPON_TIME         : 8;  // 0x93.0, s:8   d: 0 s 
	uint8_t LONG_RAMPOFF_TIME        : 8;  // 0x94.0, s:8   d: 0 s 
	uint8_t LONG_DIM_MIN_LEVEL       : 8;  // 0x95.0, s:8   d: 0.0 % 
	uint8_t LONG_DIM_MAX_LEVEL       : 8;  // 0x96.0, s:8   d: 1.0 % 
	uint8_t LONG_DIM_STEP            : 8;  // 0x97.0, s:8   d: 0.0 % 
	uint8_t LONG_OFFDELAY_STEP       : 8;  // 0x98.0, s:8   d: 0.05 % 
	uint8_t LONG_OFFDELAY_NEWTIME    : 8;  // 0x99.0, s:8   d: 0.5 s 
	uint8_t LONG_OFFDELAY_OLDTIME    : 8;  // 0x9a.0, s:8   d: 0.5 s 
	uint8_t LONG_ELSE_ACTION_TYPE    : 4;  // 0xa6.0, s:4   d: INACTIVE  
	uint8_t                          : 1;  // 0xa6.4, s:1   d:   
	uint8_t LONG_ELSE_MULTIEXECUTE   : 1;  // 0xa6.5, s:1   d: OFF  
	uint8_t LONG_ELSE_OFF_TIME_MODE  : 1;  // 0xa6.6, s:1   d: ABSOLUTE  
	uint8_t LONG_ELSE_ON_TIME_MODE   : 1;  // 0xa6.7, s:1   d: ABSOLUTE  
	uint8_t LONG_ELSE_JT_ON          : 4;  // 0xa7.0, s:4   d: ONDELAY  
	uint8_t LONG_ELSE_JT_OFF         : 4;  // 0xa7.4, s:4   d: ONDELAY  
	uint8_t LONG_ELSE_JT_ONDELAY     : 4;  // 0xa8.0, s:4   d: ONDELAY  
	uint8_t LONG_ELSE_JT_OFFDELAY    : 4;  // 0xa8.4, s:4   d: ONDELAY  
	uint8_t LONG_ELSE_JT_RAMPON      : 4;  // 0xa9.0, s:4   d: ONDELAY  
	uint8_t LONG_ELSE_JT_RAMPOFF     : 4;  // 0xa9.4, s:4   d: ONDELAY  
}; // 60 byte

 /**
 * @brief Message description:
 *
 *        00        01 02    03 04 05  06 07 08  09  10  11   12     13
 * Length MSG_Count    Type  Sender__  Receiver  ACK Cnl Stat Action RSSI
 * 0F     12        80 02    1E 7A AD  23 70 EC  01  01  BE   20     27    dimmer
 * 0E     5C        80 02    1F B7 4A  63 19 63  01  01  C8   00     42    pcb relay
 *
 * Needed frames:
 *
 * // INSTRUCTION_SET
 * <frame id="LEVEL_SET" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
 *      <parameter type="integer" index="12.0" size="2.0" PARAM="RAMP_TIME"/>
 *      <parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
 * <frame id="RAMP_STOP" direction="to_device" type="0x11" subtype="0x03" subtype_index="9" channel_field="10">
 * <frame id="OLD_LEVEL" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" const_value="201"/>
 *      <parameter type="integer" index="12.0" size="2.0" PARAM="RAMP_TIME"/>
 *      <parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
 * <frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
 *      <parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
 * <frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="14"/>
 * <frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
 *      <parameter type="integer" index="12.1" size="0.3" param="ERROR"/>
 *      <parameter type="integer" index="12.1" size="0.1" param="ERROR_OVERLOAD"/>
 *      <parameter type="integer" index="12.2" size="0.1" param="ERROR_OVERHEAT"/>
 *      <parameter type="integer" index="12.3" size="0.1" param="ERROR_REDUCED"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 *      <parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
 *
 * // INFO_ACTUATOR_STATUS
 * <frame id="ACK_STATUS" direction="from_device" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="LEVEL"/>
 *      <parameter type="integer" index="12.1" size="0.3" param="ERROR"/>
 *      <parameter type="integer" index="12.1" size="0.1" param="ERROR_OVERLOAD"/>
 *      <parameter type="integer" index="12.2" size="0.1" param="ERROR_OVERHEAT"/>
 *      <parameter type="integer" index="12.3" size="0.1" param="ERROR_REDUCED"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 *      <parameter type="integer" index="12.4" size="0.2" param="DIRECTION_FLAGS"/>
 *
 * // INSTRUCTION_SET
 * <frame id="TOGGLE_INSTALL_TEST" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="TOGGLE_FLAG"/>
 *      <parameter type="integer" index="12.0" size="2.0" const_value="0"/>
 *
 * // INFO_ACTUATOR_STATUS
 * <frame id="INFO_POWERON" direction="from_device" allowed_receivers="CENTRAL" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="0"/>
 *      <parameter type="integer" const_value="0" param="LEVEL"/>
 *      <parameter type="integer" const_value="0" param="STATE_FLAGS"/>
 *      <parameter type="integer" const_value="0" param="INHIBIT"/>
 */

