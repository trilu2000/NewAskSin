//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define EE_DBG
#include "EE.h"

// public:		//---------------------------------------------------------------------------------------------------------

EE::EE() {
} 

void EE::init(void) {
	#ifdef EE_DBG																		// only if ee debug is set
	dbg.begin(57600);																	// serial setup
	dbg << F("\n....\n");																// ...and some information
	dbg << F("EE.\n");																	// ...and some information
	#endif

	// check for first time run by checking magic byte, if yes then prepare eeprom and set magic byte
	uint16_t eepromCRC = 0, flashCRC = 0;												// define variable for storing crc
	uint8_t  *p = (uint8_t*)devDef.cnlTbl;												// cast devDef to char
	
	for (uint8_t i = 0; i < (devDef.lstNbr*6); i++) {									// step through all bytes of the channel table, one line has 6 byte
		flashCRC = crc16(flashCRC, _pgmB(&p[i]));										// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eepromCRC);												// get magic byte from eeprom
	
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("crc, flash: ") << flashCRC << F(", eeprom: ") << eepromCRC << '\n';		// ...and some information
	#endif
	
	if(flashCRC!=eepromCRC) {															// first time detected, format eeprom, load defaults and write magic byte
		// formating eeprom
		clearPeers();

		// store defaults, incl HMID and serial
		//for (uint8_t i = 0; i < eepDef.nbr; i++) {
		//	dbg << F("addr: ") << _pgmB(&eepDef.eepDefTbl[i].addr) << F(", len: ") << _pgmB(&eepDef.eepDefTbl[i].len) << '\n';//<< F(", byte: ") << _pgmB(&devDef.cnlTbl[i].sLen) << '\n';
		//}
		

		// write magic byte
		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("writing magic byte\n");												// ...and some information
		#endif
		setEEPromBlock(0,2,(void*)&flashCRC);											// write magic byte to eeprom
	}

	// load hmid and serial from eeprom
	getEEPromBlock(2,3,(void*)&HMID);
	getEEPromBlock(5,10,(void*)&HMSR);
	

}
void EE::testModul(void) {																// prints register.h content on console
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << '\n' << pLine;
	dbg << F("register.h - lists\n");
	dbg << pLine;
	dbg << F("channels: ") << devDef.cnlNbr << F(", list obj: ") << devDef.lstNbr << '\n';
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {
		dbg << F("cnl: ") << _pgmB(&devDef.cnlTbl[i].cnl) << F(", lst: ") << _pgmB(&devDef.cnlTbl[i].lst) << F(", byte: ") << _pgmB(&devDef.cnlTbl[i].sLen) << '\n';
	}
	dbg << pLine;
	dbg << F("peer database\n");
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {
		dbg << F("cnl: ") << _pgmB(&devDef.peerTbl[i].cnl) << F(", peers: ") << _pgmB(&devDef.peerTbl[i].pMax) << '\n';
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

	#endif
}
uint8_t EE::isPairValid (uint8_t *pair) {
	if (pair[0] != HMID[0]) return 0;													// compares byte by byte
	if (pair[1] != HMID[1]) return 0;
	if (pair[2] != HMID[2]) return 0;
	return 1;																			// all bytes are fitting, therefore return true
}

void    EE::clearPeers(void) {
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {										// step through all channels
		clearEEPromBlock(_pgmB(&devDef.peerTbl[i].pAddr), _pgmB(&devDef.peerTbl[i].pMax) * 4);
		//dbg << F("clear eeprom, addr ") << _pgmB(&devDef.peerTbl[i].pAddr) << F(", len ") << (_pgmB(&devDef.peerTbl[i].pMax) * 4) << '\n';																	// ...and some information
	}
}
uint8_t EE::isPeerValid (uint8_t *peer) {
	for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {										// step through all channels
		if (getIdxByPeer(i, peer) != 0xff) return 1;									// if found a valid peer is found return 1
	}
	return 0;																			// otherwise 0
}

uint8_t EE::countFreeSlots(uint8_t cnl) {
	uint8_t bCounter = 0;																// set counter to zero
	uint32_t lPeer;
	
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range
	//dbg << F("cFS: ") << _pgmB(&devDef.peerTbl[cnl-1].pMax) << '\n';
	
	for (uint8_t i = 0; i < _pgmB(&devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);	// get peer from eeprom
		if (lPeer == 0) bCounter++;														// increase counter if peer slot is empty
		//dbg << F("addr: ") << (_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4)) << F(", lPeer: ") << pHex(((uint8_t*)&lPeer),4) << '\n';
	}
	return bCounter;																	// return the counter
}
uint8_t EE::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	uint32_t lPeer;
	for (uint8_t i = 0; i < _pgmB(&devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);	// get peer from eeprom
		if (lPeer == *(uint32_t*)peer) return i;										// if result matches then return slot index
		//dbg << i << ": " << lPeer << ", s: " << (*(uint32_t*)peer) << '\n';
	}
	return 0xff;
}
uint8_t EE::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4, (void*)peer);
}
uint8_t EE::addPeer(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	setEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4, (void*)peer);
}
uint8_t EE::remPeer(uint8_t cnl, uint8_t idx) {
	clearEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4);
}
uint8_t EE::countPeerSlc(uint8_t cnl) {
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	int16_t lTmp = _pgmB(&devDef.peerTbl[cnl-1].pMax) - countFreeSlots(cnl) + 1;		// get used slots and add one for terminating zeros
	lTmp = lTmp * 4;																	// 4 bytes per slot
	
	uint8_t bMax = 0;																	// size return value
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax;																		// return amount of slices
}
uint8_t EE::getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf) {
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range
	
	uint8_t byteCnt = 0, slcCnt = 1;													// start the byte counter
	
	for (uint8_t i = 0; i < _pgmB(&devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)buf);		// get peer from eeprom
		if (*(uint32_t*)buf == 0) continue;												// peer is empty therefor next
		byteCnt+=4;																		// increase the byte counter
		//dbg << i << ": " << pHex(buf,4) << ", bC: " << byteCnt  << ", sC: " << slcCnt << '\n';

		if ((slcCnt == slc) && (byteCnt >= maxMsgLen)) {								// we are in the right slice but string is full
			return byteCnt;																// return the amount of bytes in the string
			
		} else if (byteCnt >= maxMsgLen) {												// only counter is full
			slcCnt++;																	// increase the slice counter
			byteCnt = 0;																// and reset the byte counter

		} else if (slcCnt == slc) {														// we are in the fitting slice
			buf+=4;																		// therefore we should increase the string counter
		}
	}

	*(uint32_t*)buf = 0;																// add the terminating zeros
	return byteCnt + 4;																	// return the amount of bytes
}


// private:		//---------------------------------------------------------------------------------------------------------
uint8_t EE::getPeerSlots(uint8_t cnl) {													// returns the amount of possible peers
	if (cnl > devDef.cnlNbr) return 0;
	return _pgmB(&devDef.peerTbl[cnl-1].pMax);											// return the max amount of peers
}



//- some helpers ----------------------------------------------------------------------------------------------------------
uint16_t crc16(uint16_t crc, uint8_t a) {
	uint16_t i;

	crc ^= a;
	for (i = 0; i < 8; ++i) {
		if (crc & 1)
		crc = (crc >> 1) ^ 0xA001;
		else
		crc = (crc >> 1);
	}
	return crc;
}