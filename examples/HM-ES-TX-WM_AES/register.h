#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load library's --------------------------------------------------------------------------------------------------------
	#include "hardware.h"
	#include <cmPowerSens.h>
	#include "hmkey.h"

	AS hm;																			// asksin framework
	cmPowerSens cmPowerSens[1];																// create 1 instances of channel module
	extern void initPowerSens(uint8_t channel);											// declare function to jump in

	/*
	 * HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x01, 0x02, 0x06,
		/* Serial number */   'H', 'B', 'p', 'w', 'r', 's', 'e', '0', '0', '1',		// HBpwrse001
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
    /* Firmware version 1 byte */  0x01,
    /* Model ID         2 byte */  0x00, 0xde,
    /* Sub Type ID      1 byte */  0x00,
    /* Device Info      3 byte */  0x41, 0x01, 0x00, 
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
		// channel: 0, list: 0
		0x02,0x0a,0x0b,0x0c,0x12,0x18,
		// channel: 1, list: 1
		0x08,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,
	 }; // 15 byte
/*	const uint8_t cnlAddr[] PROGMEM = {
		// List0-Register
		0x02,0x0a,0x0b,0x0c,0x12,0x15,0x18,

		// List1-Register
		0x08,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x30,0x57,

		// List3-Register
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0f,0x11,0x1c,0x1d,0x1e,0x1f,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8f,0x91,0x9c,0x9d,0x9e,0x9f,
	};*/

	/*
	 * Channel - List translation table
	 * channel, list, startIndex, start address in EEprom, hidden
	 */
/*	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{  0,    0,   0x00, 7,    0x001F, 0, },
		{  1,    1,   0x07, 9,    0x0028, 0, },
		{  1,    3,   0x10, 38,   0x0036, 0, },
	};*/
	   EE::s_cnlTbl cnlTbl[] = {
	      // cnl, lst, sIdx, sLen, pAddr,  hidden
	      {  0,   0,   0x00,  6,   0x0020, 0, },
	      {  1,   1,   0x06,  9,   0x0026, 0, },
	   }; // 14 byte

	/*
	 * Peer-Device-List-Table
	 * channel, maximum allowed peers, start address in EEprom
	 */
	   EE::s_peerTbl peerTbl[] = {
	       // cnl, pMax, pAddr;
		   { 1, 2, 0x020d, },
	    }; // 0 byte

	/*
	 * handover to AskSin lib
	 *
	 * TODO: Describe
	 */
	   EE::s_devDef devDef = {
	      1, 2, devIdnt, cnlAddr,
	   }; // 6 byte

	/*
	 * module registrar
	 *
	 * TODO: Describe
	 */
	RG::s_modTable modTbl[1];
	/**
	   * @brief First time and regular start functions
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
	      //hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
	      hm.pw.setMode(0);                                                   // set power management mode

	      // register user modules
			cmPowerSens[0].regInHM(1, 2, &hm);												// register user module
			cmPowerSens[0].config(&initPowerSens);
	   }

	   void firstTimeStart(void) {
	      /*
	      * place here everything which should be done on the first start or after a complete reset of the sketch
	      * typical usecase are default values which should be written into the register or peer database
	      */

	      const uint8_t cnl0lst0[] = {
	         0x80,0x00,0x00,0x00,0x69,0x00,
	      };

	      const uint8_t cnl1lst1[] = {
	         0x00,0x03,0x64,0x00,0x0a,0x00,0x10,0x27,0x00,
	      };

	   }
#endif
