#ifndef _REGISTER_h
	#define _REGISTER_h

	/**
	 * @brief Libraries needed to run AskSin library
	 */
	#include <AS.h> 
	#include <Registrar.h>
	#include "hardware.h"
	#include "hmkey.h"
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
	cmSwitch cm_Switch[1];


	/*
	 * @brief HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x33,0x11,0x22,
		/* Serial number */   'H','B','s','w','i','t','c','h','0','1',		// HBswitch01 
		/* Default-Key */     HM_DEVICE_AES_KEY,
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX,
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
	const uint8_t devIdnt[] PROGMEM = {               // testID 
		/* firmwareVersion 1 byte */  0x10,           // or GE 
		/* modelID         2 byte */  0x00,0x6c,
		/* subTypeID       1 byte */  0x00,           // replace __ by a valid type id 
		/* deviceInfo      3 byte */  0x41,0x01,0x00, // device info not found, replace by valid values 
	};


	/**
	 * @brief Register definitions
	 * The values are adresses in relation to the start adress defines in cnlTbl
	 * Register values can found in related Device-XML-File.
	 *
	 * Spechial register list 0: 0x0A, 0x0B, 0x0C
	 * Spechial register list 1: 0x08
	 *
	 * @See Defines.h
	 *
	 * @See: cnlTbl
	 */
	const uint8_t cnlAddr[] PROGMEM = {
		// channel: 0, list: 0
		0x02,0x0a,0x0b,0x0c,0x12,0x18,
		// channel: 1, list: 1
		0x08,
		// channel: 1, list: 3
		0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
	}; // 29 byte
	   

	/**
	 * @brief Channel, List defaults
	 * Source of the default values is the respective xml file.
	 * This values are the defined default values and should be set
	 * in the first start function.
	 */
	const uint8_t cnlDefs[] = {
		// channel: 0, list: 0
		0x80,0x00,0x00,0x00,0x1e,0x00,
		// channel: 1, list: 1
		0x00,
		// channel: 1, list: 3
		0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x01,0x44,0x44,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x21,0x44,0x44,
	}; // 29 byte


	/**
	 * @brief Channel - List translation table
	 * channel, list, startIndex, start address in EEprom, hidden
	 * do not edit the table, if you need more peers edit the defines accordingly.
	 */
	#define PHY_ADDR_START 0x20
	#define CNL_01_PEERS   10 

	const EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx, sLen, hide, pAddr 
		{    0,   0,    0,    6,    0, PHY_ADDR_START },
		{    1,   1,    6,    1,    0, cnlTbl[0].pAddr + cnlTbl[0].sLen },
		{    1,   3,    7,   22,    0, cnlTbl[1].pAddr + cnlTbl[1].sLen },
	}; // 21 byte 
	//	{ 0, 0, 0x00,  6, 0x001f, 0, },
	//	{ 1, 1, 0x00,  0, 0x0000, 0, },
	//	{ 1, 3, 0x06, 22, 0x0025, 0, },


	/**
	 * @brief Peer-Device-List-Table
	 * maximum allowed peers, link to row in cnlTbl, start address in EEprom
	 */
	const EE::s_peerTbl peerTbl[] = {
		//    pMax, pLink, pAddr; 
		{            0, 0, cnlTbl[2].pAddr + (cnlTbl[2].sLen * CNL_01_PEERS) },
		{ CNL_01_PEERS, 2, peerTbl[0].pAddr + (peerTbl[0].pMax * 4) },
	}; // 8 byte

	/**
	 * @brief Struct with basic information for the AskSin library.
	 * amount of user channels, amount of lines in the channel table,
	 * link to devIdent byte array, link to cnlAddr byte array
	 */
	const uint8_t cnl_max = 1;
	const uint8_t cnl_tbl_max = 3;

	/**
	 * @brief Sizing of the user module register table.
	 * Within this register table all user modules are registered to make
	 * them accessible for the AskSin library
	 */
	RG::s_modTable modTbl[cnl_max + 1];

	/**
	 * @brief Regular start function
	 * This function is called by the main function every time when the device starts,
	 * here we can setup everything which is needed for a proper device operation
	 */
	void everyTimeStart(void) {

		// channel 0 section 
		hm.ld.set(welcome);
		hm.confButton.config(2);
		hm.pw.setMode(POWER_MODE_NO_SLEEP);
		hm.bt.set(30, 3600000);
		// channel 1 section 
		cm_Switch[0].regInHM(1);
	}

	/**
	 * @brief First time start function
	 * This function is called by the main function on the first boot of a device.
	 * First boot is indicated by a magic byte in the eeprom.
	 * Here we can setup everything which is needed for a proper device operation, like cleaning
	 * of eeprom variables, or setting a default link in the peer table for 2 channels
	 */
	void firstTimeStart(void) {

		#ifdef SER_DBG
		/*// some debug
		dbg << F("First time start active:\n");
		dbg << F("cnl\tlst\tsIdx\tsLen\thide\tpAddr\n");
		for (uint8_t i = 0; i < cnl_tbl_max; i++) {
			// cnl, lst, sIdx, sLen, hide, pAddr 
			dbg << cnlTbl[i].cnl << "\t" << cnlTbl[i].lst << "\t" << cnlTbl[i].sIdx << "\t" << cnlTbl[i].sLen << "\t" << cnlTbl[i].vis << "\t" << cnlTbl[i].pAddr << "\n";
		}*/
		#endif

		// fill register with default values, peer registers are not filled while done in usermodules
		hm.ee.setList(0, 0, 0, (uint8_t*)&cnlDefs[0]);
		hm.ee.setList(1, 1, 0, (uint8_t*)&cnlDefs[6]);

		// format peer db
		hm.ee.clearPeers();
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
 * <frame id="LEVEL_SET" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.0" size="2.0" const_value="0"/>
 *      <parameter type="integer" index="14.0" size="2.0" PARAM="ON_TIME" omit_if="0"/>
 * <frame id="SET_LOCK" direction="to_device" type="0x11" channel_field="10">
 *      <parameter type="integer" index="9.0" size="0.1" param="INHIBIT"/>
 * <frame id="LEVEL_GET" direction="to_device" type="0x01" channel_field="9">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="14"/>
 * <frame id="INFO_LEVEL" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x10" subtype="6" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 * <frame id="ACK_STATUS" direction="from_device" allowed_receivers="BROADCAST,CENTRAL,OTHER" event="true" type="0x02" subtype="1" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="STATE"/>
 *      <parameter type="integer" index="12.4" size="0.3" param="STATE_FLAGS"/>
 * <frame id="TOGGLE_INSTALL_TEST" direction="to_device" type="0x11" subtype="0x02" subtype_index="9" channel_field="10">
 *      <parameter type="integer" index="11.0" size="1.0" param="TOGGLE_FLAG"/>
 *      <parameter type="integer" index="12.0" size="2.0" const_value="0"/>
 * <frame id="INFO_POWERON" direction="from_device" allowed_receivers="CENTRAL" event="true" type="0x10" subtype="6" subtype_index="9" fixed_channel="*">
 *      <parameter type="integer" index="10.0" size="1.0" const_value="0"/>
 *      <parameter type="integer" const_value="0" param="STATE"/>
 *      <parameter type="integer" const_value="0" param="STATE_FLAGS"/>
 *      <parameter type="integer" const_value="0" param="INHIBIT"/>
 */