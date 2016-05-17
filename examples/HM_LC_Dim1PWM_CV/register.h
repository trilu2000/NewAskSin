
//- ----------------------------------------------------------------------------------------------------------------------
//- load libraries -------------------------------------------------------------------------------------------------------
#include <AS.h>                                                         // the asksin framework
#include "hardware.h"                                                   // hardware definition
#include <cmDimmer.h>
#include "hmkey.h"

//- stage modules --------------------------------------------------------------------------------------------------------
AS hm;                                                                  // asksin framework

cmDimmer cm_Dimmer[3];												    // create instances of channel module
extern void initDim(uint8_t channel);                                   // declare function to jump in
extern void switchDim(uint8_t channel, uint8_t status, uint8_t characteristic);// declare function to jump in
uint8_t bTemperature;

/*
* HMID, Serial number, HM-Default-Key, Key-Index
*/
const uint8_t HMSerialData[] PROGMEM = {
	/* HMID */            0x01, 0x02, 0x07,
	/* Serial number */   'H', 'B', 'd', 'i', 'm', 'm', 'e', 'r', '0', '1',		// HBdimmer01
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
	/* firmwareVersion 1 byte */  0x25,
	/* modelID         2 byte */  0x00,0x67,
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
	// List1-Register
	0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
	// List3-Register
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
};  // 74 byte

/*
* Channel - List translation table
* channel, list, startIndex, start address in EEprom, hidden
*/
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx,  sLen, pAddr,  hidden
	{ 0, 0, 0x00,  6, 0x001f, 0, },
	{ 1, 1, 0x06,  8, 0x0025, 0, },
	{ 1, 3, 0x0e, 60, 0x002d, 0, }, // 60 byte * 6 peers = 360 / 0x168
	{ 2, 1, 0x06,  8, 0x0195, 0, },
	{ 2, 3, 0x0e, 60, 0x019d, 0, }, // 60 byte * 1 peers = 60 / 0x3c
	{ 3, 1, 0x06,  8, 0x01d9, 0, },
	{ 3, 3, 0x0e, 60, 0x01e1, 0, }, // 60 byte * 1 peers = 60 / 0x3c

};  // 49 byte

	/*
	* Peer-Device-List-Table
	* channel, maximum allowed peers, start address in EEprom
	*/
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{ 1, 6, 0x021d, }, // 6 peers + 4 byte = 24 byte / 0x18 
	{ 2, 1, 0x0235, },
	{ 3, 1, 0x0239, },
};

/*
* handover to AskSin lib
*
* TODO: Describe
*/
EE::s_devDef devDef = {
	3, 7, devIdnt, cnlAddr,
};

/*
* module registrar
*
* TODO: Describe
*/
RG::s_modTable modTbl[3];


/**
* @brief Regular start function
* This function is called by the main function every time when the device starts,
* here we can setup everything which is needed for a proper device operation
*/
void everyTimeStart(void) {
	// place here everything which should be done on each start or reset of the device
	// typical use case are loading default values or user class configurations

	// init the homematic framework
	hm.confButton.config(2);                                            // configure the config button mode
	hm.ld.set(welcome);                                                 // show something
	hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
	hm.pw.setMode(POWER_MODE_NO_SLEEP);                                 // set power management mode

	// register user modules
	cm_Dimmer[0].regInHM(1, 3, &hm);                                     // register user module
	cm_Dimmer[0].config(&initDim, &switchDim, &bTemperature);            // configure user module

	cm_Dimmer[1].regInHM(2, 3, &hm);                                     // register user module
	cm_Dimmer[1].config(&initDim, &switchDim, &bTemperature);            // configure user module

	cm_Dimmer[2].regInHM(3, 3, &hm);                                     // register user module
	cm_Dimmer[2].config(&initDim, &switchDim, &bTemperature);            // configure user module
}

void firstTimeStart(void) {
	// place here everything which should be done on the first start or after a complete reset of the sketch
	// typical use case are default values which should be written into the register or peer database

	//uint8_t xT[] = {0x15, 0xff, 0x16, 0xff};
	//hm.ee.setListArray(0, 0, 0, 4, xT);

	//uint8_t pT[] = {0x01,0x02,0x04,0x01,0x00};
	//hm.ee.addPeer(1, pT);
	//hm.ee.addPeer(2, pT);
	//hm.ee.addPeer(3, pT);

}