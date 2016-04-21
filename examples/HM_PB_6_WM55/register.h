#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load libraries -------------------------------------------------------------------------------------------------------
	#include <AS.h>
	#include "hardware.h"                                                   // hardware definition
	#include <cmKey.h>
	#include "hmkey.h"

	//- stage modules --------------------------------------------------------------------------------------------------------
	AS hm;                                                                  // asksin framework
	cmKey cmKey[6];                                                      // create instances of channel module

	//cmSwitch cmSwitch[1];													// create instances of channel module
	//extern void initRly(uint8_t channel);                                   // declare function to jump in
	//extern void switchRly(uint8_t channel, uint8_t status);                 // declare function to jump in

	/*
	* HMID, Serial number, HM-Default-Key, Key-Index
	*/
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x01, 0x02, 0x08,
		/* Serial number */   'H', 'B', 'r', 'e', 'm', 'o', 't', 'e', '0', '1',		// HBremote01
		/* Default-Key */     HM_DEVICE_AES_KEY,
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX,
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
	const uint8_t devIdnt[] PROGMEM = {
		/* firmwareVersion 1 byte */  0x11,
		/* modelID         2 byte */  0x00,0xa9,
		/* subTypeID       1 byte */  0x40,
		/* deviceInfo      3 byte */  0x06, 0x00, 0x00,
	};

	/*
	* Register definitions
	* The values are adresses in relation to the start adress defines in cnlTbl
	* Register values can found in related Device-XML-File.

	* Spechial register list 0: 0x0A, 0x0B, 0x0C
	* Spechial register list 1: 0x08
	*
	* @See Defines.h
	*
	* @See: cnlTbl
	*/
	const uint8_t cnlAddr[] PROGMEM = {
		// List0-Register
		0x01,0x02,0x0a,0x0b,0x0c,0x18,
		// List1-Register
		0x04,0x08,0x09,
		// List3-Register
		0x01,
	};  // 10 byte

	/*
	* Channel - List translation table
	* channel, list, startIndex, start address in EEprom, hidden
	*/
	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{ 0,   0,   0x00,  6,   0x0020, 0, },
		{ 1,   1,   0x06,  3,   0x0026, 0, },
		{ 1,   4,   0x09,  1,   0x0029, 0, },
		{ 2,   1,   0x06,  3,   0x002f, 0, },
		{ 2,   4,   0x09,  1,   0x0032, 0, },
		{ 3,   1,   0x06,  3,   0x0038, 0, },
		{ 3,   4,   0x09,  1,   0x003b, 0, },
		{ 4,   1,   0x06,  3,   0x0041, 0, },
		{ 4,   4,   0x09,  1,   0x0044, 0, },
		{ 5,   1,   0x06,  3,   0x004a, 0, },
		{ 5,   4,   0x09,  1,   0x004d, 0, },
		{ 6,   1,   0x06,  3,   0x0053, 0, },
		{ 6,   4,   0x09,  1,   0x0056, 0, },
	}; // 91 byte

	/*
	* Peer-Device-List-Table
	* channel, maximum allowed peers, start address in EEprom
	*/
	EE::s_peerTbl peerTbl[] = {
		// cnl, peerMax, pAddr;
		{ 1, 6, 0x0099, },
	};
	
	/*
	* handover to AskSin lib
	*
	* TODO: Describe
	*/
	EE::s_devDef devDef = {
		6, 13, devIdnt, cnlAddr,
	}; // 6 byte

	/*
	* module registrar
	*
	* TODO: Describe
	*/
	RG::s_modTable modTbl[1];


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
		hm.confButton.config(2, CONFIG_KEY_PCIE, CONFIG_KEY_INT);           // configure the config button, mode, pci byte and pci bit
		hm.ld.init(2, &hm);                                                 // set the led
		hm.ld.set(welcome);                                                 // show something
		hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
		hm.pw.setMode(POWER_MODE_NO_SLEEP);                                 // set power management mode

		// register user modules
		//cmSwitch[0].regInHM(1, 3, &hm);                                    // register user module
		//cmSwitch[0].config(&initRly, &switchRly);                          // configure user module

	}

	/**
	* @brief First time start function
	* This function is called by the main function on the first boot of a device. 
	* First boot is indicated by a magic byte in the eeprom.
	* Here we can setup everything which is needed for a proper device operation, like cleaning 
	* of eeprom variables, or setting a default link in the peer table for 2 channels
	*/
	void firstTimeStart(void) {
	}
#endif
