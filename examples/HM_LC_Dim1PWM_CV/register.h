
//- ----------------------------------------------------------------------------------------------------------------------
//- load libraries -------------------------------------------------------------------------------------------------------
#include <AS.h>                                                         // the asksin framework
#include "hardware.h"                                                   // hardware definition
#include <cmDimmer.h>

//- stage modules --------------------------------------------------------------------------------------------------------
AS hm;                                                                  // asksin framework

cmDimmer cmDimmer[3];												    // create instances of channel module
extern void initDim(uint8_t channel);                                   // declare function to jump in
extern void switchDim(uint8_t channel, uint8_t status, uint8_t characteristic);// declare function to jump in
uint8_t bTemperature;

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0xac,0xaf,0xff,};
uint8_t  EEMEM eHMSR[10] = {'X','M','S','2','3','4','5','6','7','8',};
uint8_t  EEMEM eHMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,};

// if HMID and Serial are not set, then eeprom ones will be used
uint8_t HMID[3] = {0xac,0xaf,0xff,};
uint8_t HMSR[10] = {'X','M','S','2','3','4','5','6','7','8',};          // XMS2345678
uint8_t HMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,}; 

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version  1 byte */  0x25,                               // don't know for what it is good for
	/* Model ID          2 byte */  0x00,0x67,                          // model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID       1 byte */  0x00,                               // not needed for FHEM, it's something like a group ID
	/* Device Info       3 byte */  0x41,0x01,0x00,                     // describes device, not completely clear yet. includes amount of channels
};  // 7 byte

//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
	0x02,0x0a,0x0b,0x0c,0x12,0x18,
	0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
};  // 74 byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx, sLen, pAddr, hidden
	{ 0, 0, 0x00,  6, 0x000f, 0, },
	{ 1, 1, 0x06,  8, 0x0015, 0, },
	{ 1, 3, 0x0e, 60, 0x001d, 0, },
	{ 2, 1, 0x06,  8, 0x0185, 0, },
	{ 2, 3, 0x0e, 60, 0x018d, 0, },
	{ 3, 1, 0x06,  8, 0x01c9, 0, },
	{ 3, 3, 0x0e, 60, 0x01d1, 0, },
};  // 49 byte

//- peer device list table -----------------------------------------------------------------------------------------------
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{ 1, 6, 0x020d, },
	{ 2, 1, 0x0225, },
	{ 3, 1, 0x0229, },
};  // 12 byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	3, 7, devIdnt, cnlAddr,
};  // 6 byte

//- module registrar -----------------------------------------------------------------------------------------------------
RG::s_modTable modTbl[3];

//- ----------------------------------------------------------------------------------------------------------------------
//- first time and regular start functions -------------------------------------------------------------------------------

void everyTimeStart(void) {
	// place here everything which should be done on each start or reset of the device
	// typical use case are loading default values or user class configurations

	// init the homematic framework
	hm.confButton.config(2, CONFIG_KEY_PCIE, CONFIG_KEY_INT);           // configure the config button, mode, pci byte and pci bit
	hm.ld.init(2, &hm);                                                 // set the led
	hm.ld.set(welcome);                                                 // show something
	hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
	hm.pw.setMode(0);                                                   // set power management mode

	// register user modules
	cmDimmer[1].regInHM(1, 3, &hm);                                     // register user module
	cmDimmer[1].config(&initDim, &switchDim, &bTemperature);            // configure user module

	cmDimmer[0].regInHM(2, 3, &hm);                                     // register user module
	cmDimmer[0].config(&initDim, &switchDim, &bTemperature);            // configure user module

	cmDimmer[2].regInHM(3, 3, &hm);                                     // register user module
	cmDimmer[2].config(&initDim, &switchDim, &bTemperature);            // configure user module

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