
#ifndef _REGISTER_h
#define _REGISTER_h

/**
* @brief Libraries needed to run AskSin library
*/
#include <AS.h> 
#include "hardware.h"
#include "hmkey.h"
#include <cmMaintenance.h> 
#include <cmRemote.h> 

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
const uint8_t cmMaintenance_ChnlReg[] PROGMEM = { 0x02,0x08,0x0a,0x0b,0x0c,0x12, };
const uint8_t cmMaintenance_ChnlDef[] PROGMEM = { 0x80,0x01,0x00,0x00,0x00,0x69, };
const uint8_t cmMaintenance_ChnlLen = 6;

cmMaster *ptr_CM[7] = {
	new cmMaintenance(0),
	new cmRemote(11),
	new cmRemote(10),
	new cmRemote(10),
	new cmRemote(10),
	new cmRemote(10),
	new cmRemote(10),
};


/*
* @brief HMID, Serial number, HM-Default-Key, Key-Index
*/
const uint8_t HMSerialData[] PROGMEM = {
	/* HMID */            0x00,0x11,0x22,
	/* Serial number */   'H','B','r','e','m','o','t','e','0','1',		// HBremote01 
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
	/* firmwareVersion 1 byte */  0x11,           // or GE 
	/* modelID         2 byte */  0x00,0xa9,
	/* subTypeID       1 byte */  0x40,           // replace __ by a valid type id 
	/* deviceInfo      3 byte */  0x06,0x00,0x00, // device info not found, replace by valid values 
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
	btn.config(1);
	pom.setMode(POWER_MODE_NO_SLEEP);
	bat.set(30, 3600000);
	// channel 1 section 
	ptr_CM[1]->cm_init_pin(PIN_C0);
	ptr_CM[2]->cm_init_pin(PIN_C1);
	ptr_CM[3]->cm_init_pin(PIN_C2);
	ptr_CM[4]->cm_init_pin(PIN_C3);
	ptr_CM[5]->cm_init_pin(PIN_C4);
	ptr_CM[6]->cm_init_pin(PIN_C5);
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

	/* add some peers to test - peers can be added per default, important if we want to build up combined devices.
	*  by doing this, we can build a light switch by combing a remote and a switch channel.
	*  first we need an array with the peer address and in a second step it is written into the respective channel.
	*  set_peer needs two parameters, the index which reflects the slot where the peer is written to and second the peer 
	*  address as array. please note: no defaults a written into the respective peer list, this has to be done manually */
	uint8_t temp[] = { 0x01,0x02,0x01,0x01, };												// declare and fill array
	ptr_CM[1]->peerDB.set_peer(0, temp);													// write it to index 0
	//temp[2] = 0x02;																			// adjust array content
	//ptr_CM[1]->peerDB.set_peer(1, temp);													// write to index 1
	//temp[2] = 0x03;
	//ptr_CM[1]->peerDB.set_peer(2, temp);
	//temp[2] = 0x04;
	//ptr_CM[1]->peerDB.set_peer(3, temp);

	/* this example shows how peer lists (list3/4) could be set manually. in the first example we set default values
	*  to channel 1 list 4 for the first peer. in the second example we write a custom information to channel 1 list 4
	*  peer 2,3 and 4. default values are stored in the respective channel module */
	//ptr_CM[1]->lstP.load_default();															// load the defaults into the list 4
	//ptr_CM[1]->lstP.save_list(0);															// write it to index 0

	//temp[0] = 0x01; temp[1] = 0x01;															// adjust array content
	//ptr_CM[1]->lstP.write_array(temp, 2, 1);												// write 2 bytes into index 1
	//ptr_CM[1]->lstP.write_array(temp, 2, 2);
	//ptr_CM[1]->lstP.write_array(temp, 2, 3);
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
	uint8_t                       : 4;  // 0x04.0, s:4   d:   
	uint8_t LONG_PRESS_TIME       : 4;  // 0x04.4, s:4   d: 0.4 s 
	uint8_t AES_ACTIVE            : 1;  // 0x08.0, s:1   d: false  
	uint8_t                       : 7;  // 0x08.1, s:7   d:   
	uint8_t DBL_PRESS_TIME        : 4;  // 0x09.0, s:4   d: 0.0 s 
	uint8_t                       : 4;  // 0x09.4, s:4   d:   
}; // 3 byte

struct s_cnl1_lst4 {
	uint8_t PEER_NEEDS_BURST      : 1;  // 0x01.0, s:1   d: false  
    uint8_t                       : 6;  // 0x01.1, s:6   d:   
	uint8_t EXPECT_AES            : 1;  // 0x01.7, s:1   d: false  
}; // 1 byte

   // struct s_cnl2_lst1 linked to 01 01
   // struct s_cnl2_lst4 linked to 01 04
   // struct s_cnl3_lst1 linked to 01 01
   // struct s_cnl3_lst4 linked to 01 04
   // struct s_cnl4_lst1 linked to 01 01
   // struct s_cnl4_lst4 linked to 01 04
   // struct s_cnl5_lst1 linked to 01 01
   // struct s_cnl5_lst4 linked to 01 04
   // struct s_cnl6_lst1 linked to 01 01
   // struct s_cnl6_lst4 linked to 01 04

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
   * <frame id="KEY_EVENT_SHORT" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
   *      <parameter type="integer" index="9.6" size="0.1" const_value="0"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
   * <frame id="KEY_EVENT_LONG" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
   *      <parameter type="integer" index="9.6" size="0.1" const_value="1"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
   * <frame id="KEY_EVENT_LONG_BIDI" direction="from_device" allowed_receivers="CENTRAL,BROADCAST,OTHER" event="true" type="0x40" channel_field="9:0.6">
   *      <parameter type="integer" index="1.5" size="0.1" const_value="1"/>
   *      <parameter type="integer" index="9.6" size="0.1" const_value="1"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="COUNTER"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="TEST_COUNTER"/>
   * <frame id="KEY_SIM_SHORT" direction="from_device" type="0x40" channel_field="9:0.6">
   *      <parameter type="integer" index="9.6" size="0.1" const_value="0"/>
   *      <parameter type="integer" index="9.7" size="0.1" const_value="0"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="SIM_COUNTER"/>
   * <frame id="KEY_SIM_LONG" direction="from_device" type="0x40" channel_field="9:0.6">
   *      <parameter type="integer" index="9.6" size="0.1" const_value="1"/>
   *      <parameter type="integer" index="9.7" size="0.1" const_value="0"/>
   *      <parameter type="integer" index="10.0" size="1.0" param="SIM_COUNTER"/>
   */

