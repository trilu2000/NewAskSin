#ifndef _REGISTER_h
	#define _REGISTER_h

	#include <AS.h>                                                       // the asksin framework
	#include "hardware.h"                                                 // hardware definition
	#include <cmRemote.h>
	#include "hmkey.h"


	AS hm;                                                               // asksin framework
	cmRemote cm_Remote[6];                                               // create instances of channel module

	/*
	* HMID, Serial number, HM-Default-Key, Key-Index
	*/
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x5d,0xa8,0x79,
		/* Serial number */   'H','B','r','e','m','o','t','e','0','1',   // HBremote01
		/* Default-Key */     HM_DEVICE_AES_KEY,
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX
	};

	/*
	* Settings of HM device
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
	const uint8_t devIdnt[] PROGMEM = {               // HM-PB-6-WM55 
		/* firmwareVersion 1 byte */  0x11,           // or GE 
		/* modelID         2 byte */  0x00,0xa9,
		/* subTypeID       1 byte */  0x40,           // replace __ by a valid type id 
		/* deviceInfo      3 byte */  0x06,0x00,0x00, // device info not found, replace by valid values 
	};

	/*
	* Register definitions
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
		0x0a,0x0b,0x0c,0x18,
		// channel: 1, list: 1
		0x04,0x08,0x09,
		// channel: 1, list: 4
		0x01,
		// channel: 2, list: 1, link to 01 01
		// channel: 2, list: 4, link to 01 04
		// channel: 3, list: 1, link to 01 01
		// channel: 3, list: 4, link to 01 04
		// channel: 4, list: 1, link to 01 01
		// channel: 4, list: 4, link to 01 04
		// channel: 5, list: 1, link to 01 01
		// channel: 5, list: 4, link to 01 04
		// channel: 6, list: 1, link to 01 01
		// channel: 6, list: 4, link to 01 04
	}; // 8 byte

	/*
	* Channel - List translation table
	* channel, list, startIndex, start address in EEprom, hidden
	* do not edit the table, if you need more peers edit the defines accordingly.
	*/
	#define PHY_ADDR_START 0x20
	#define CNL_01_PEERS   6 
	#define CNL_02_PEERS   6 
	#define CNL_03_PEERS   6 
	#define CNL_04_PEERS   6 
	#define CNL_05_PEERS   6 
	#define CNL_06_PEERS   6 

	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx, sLen, hide, pAddr 
		{ 0,   0,    0,    4,    0, PHY_ADDR_START },
		{ 1,   1,    4,    3,    0, cnlTbl[0].pAddr + cnlTbl[0].sLen },
		{ 1,   4,    7,    1,    0, cnlTbl[1].pAddr + cnlTbl[1].sLen },
		{ 2,   1,    4,    3,    0, cnlTbl[2].pAddr + (cnlTbl[2].sLen * CNL_01_PEERS) },
		{ 2,   4,    7,    1,    0, cnlTbl[3].pAddr + cnlTbl[3].sLen },
		{ 3,   1,    4,    3,    0, cnlTbl[4].pAddr + (cnlTbl[4].sLen * CNL_02_PEERS) },
		{ 3,   4,    7,    1,    0, cnlTbl[5].pAddr + cnlTbl[5].sLen },
		{ 4,   1,    4,    3,    0, cnlTbl[6].pAddr + (cnlTbl[6].sLen * CNL_03_PEERS) },
		{ 4,   4,    7,    1,    0, cnlTbl[7].pAddr + cnlTbl[7].sLen },
		{ 5,   1,    4,    3,    0, cnlTbl[8].pAddr + (cnlTbl[8].sLen * CNL_04_PEERS) },
		{ 5,   4,    7,    1,    0, cnlTbl[9].pAddr + cnlTbl[9].sLen },
		{ 6,   1,    4,    3,    0, cnlTbl[10].pAddr + (cnlTbl[10].sLen * CNL_05_PEERS) },
		{ 6,   4,    7,    1,    0, cnlTbl[11].pAddr + cnlTbl[11].sLen },
	}; // 91 byte 

	/*
	* Peer-Device-List-Table
	* channel, maximum allowed peers, start address in EEprom
	*/
	EE::s_peerTbl peerTbl[] = {
		// pMax, pAddr; 
		{ 0, cnlTbl[12].pAddr + (cnlTbl[12].sLen * CNL_06_PEERS) },
		{ CNL_01_PEERS, peerTbl[0].pAddr + (peerTbl[0].pMax * 4) },
		{ CNL_02_PEERS, peerTbl[1].pAddr + (peerTbl[1].pMax * 4) },
		{ CNL_03_PEERS, peerTbl[2].pAddr + (peerTbl[2].pMax * 4) },
		{ CNL_04_PEERS, peerTbl[3].pAddr + (peerTbl[3].pMax * 4) },
		{ CNL_05_PEERS, peerTbl[4].pAddr + (peerTbl[4].pMax * 4) },
		{ CNL_06_PEERS, peerTbl[5].pAddr + (peerTbl[5].pMax * 4) },
	}; // 18 byte 	
   

	/*
	* Device definition table
	* Parameter: amount of user channel(s), amount of lists,
	* pointer to device identification string and pointer to the channel table
	*/
	EE::s_devDef devDef = {
		6, 13, devIdnt, cnlAddr,
	}; // 6 byte

   /*
   * module registrar
   * size table to register and access channel modules
   */
	RG::s_modTable modTbl[6];


	/**
	* @brief Regular start function
	* This function is called by the main function every time when the device starts, 
	* here we can setup everything which is needed for a proper device operation
	*/
	void everyTimeStart(void) {
		/*
		* Place here everything which should be done on each start or reset of the device.
		* Typical use case are loading default values or user class configurations.
		*/

		// init the homematic framework
		hm.confButton.config(1);	                                          // configure the config button mode
		hm.ld.set(welcome);                                                 // show something
		hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
		hm.pw.setMode(POWER_MODE_NO_SLEEP);                                 // set power management mode

		// register user modules
		cm_Remote[0].config(PIN_C0);                                        // hand over the pin to check for action
		cm_Remote[0].regInHM(1, 4, &hm);                                    // register user module
		cm_Remote[1].config(PIN_C1);                                        // hand over the pin to check for action
		cm_Remote[1].regInHM(2, 4, &hm);                                    // register user module
		cm_Remote[2].config(PIN_C2);                                        // hand over the pin to check for action
		cm_Remote[2].regInHM(3, 4, &hm);                                    // register user module
		cm_Remote[3].config(PIN_C3);                                        // hand over the pin to check for action
		cm_Remote[3].regInHM(4, 4, &hm);                                    // register user module
		cm_Remote[4].config(PIN_C4);                                        // hand over the pin to check for action
		cm_Remote[4].regInHM(5, 4, &hm);                                    // register user module
		cm_Remote[5].config(PIN_C5);                                        // hand over the pin to check for action
		cm_Remote[5].regInHM(6, 4, &hm);                                    // register user module

	}

	/**
	* @brief First time start function
	* This function is called by the main function on the first boot of a device. 
	* First boot is indicated by a magic byte in the eeprom.
	* Here we can setup everything which is needed for a proper device operation, like cleaning 
	* of eeprom variables, or setting a default link in the peer table for 2 channels
	*/
	void firstTimeStart(void) {
		/*
		* place here everything which should be done on the first start or after a complete reset of the sketch
		* typical usecase are default values which should be written into the register or peer database
		*/

		const uint8_t cnl0lst0[] = {
			0x80,0x00,0x00,0x00,0x69,0x00,
		};

		const uint8_t cnl1lst1[] = {
			0x10,0x00,0x00,
		};

		const uint8_t cnl2lst1[] = {
			0x10,0x00,0x00,
		};

		const uint8_t cnl3lst1[] = {
			0x10,0x00,0x00,
		};

		const uint8_t cnl4lst1[] = {
			0x10,0x00,0x00,
		};

		const uint8_t cnl5lst1[] = {
			0x10,0x00,0x00,
		};

		const uint8_t cnl6lst1[] = {
			0x10,0x00,0x00,
		};

	}
