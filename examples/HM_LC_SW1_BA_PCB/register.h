#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load libraries -------------------------------------------------------------------------------------------------------
	#include <AS.h>
	#include "hardware.h"                                                   // hardware definition
	#include <cmSwitch.h>
	#include "hmkey.h"

	//- stage modules --------------------------------------------------------------------------------------------------------
	AS hm;                                                                  // asksin framework

	cmSwitch cm_Switch[1];													// create instances of channel module
	extern void initRly(uint8_t channel);                                   // declare function to jump in
	extern void switchRly(uint8_t channel, uint8_t status);                 // declare function to jump in

	/*
	* HMID, Serial number, HM-Default-Key, Key-Index
	*/
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x01, 0x02, 0x06,
		/* Serial number */   'H', 'B', 's', 'w', 'i', 't', 'c', 'h', '0', '1',		// HBswitch01
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
		/* firmwareVersion 1 byte */  0x10,
		/* modelID         2 byte */  0x00,0x6c,
		/* subTypeID       1 byte */  0x00,
		/* deviceInfo      3 byte */  0x41, 0x01, 0x00,
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
		0x02,0x0a,0x0b,0x0c,0x12,0x18,
		// List3-Register
		0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
	};  // 28 byte

	/*
	* Channel - List translation table
	* channel, list, startIndex, start address in EEprom, hidden
	*/
	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{ 0, 0, 0x00,  6, 0x001f, 0, },
		{ 1, 1, 0x00,  0, 0x0000, 0, },
		{ 1, 3, 0x06, 22, 0x0025, 0, },
	};  // 21 byte

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
		1, 3, devIdnt, cnlAddr,
	};

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
		hm.confButton.config(2);                                            // configure the config button, mode, pci byte and pci bit
		hm.ld.set(welcome);                                                 // show something
		hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
		hm.pw.setMode(POWER_MODE_NO_SLEEP);                                 // set power management mode

		// register user modules
		cm_Switch[0].regInHM(1, 3, &hm);	                                // register user module
		cm_Switch[0].config(&initRly, &switchRly);                          // configure user module

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
