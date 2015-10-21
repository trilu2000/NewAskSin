#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load library's --------------------------------------------------------------------------------------------------------
	#include <AS.h>
	#include "hardware.h"
	#include <cmBlind.h>

	AS hm;																			// asksin framework
	cmBlind cmBlind[1];																// create 1 instances of channel module
	extern void initBlind(uint8_t channel);											// declare function to jump in
	extern void switchBlind(uint8_t channel, uint8_t status);						// declare function to jump in

	/*
	 * HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x01, 0x02, 0x06,
		/* Serial number */   'H', 'B', 'b', 'l', 'i', 'n', 'd', '0', '0', '1',		// HBblind001
		/* Default-Key */     0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
		/* Key-Index */       0x00
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
		/* modelID         2 byte */  0x00, 0x53,			// HM-LC-Bl1-PB-FM
		/* subTypeID       1 byte */  0x30,
		/* deviceInfo      3 byte */  0x01, 0x01, 0x00,
	};

	/*
	 * Register definitions
	 * The values are offset adresses in relation to the start adress defines in cnlTbl
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
		0x0b,0x0d,0x0f,0x10,0x08,

		// List3-Register
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0f,0x11,0x1c,0x1d,0x1e,0x1f,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8f,0x91,0x9c,0x9d,0x9e,0x9f,
	};

	/*
	 * Channel - List translation table
	 * channel, list, startIndex, start address in EEprom, hidden
	 */
	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{  0,    0,   0x00, 6,    0x001f, 0, },
		{  1,    1,   0x06, 5,    0x0025, 0, },
		{  1,    3,   0x0a, 38,   0x002A, 0, },
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
		hm.confButton.config(1, CONFIG_KEY_PCIE, CONFIG_KEY_INT);					// configure the config button, mode, pci byte and pci bit

		hm.ld.init(2, &hm);															// configure the led
		hm.ld.set(welcome);															// show a "hello" at the led

		// hm.bt.set(30, 3600000);													// set battery check, internal, 2.7 reference, measurement each hour
		hm.pw.setMode(0);															// set power management mode

		// register user modules
		cmBlind[0].regInHM(1, 3, &hm);												// register user module
		cmBlind[0].config(&initBlind, &switchBlind);								// configure user module
	}

	/**
	 * TODO: maybe we can delete this?
	 */
	void firstTimeStart(void) {
	}


	/**
	 * TODO: Describe
	 */
	struct s_lst0Cnl0 {
	// 0x02, 0x0a, 0x0b, 0x0c,
		uint8_t                           :7;       // 0x02, s:0, e:7
		uint8_t INTERNAL_KEYS_VISIBLE     :1;       // 0x02, s:7, e:8
		uint8_t Master_ID_A               :8;       // 0x0a, s:0, e:8
		uint8_t Master_ID_B               :8;       // 0x0b, s:0, e:8
		uint8_t Master_ID_C               :8;       // 0x0c, s:0, e:8
	};  // 4 byte

	/**
	 * TODO: Describe
	 */
	struct s_lst1Cnl1 {
	 // 0x0b, 0x0d, 0x0f, 0x10,
		uint8_t REFERENCE_RUNNING_TIME_TOP_BOTTOM :16;       // 0x0b, s:0, e:16
		uint8_t REFERENCE_RUNNING_TIME_BOTTOM_TOP :16;       // 0x0d, s:0, e:16
		uint8_t CHANGE_OVER_DELAY         :8;       // 0x0f, s:0, e:8
		uint8_t REFERENCE_RUN_COUNTER     :8;       // 0x10, s:0, e:8
	};  // 4 byte

	/**
	 * TODO: Describe
	 */
	struct s_lst3Cnl1 {
	// 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x11, 0x1c, 0x1d, 0x1e, 0x1f, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8f, 0x91, 0x9c, 0x9d, 0x9e, 0x9f,
	    uint8_t SHORT_CT_RAMPON           :4;       // 0x01, s:0, e:4
	    uint8_t SHORT_CT_RAMPOFF          :4;       // 0x01, s:4, e:8
	    uint8_t SHORT_CT_ONDELAY          :4;       // 0x02, s:0, e:4
	    uint8_t SHORT_CT_OFFDELAY         :4;       // 0x02, s:4, e:8
	    uint8_t SHORT_CT_ON               :4;       // 0x03, s:0, e:4
	    uint8_t SHORT_CT_OFF              :4;       // 0x03, s:4, e:8
	    uint8_t SHORT_COND_VALUE_LO       :8;       // 0x04, s:0, e:8
	    uint8_t SHORT_COND_VALUE_HI       :8;       // 0x05, s:0, e:8
	    uint8_t SHORT_ONDELAY_TIME        :8;       // 0x06, s:0, e:8
	    uint8_t SHORT_ON_TIME             :8;       // 0x07, s:0, e:8
	    uint8_t SHORT_OFFDELAY_TIME       :8;       // 0x08, s:0, e:8
	    uint8_t SHORT_OFF_TIME            :8;       // 0x09, s:0, e:8
	    uint8_t SHORT_ACTION_TYPE         :2;       // 0x0a, s:0, e:2
	    uint8_t                           :4;       // 0x0a, s:2, e:6
	    uint8_t SHORT_OFF_TIME_MODE       :1;       // 0x0a, s:6, e:7
	    uint8_t SHORT_ON_TIME_MODE        :1;       // 0x0a, s:7, e:8
	    uint8_t SHORT_JT_ON               :4;       // 0x0b, s:0, e:4
	    uint8_t SHORT_JT_OFF              :4;       // 0x0b, s:4, e:8
	    uint8_t SHORT_JT_ONDELAY          :4;       // 0x0c, s:0, e:4
	    uint8_t SHORT_JT_OFFDELAY         :4;       // 0x0c, s:4, e:8
	    uint8_t SHORT_JT_RAMPON           :4;       // 0x0d, s:0, e:4
	    uint8_t SHORT_JT_RAMPOFF          :4;       // 0x0d, s:4, e:8
	    uint8_t SHORT_OFF_LEVEL           :8;       // 0x0f, s:0, e:8
	    uint8_t SHORT_ON_LEVEL            :8;       // 0x11, s:0, e:8
	    uint8_t SHORT_CT_REFON            :4;       // 0x1c, s:0, e:4
	    uint8_t SHORT_CT_REFOFF           :4;       // 0x1c, s:4, e:8
	    uint8_t SHORT_MAX_TIME_FIRST_DIR  :8;       // 0x1d, s:0, e:8
	    uint8_t SHORT_JT_REFON            :4;       // 0x1e, s:0, e:4
	    uint8_t SHORT_JT_REFOFF           :4;       // 0x1e, s:4, e:8
	    uint8_t SHORT_DRIVING_MODE        :8;       // 0x1f, s:0, e:8
	    uint8_t LONG_CT_RAMPON            :4;       // 0x81, s:0, e:4
	    uint8_t LONG_CT_RAMPOFF           :4;       // 0x81, s:4, e:8
	    uint8_t LONG_CT_ONDELAY           :4;       // 0x82, s:0, e:4
	    uint8_t LONG_CT_OFFDELAY          :4;       // 0x82, s:4, e:8
	    uint8_t LONG_CT_ON                :4;       // 0x83, s:0, e:4
	    uint8_t LONG_CT_OFF               :4;       // 0x83, s:4, e:8
	    uint8_t LONG_COND_VALUE_LO        :8;       // 0x84, s:0, e:8
	    uint8_t LONG_COND_VALUE_HI        :8;       // 0x85, s:0, e:8
	    uint8_t LONG_ONDELAY_TIME         :8;       // 0x86, s:0, e:8
	    uint8_t LONG_ON_TIME              :8;       // 0x87, s:0, e:8
	    uint8_t LONG_OFFDELAY_TIME        :8;       // 0x88, s:0, e:8
	    uint8_t LONG_OFF_TIME             :8;       // 0x89, s:0, e:8
	    uint8_t LONG_ACTION_TYPE          :2;       // 0x8a, s:0, e:2
	    uint8_t                           :3;       // 0x8a, s:2, e:5
	    uint8_t LONG_MULTIEXECUTE         :1;       // 0x8a, s:5, e:6
	    uint8_t LONG_OFF_TIME_MODE        :1;       // 0x8a, s:6, e:7
	    uint8_t LONG_ON_TIME_MODE         :1;       // 0x8a, s:7, e:8
	    uint8_t LONG_JT_ON                :4;       // 0x8b, s:0, e:4
	    uint8_t LONG_JT_OFF               :4;       // 0x8b, s:4, e:8
	    uint8_t LONG_JT_ONDELAY           :4;       // 0x8c, s:0, e:4
	    uint8_t LONG_JT_OFFDELAY          :4;       // 0x8c, s:4, e:8
	    uint8_t LONG_JT_RAMPON            :4;       // 0x8d, s:0, e:4
	    uint8_t LONG_JT_RAMPOFF           :4;       // 0x8d, s:4, e:8
	    uint8_t LONG_OFF_LEVEL            :8;       // 0x8f, s:0, e:8
	    uint8_t LONG_ON_LEVEL             :8;       // 0x91, s:0, e:8
	    uint8_t LONG_CT_REFON             :4;       // 0x9c, s:0, e:4
	    uint8_t LONG_CT_REFOFF            :4;       // 0x9c, s:4, e:8
	    uint8_t LONG_MAX_TIME_FIRST_DIR   :8;       // 0x9d, s:0, e:8
	    uint8_t LONG_JT_REFON             :4;       // 0x9e, s:0, e:4
	    uint8_t LONG_JT_REFOFF            :4;       // 0x9e, s:4, e:8
	    uint8_t LONG_DRIVING_MODE         :8;       // 0x9f, s:0, e:8
	};  // 38 byte
#endif
