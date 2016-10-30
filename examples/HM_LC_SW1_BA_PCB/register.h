
#ifndef _REGISTER_h
	#define _REGISTER_h
	
	/**
	 * @brief Libraries needed to run AskSin library
	 */
	#include <AS.h> 
	#include "hardware.h"
	#include "hmkey.h"
	#include <cmMaintenance.h> 
	#include <cmSwitch.h> 

	/**
	 * @brief Stage the modules and declare external functions.
	 *
	 * This functions are the call backs for user modules,
	 * declaration is in the register.h but the functions needs
	 * to be defined in the user sketch.
	 */
	AS hm;


	/*
	* cmSwitch requires this functions in the user sketch:
	* void cmSwitch::initSwitch(uint8_t channel);
	* void cmSwitch::switchSwitch(uint8_t channel, uint8_t status);
	*/
	const uint8_t cmMaintenance_ChnlReg[] PROGMEM = { 0x02,0x05,0x0a,0x0b,0x0c,0x12, };
	const uint8_t cmMaintenance_ChnlDef[] PROGMEM = { 0x80,0x00,0x00,0x00,0x00,0x69, };
	const uint8_t cmMaintenance_ChnlLen = 6;

	cmMaster *ptr_CM[3] = {
		new cmMaintenance(0),
		new cmSwitch(11),
	};


	/*
	 * @brief HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x33,0x11,0x22,
		/* Serial number */   'H','B','s','w','i','t','c','h','0','1',		// HBswitch01 
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
		/* firmwareVersion 1 byte */  0x15,           // or GE 
		/* modelID         2 byte */  0x00,0x6c,
		/* subTypeID       1 byte */  0x10,           // replace __ by a valid type id 
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
	uint8_t                       : 7;  // 0x02.0, s:7   d:   
	uint8_t INTERNAL_KEYS_VISIBLE : 1;  // 0x02.7, s:1   d: true  
	uint8_t MASTER_ID             : 24; // 0x0a.0, s:24  d:   
	uint8_t LOW_BAT_LIMIT         : 8;  // 0x12.0, s:8   d: 3 V 
	uint8_t LOCAL_RESET_DISABLE   : 1;  // 0x18.0, s:1   d: false  
	uint8_t                       : 7;  // 0x18.1, s:7   d:   
}; // 6 byte

struct s_cnl1_lst1 {
	uint8_t AES_ACTIVE            : 1;  // 0x08.0, s:1   d: false  
	uint8_t                       : 7;  // 0x08.1, s:7   d:   
}; // 1 byte

struct s_cnl1_lst3 {
	uint8_t SHORT_CT_ONDELAY      : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_OFFDELAY     : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_ON           : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_CT_OFF          : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t SHORT_COND_VALUE_LO   : 8;  // 0x04.0, s:8   d: 50  
	uint8_t SHORT_COND_VALUE_HI   : 8;  // 0x05.0, s:8   d: 100  
	uint8_t SHORT_ONDELAY_TIME    : 8;  // 0x06.0, s:8   d: 0 s 
	uint8_t SHORT_ON_TIME         : 8;  // 0x07.0, s:8   d: 111600.0 s 
	uint8_t SHORT_OFFDELAY_TIME   : 8;  // 0x08.0, s:8   d: 0 s 
	uint8_t SHORT_OFF_TIME        : 8;  // 0x09.0, s:8   d: 111600.0 s 
	uint8_t SHORT_ACTION_TYPE     : 2;  // 0x0a.0, s:2   d: JUMP_TO_TARGET  
	uint8_t                       : 4;  // 0x0a.2, s:4   d:   
	uint8_t SHORT_OFF_TIME_MODE   : 1;  // 0x0a.6, s:1   d: ABSOLUTE  
	uint8_t SHORT_ON_TIME_MODE    : 1;  // 0x0a.7, s:1   d: ABSOLUTE  
	uint8_t SHORT_JT_ON           : 4;  // 0x0b.0, s:4   d: OFF  
	uint8_t SHORT_JT_OFF          : 4;  // 0x0b.4, s:4   d: OFF  
	uint8_t SHORT_JT_ONDELAY      : 4;  // 0x0c.0, s:4   d: OFF  
	uint8_t SHORT_JT_OFFDELAY     : 4;  // 0x0c.4, s:4   d: OFF  
	uint8_t LONG_CT_ONDELAY       : 4;  // 0x82.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_OFFDELAY      : 4;  // 0x82.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_ON            : 4;  // 0x83.0, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_CT_OFF           : 4;  // 0x83.4, s:4   d: X GE COND_VALUE_LO  
	uint8_t LONG_COND_VALUE_LO    : 8;  // 0x84.0, s:8   d: 50  
	uint8_t LONG_COND_VALUE_HI    : 8;  // 0x85.0, s:8   d: 100  
	uint8_t LONG_ONDELAY_TIME     : 8;  // 0x86.0, s:8   d: 0 s 
	uint8_t LONG_ON_TIME          : 8;  // 0x87.0, s:8   d: 111600.0 s 
	uint8_t LONG_OFFDELAY_TIME    : 8;  // 0x88.0, s:8   d: 0 s 
	uint8_t LONG_OFF_TIME         : 8;  // 0x89.0, s:8   d: 111600.0 s 
	uint8_t LONG_ACTION_TYPE      : 2;  // 0x8a.0, s:2   d: JUMP_TO_TARGET  
	uint8_t                       : 3;  // 0x8a.2, s:3   d:   
	uint8_t LONG_MULTIEXECUTE     : 1;  // 0x8a.5, s:1   d: ON  
	uint8_t LONG_OFF_TIME_MODE    : 1;  // 0x8a.6, s:1   d: ABSOLUTE  
	uint8_t LONG_ON_TIME_MODE     : 1;  // 0x8a.7, s:1   d: ABSOLUTE  
	uint8_t LONG_JT_ON            : 4;  // 0x8b.0, s:4   d: OFF  
	uint8_t LONG_JT_OFF           : 4;  // 0x8b.4, s:4   d: OFF  
	uint8_t LONG_JT_ONDELAY       : 4;  // 0x8c.0, s:4   d: OFF  
	uint8_t LONG_JT_OFFDELAY      : 4;  // 0x8c.4, s:4   d: OFF  
}; // 22 byte

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
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.0" size="2.0" const_value="0"/>
 *      <parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
 * <frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
 *      <parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
 * <frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="14"/>
 *
 * // INFO_ACTUATOR_STATUS
 * <frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 *
 * // ACK_STATUS
 * <frame id="ACK_STATUS" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 *
 * // INSTRUCTION_SET
 * <frame id="TOGGLE_INSTALL_TEST" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="TOGGLE_FLAG"/>
 *      <parameter type="integer" index="12.0" size="2.0" const_value="0"/>
 *
 * // INFO_ACTUATOR_STATUS
 * <frame id="INFO_POWERON" direction="from_device" allowed_receivers="CENTRAL" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="0"/>
 *      <parameter type="integer" const_value="0" param="STATE"/>
 *      <parameter type="integer" const_value="0" param="STATE_FLAGS"/>
 *      <parameter type="integer" const_value="0" param="INHIBIT"/>
 */