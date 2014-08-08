#include "AS.h"

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0x01,0x02,0x04};
uint8_t  EEMEM eHMSR[10] = {'t','l','u','1','0','0','1','2','3','4'};

// if HMID and Serial are not set, then eeprom ones will be used
uint8_t HMID[3] = {0x01,0x02,0x04};
uint8_t HMSR[10] = {'t','l','u','1','0','0','1','2','3','4'};

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version 1 byte */  0x15,									// don't know for what it is good for
	/* Model ID	        2 byte */  0x00, 0x11,								// model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID      1 byte */  0x10,									// not needed for FHEM, it's something like a group ID
	/* Device Info      3 byte */  0x01, 0x01, 0x00							// describes device, not completely clear yet. includes amount of channels
};


//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
	0x02,0x05,0x0a,0x0b,0x0c,0x12,
	0x08,
	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
};  // 29 byte

//- channel device list table --------------------------------------------------------------------------------------------
const s_cnlTbl cnlTbl[] PROGMEM = {
	// cnl, lst, sIdx, sLen, pAddr;
	{0, 0, 0x00, 6, 0x000F},
	{1, 1, 0x06, 1, 0x0015},
	{1, 3, 0x07, 22, 0x0016},
};  // 18 byte

//- channel device list table --------------------------------------------------------------------------------------------
const s_peerTbl peerTbl[] PROGMEM = {
	// cnl, pMax, pAddr;
	{1, 6, 0x009A}
};  // 4 byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	1, 3, devIdnt, cnlAddr, cnlTbl, peerTbl,
};  // 10 byte

