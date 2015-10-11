//- ----------------------------------------------------------------------------------------------------------------------
//- load libraries -------------------------------------------------------------------------------------------------------
#include <AS.h>                                                         // the asksin framework
#include "hardware.h"                                                   // hardware definition
#include <cmSwitch.h>

//- stage modules --------------------------------------------------------------------------------------------------------
AS hm;                                                                  // asksin framework

cmSwitch cmSwitch[1];                                                 // create instances of channel module
extern void initRly(uint8_t channel);                                   // declare function to jump in
extern void switchRly(uint8_t channel, uint8_t status);                 // declare function to jump in

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0x58,0x23,0xff,};
uint8_t  EEMEM eHMSR[10] = {'X','M','S','1','2','3','4','5','6','7',};
uint8_t  EEMEM eHMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,};

// if HMID and Serial are not set, then eeprom ones will be used
uint8_t HMID[3] = {0x58,0x23,0xff,};
uint8_t HMSR[10] = {'X','M','S','1','2','3','4','5','6','7',};          // XMS1234567
uint8_t HMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,};

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version  1 byte */  0x10,                               // don't know for what it is good for
	/* Model ID          2 byte */  0x00,0x6c,                          // model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID       1 byte */  0x00,                               // not needed for FHEM, it's something like a group ID
	/* Device Info       3 byte */  0x41,0x01,0x00,                     // describes device, not completely clear yet. includes amount of channels
};  // 7 byte

//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
	0x02,0x0a,0x0b,0x0c,0x12,0x18,
	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
};  // 28 byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx, sLen, pAddr, hidden
	{ 0, 0, 0x00,  6, 0x000f, 0, },
	{ 1, 1, 0x00,  0, 0x0000, 0, },
	{ 1, 3, 0x06, 22, 0x0015, 0, },
};  // 21 byte

//- peer device list table -----------------------------------------------------------------------------------------------
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{ 1, 6, 0x0099, },
};  // 4 byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	1, 3, devIdnt, cnlAddr,
};  // 6 byte

//- module registrar -----------------------------------------------------------------------------------------------------
RG::s_modTable modTbl[1];

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
    cmSwitch[0].regInHM(1, 3, &hm);                                    // register user module
    cmSwitch[0].config(&initRly, &switchRly);                          // configure user module

}

void firstTimeStart(void) {
	// place here everything which should be done on the first start or after a complete reset of the sketch
	// typical use case are default values which should be written into the register or peer database

}
