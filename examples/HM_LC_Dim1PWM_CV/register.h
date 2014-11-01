#include <AS.h>

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0x01,0x02,0x04};
uint8_t  EEMEM eHMSR[10] = {'t','l','u','1','0','0','1','2','3','4'};

// if HMID and Serial are not set, then eeprom ones will be used
uint8_t HMID[3] = {0x01,0x02,0x04};
uint8_t HMSR[10] = {'t','l','u','1','0','0','1','2','3','4'}; // tlu1001234

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version 1 byte */  0x22,									// don't know for what it is good for
	/* Model ID	        2 byte */  0x00, 0x67,								// model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID      1 byte */  0x20,									// not needed for FHEM, it's something like a group ID
	/* Device Info      3 byte */  0x41, 0x01, 0x00							// describes device, not completely clear yet. includes amount of channels
};


//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
    0x02,0x0a,0x0b,0x0c,0x15,0x18,
    0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
};  // 72 byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx, sLen, pAddr;
	{0, 0, 0x00, 6,  0x000F},
	{1, 1, 0x06, 8,  0x0015},
	{1, 3, 0x0e, 58, 0x001d}, // 58*6=348 (15c)
	{2, 1, 0x06, 8,  0x0179},
	{2, 3, 0x0e, 58, 0x0181}, // 58*1=58 (3a)
	{3, 1, 0x06, 8,  0x01bb},
	{3, 3, 0x0e, 58, 0x01c3}, // 58*1=58 (3a)
};  // byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{1, 6, 0x01fd}, // 6*4=24 (18) 
	{2, 1, 0x0215}, // 1*4=4 (4)
	{3, 1, 0x0219}, // 1*4=4 (4)
};  // byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	3, 7, devIdnt, cnlAddr,
};  // 10 byte

//- module registrar -----------------------------------------------------------------------------------------------------
RG::s_modTable modTbl[3];