#endif


	/*
	* Channel structs (for developers)
	* Within the channel struct you will find the definition of the respective registers per channel and list.
	* These information is only needed if you want to develop your own channel module, for pre defined
	* channel modules all this definitions enclosed in the pre defined module.
	*/

	struct s_cnl0_lst0 {
		uint8_t MASTER_ID : 24;              // 0x0a.0, s:24  d:   
		uint8_t LOCAL_RESET_DISABLE : 1;     // 0x18.0, s:1   d: false  
		uint8_t :7;                          // 0x18.1, s:7   d:   
	}; // 4 byte

	struct s_cnl1_lst1 {
		uint8_t : 4;                         // 0x04.0, s:4   d:   
		uint8_t LONG_PRESS_TIME : 4;         // 0x04.4, s:4   d: 0.4 s 
		uint8_t AES_ACTIVE : 1;              // 0x08.0, s:1   d: false  
		uint8_t : 7;                         // 0x08.1, s:7   d:   
		uint8_t DBL_PRESS_TIME : 4;          // 0x09.0, s:4   d: 0.0 s 
		uint8_t : 4;                         // 0x09.4, s:4   d:   
	}; // 3 byte

	struct s_cnl1_lst4 {
		uint8_t PEER_NEEDS_BURST : 1;        // 0x01.0, s:1   d: false  
		uint8_t : 6;                         // 0x01.1, s:6   d:   
		uint8_t EXPECT_AES : 1;              // 0x01.7, s:1   d: false  
	}; // 1 byte

	/*
	* Message description:
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
