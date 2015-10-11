#include <AS.h>

//  "003D" => {name=>"HM-WDS10-TH-O"           ,st=>'THSensor'          ,cyc=>'00:10' ,rxt=>'c:f'    ,lst=>'p'            ,chn=>"",}, #:w  todo should be wakeup, does not react

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0x01,0x02,0x04};
uint8_t  EEMEM eHMSR[10] = {'t','l','u','2','0','0','1','2','3','4'};

// if HMID and Serial are not set, then eeprom ones will be used
uint8_t HMID[3] = {0x02,0x02,0x06};
uint8_t HMSR[10] = {'t','l','u','2','0','0','1','2','3','6'}; // tlu1001236

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version 1 byte */  0x10,									// don't know for what it is good for
	/* Model ID	        2 byte */  0x00, 0x3d,								// model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID      1 byte */  0x70,									// not needed for FHEM, it's something like a group ID
	/* Device Info      3 byte */  0x03, 0x01, 0x00							// describes device, not completely clear yet. includes amount of channels
};


//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
	0x01,0x02,0x0a,0x0b,0x0c,
	0x01,
}; // 6 byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx, sLen, pAddr;
	{0, 0, 0x00,  5, 0x000f},
	{1, 4, 0x05,  1, 0x0014},   //  1 *  6 =   6 (0x0006)
}; // 12 byte

//- peer device list table -----------------------------------------------------------------------------------------------
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{1, 6, 0x001a}              //  6 * 4 =  24 (0x18)
}; // 4 byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	1, 2, devIdnt, cnlAddr,
}; // 6 byte

//- module registrar -----------------------------------------------------------------------------------------------------
RG::s_modTable modTbl[1];

void everyTimeStart(void) {
	// place here everything which should be done on each start or reset of the device
	// typical use case are loading default values or user class configurations


}

void firstTimeStart(void) {
	// place here everything which should be done on the first start or after a complete reset of the sketch
	// typical use case are default values which should be written into the register or peer database


}
