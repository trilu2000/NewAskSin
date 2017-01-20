#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load library's --------------------------------------------------------------------------------------------------------
	#include "hardware.h"
	#include "cmMyBlind.h"
	#include "hmkey.h"

	#define EEVARS_EEPROM_ADDR    0x03FB

	struct s_eeVars {
		uint8_t initialPos;														// initialPos:         byte 1020 in eeprom
		uint8_t motorLastDirection;												// motorLastDirection: byte 1021 in eeprom
		int16_t travelCount;													// travelCount:        bytes 1022-1023 in eeprom
	} eeVars;

	AS hm;																			// asksin framework
	cmMyBlind cmMyBlind[1];															// create 1 instances of channel module
	extern void initBlind(uint8_t channel);											// declare function to jump in
	extern void blindUpdateState(uint8_t channel, uint8_t state, uint32_t rrttb);	// declare function to jump in

	/*
	 * HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x01, 0x02, 0x06,
		/* Serial number */   'H', 'B', 'b', 'l', 'i', 'n', 'd', '0', '0', '1',		// HBblind001

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
		/* firmwareVersion 1 byte */  0x24,
		/* modelID         2 byte */  0x00, 0x53,									// HM-LC-Bl1-PB-FM
		/* subTypeID       1 byte */  0x30,
		/* deviceInfo      3 byte */  0x01, 0x01, 0x00,
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
		0x02,0x0a,0x0b,0x0c,0x12,0x15,0x18,

		// List1-Register
		0x08,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x30,0x57,

		// List3-Register
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0f,0x11,0x1c,0x1d,0x1e,0x1f,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8f,0x91,0x9c,0x9d,0x9e,0x9f,
	};

	/*
	 * Channel - List translation table
	 * channel, list, startIndex, start address in EEprom, hidden
	 */
	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{  0,    0,   0x00, 7,    0x001F, 0, },
		{  1,    1,   0x07, 9,    0x0028, 0, },
		{  1,    3,   0x10, 38,   0x0036, 0, },
	};

	/*
	 * Peer-Device-List-Table
	 * channel, maximum allowed peers, start address in EEprom
	 */
	EE::s_peerTbl peerTbl[] = {
		// cnl, peerMax, pAddr;
		{  1,   6,       0x010d, },
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
	 * @brief First time and regular start functions
	 */
	void everyTimeStart(void) {
		/*
		 * Place here everything which should be done on each start or reset of the device.
		 * Typical use case are loading default values or user class configurations.
		 */

		// init the homematic framework
		hm.confButton.config(2, CONFIG_KEY_PCIE, CONFIG_KEY_INT);					// configure the config button, mode, pci byte and pci bit

		hm.ld.init(2, &hm);															// configure the led
		hm.ld.set(welcome);															// show a "hello" at the led

		// hm.bt.set(30, 3600000);													// set battery check, internal, 2.7 reference, measurement each hour
		hm.pw.setMode(POWER_MODE_NO_SLEEP);											// set power management mode

		// register user modules
		cmMyBlind[0].regInHM(1, 3, &hm);											// register user module
		cmMyBlind[0].config(&initBlind, &blindUpdateState, eeVars.initialPos);		// configure user module
	}

	/**
	 * TODO: maybe we can delete this?
	 */
	void firstTimeStart(void) {
	}
#endif
