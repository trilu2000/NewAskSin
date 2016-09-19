//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------


#include "00_debug-flag.h"
#ifdef EE_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif

//#define EE_DBG
//#define EE_DBG_TEST
#include "EEprom.h"
#include "AS.h"

uint8_t *MAID;
uint8_t HMID[3];
uint8_t HMSR[10];
uint8_t HMKEY[16];
uint8_t hmKeyIndex[1];


// public:		//---------------------------------------------------------------------------------------------------------








// private:		//---------------------------------------------------------------------------------------------------------
EE::EE() {
}

// general functions
void     EE::init(void) {
	DBG( F("EE:init\n"));																// some debug





}

void     EE::initHMKEY(void) {
	memcpy_P(HMKEY, HMSerialData+13, 16);												// get default HMKEY
	setEEPromBlock(15, 16, HMKEY);														// store default HMKEY to EEprom
	memcpy_P(hmKeyIndex, HMSerialData+29, 1);											// get default HMKEY
	setEEPromBlock(14, 1, hmKeyIndex);													// get hmKeyIndex from EEprom
}


void     EE::testModul(void) {															// prints register.h content on console
	#ifdef EE_DBG_TEST																		// only if ee debug is set
	dbg << '\n' << pLine;
	dbg << F("register.h - lists\n");
	dbg << pLine;
	dbg << F("channels: ") << devDef.cnlNbr << F(", list obj: ") << devDef.lstNbr << '\n';
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {
		dbg << F("cnl: ") << _pgmB(devDef.cnlTbl[i].cnl) << F(", lst: ") << _pgmB(devDef.cnlTbl[i].lst) << F(", byte: ") << _pgmB(devDef.cnlTbl[i].sLen) << '\n';
	}
	dbg << pLine;
	dbg << F("peer database\n");
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {
		dbg << F("cnl: ") << _pgmB(devDef.peerTbl[i].cnl) << F(", peers: ") << _pgmB(devDef.peerTbl[i].pMax) << '\n';
	}

	dbg << pLine;
	dbg << "HMID: " << pHex(HMID,3) << ", serial: "; dbg.write(HMSR,10);dbg << '\n';

	dbg << pLine;
	dbg << F("get peer list by slice...\n");
	// prepare the peer list
	uint8_t xPeer[4];
	*(uint32_t*)xPeer = 0x00060700;
	addPeer(1, 0, xPeer);
	*(uint32_t*)xPeer = 0x00;
	addPeer(1, 1, xPeer);
	*(uint32_t*)xPeer = 0x02060802;
	addPeer(1, 2, xPeer);
	*(uint32_t*)xPeer = 0x03050403;
	addPeer(1, 3, xPeer);
	*(uint32_t*)xPeer = 0x04070604;
	addPeer(1, 4, xPeer);
	*(uint32_t*)xPeer = 0x05060105;
	addPeer(1, 5, xPeer);
	_delay_ms(100);

	// get the strings
	uint8_t aTst5[16], bReturn, bSlices, bFreeSlots, bTotalSlots;
	bTotalSlots = getPeerSlots(1);
	bFreeSlots = countFreeSlots(1);
	bSlices = countPeerSlc(1);

	dbg << F("total slots: ") << bTotalSlots << F(", free slots: ") << bFreeSlots << F(", slices: ") << bSlices << '\n';
	for (uint8_t i = 1; i <= bSlices; i++) {
		bReturn = getPeerListSlc(1, i, aTst5);
		dbg << "s" << i << ": " << bReturn << F(" byte, ") << pHex(aTst5,bReturn) << '\n';
	}

	dbg << pLine;
	dbg << F("is peer valid...\n");
	dbg << F("result 1: ") << isPeerValid(xPeer) << '\n';
	dbg << F("result 0: ") << isPeerValid(HMID) << '\n';

	dbg << pLine;
	dbg << F("is pair valid...\n");
	dbg << F("result 1: ") << isPairValid(HMID) << '\n';
	dbg << F("result 0: ") << isPairValid(xPeer) << '\n';

	dbg << pLine;
	dbg << F("test get index by peer...\n");
	*(uint32_t*)xPeer = 0x04070604;
	dbg << F("result 4: ") << getIdxByPeer(1,xPeer) << '\n';

	dbg << pLine;
	dbg << F("get peer by index...\n");
	*(uint32_t*)xPeer = 0x00;
	getPeerByIdx(1,3,xPeer);
	dbg << F("result 03040503: ") << pHex(xPeer,4) << '\n';


	dbg << pLine;
	dbg << F("clear, read, write regs...\n");
	clearRegs();
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {
		dbg << i << F(", cnl:") << _pgmB(devDef.cnlTbl[i].cnl) << F(", lst: ") << _pgmB(devDef.cnlTbl[i].lst) \
		    << F(", slc: ") <<  countRegListSlc(_pgmB(devDef.cnlTbl[i].cnl), _pgmB(devDef.cnlTbl[i].lst)) << '\n';
	}

	uint8_t aTst6[] = {0x04,0x44, 0x05,0x55, 0x06,0x66, 0x07,0x77, 0x08,0x88, 0x09,0x99, 0x0a,0xaa, 0x0b,0xbb,};
	uint32_t xtime = millis();
	setListArray(1, 3, 1, 16, aTst6);
	dbg << (millis()-xtime) << F(" ms\n");

	xtime = millis();
	bReturn = getRegListSlc(1, 3, 1, 0, aTst5);
	dbg << pHex(aTst5, bReturn) << '\n';
	bReturn = getRegListSlc(1, 3, 1, 1, aTst5);
	dbg << pHex(aTst5, bReturn) << '\n';
	bReturn = getRegListSlc(1, 3, 1, 2, aTst5);
	dbg << pHex(aTst5, bReturn) << '\n';
	dbg << (millis()-xtime) << F(" ms\n");

	dbg << pLine;
	dbg << F("search a reg...\n");
	dbg << F("result 77: ") << pHexB(getRegAddr(1, 3, 1, 0x07)) << '\n';

	#endif
}








