//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define EE_DBG
#include "EE.h"

// public:		//---------------------------------------------------------------------------------------------------------
EE::EE() {
} 

// general functions
void    EE::init(void) {
	#ifdef EE_DBG																		// only if ee debug is set
	dbg.begin(57600);																	// serial setup
	dbg << F("\n....\n");																// ...and some information
	dbg << F("EE.\n");																	// ...and some information
	#endif

	initEEProm();																		// init function if a i2c eeprom is used
	
	// check for first time run by checking magic byte, if yes then prepare eeprom and set magic byte
	uint16_t eepromCRC = 0, flashCRC = 0;												// define variable for storing crc
	uint8_t  *p = (uint8_t*)devDef.cnlTbl;												// cast devDef to char
	
	for (uint8_t i = 0; i < (devDef.lstNbr*6); i++) {									// step through all bytes of the channel table, one line has 6 byte
		flashCRC = crc16(flashCRC, _pgmB(p[i]));										// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eepromCRC);												// get magic byte from eeprom
	
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("crc, flash: ") << flashCRC << F(", eeprom: ") << eepromCRC << '\n';		// ...and some information
	#endif
	
	if(flashCRC!=eepromCRC) {															// first time detected, format eeprom, load defaults and write magic byte
		// formating eeprom
		clearPeers();
		clearRegs();
		
		// write magic byte
		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("writing magic byte\n");												// ...and some information
		#endif
		setEEPromBlock(0,2,(void*)&flashCRC);											// write magic byte to eeprom
	}

	// load HMID and serial from eeprom
	if (*(uint16_t*)&HMID == NULL) getEEPromBlock(2,3,(void*)&HMID);
	if (*(uint16_t*)&HMSR == NULL) getEEPromBlock(2,3,(void*)&HMSR);
	
	// load the master id
	getMasterID();	

}
void    EE::getMasterID(void) {
	MAID[0] = getRegAddr(0, 0, 0, 0x0a);
	MAID[1] = getRegAddr(0, 0, 0, 0x0b);
	MAID[2] = getRegAddr(0, 0, 0, 0x0c);
}
void    EE::testModul(void) {															// prints register.h content on console
	#ifdef EE_DBG																		// only if ee debug is set
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
uint8_t EE::isHMIDValid(uint8_t *toID) {
	return memcmp(toID, HMID, 3)?0:1;
}
uint8_t EE::isPairValid (uint8_t *reID) {
	return memcmp(reID, MAID, 3)?0:1;
}
uint8_t EE::isBroadCast(uint8_t *toID) {
	uint8_t zero[3] = {0,0,0};
	return memcmp(toID, zero, 3)?0:1;
}
uint8_t EE::getIntend(uint8_t *reId, uint8_t *toId) {
	if (isBroadCast(toId)) return 'b';													// broadcast message
	if (!isHMIDValid(toId)) return 'l';													// not for us, only show as log message

	if (isPairValid(reId)) return 'm';													// coming from master
	if (isPeerValid(reId)) return 'p';													// coming from a peer
	return 'u';																			// should never happens
}


// peer functions 
void    EE::clearPeers(void) {
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {										// step through all channels
		clearEEPromBlock(_pgmW(devDef.peerTbl[i].pAddr), _pgmB(devDef.peerTbl[i].pMax) * 4);
		//dbg << F("clear eeprom, addr ") << _pgmW(devDef.peerTbl[i].pAddr) << F(", len ") << (_pgmB(devDef.peerTbl[i].pMax) * 4) << '\n';																	// ...and some information
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
	//dbg << F("cFS: ") << _pgmB(devDef.peerTbl[cnl-1].pMax) << '\n';
	
	for (uint8_t i = 0; i < _pgmB(devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);		// get peer from eeprom
		//getPeerByIdx(cnl, i, (uint8_t*)&lPeer);											// get peer from eeprom
		if (lPeer == 0) bCounter++;														// increase counter if peer slot is empty
		//dbg << F("addr: ") << (_pgmB(devDef.peerTbl[cnl-1].pAddr)+(i*4)) << F(", lPeer: ") << pHex(((uint8_t*)&lPeer),4) << '\n';
	}
	return bCounter;																	// return the counter
}
uint8_t EE::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	uint32_t lPeer;
	for (uint8_t i = 0; i < _pgmB(devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);		// get peer from eeprom
		if (lPeer == *(uint32_t*)peer) return i;										// if result matches then return slot index
		//dbg << i << ": " << lPeer << ", s: " << (*(uint32_t*)peer) << '\n';
	}
	return 0xff;
}
uint8_t EE::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4, (void*)peer);
}
uint8_t EE::addPeer(uint8_t cnl, uint8_t *peer) {
	uint32_t lPeer;

	// set bit mask against peer cnl
	uint8_t cnt = 0, ret = 0;
	if (peer[3]) cnt |= 1;
	if (peer[4]) cnt |= 2;

	// count free peer slots and check against cnt
	for (uint8_t i = 0; i < _pgmB(devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);		// get peer from eeprom
		if (lPeer == 0) ret++;															// increase counter if peer slot is empty
	}
	if (((peer[3]) && (peer[4])) && (ret < 2)) return 0;
	if (((peer[3]) || (peer[4])) && (ret < 1)) return 0;
	
	// search for free peer slots and write content
	for (uint8_t i = 0; i < _pgmB(devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);		// get peer from eeprom

		if        ((lPeer == 0) && (cnt & 1)) {											// slot is empty and peer cnlA is set
			cnt ^= 1;
			setEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)peer);

		} else if ((lPeer == 0) && (cnt & 2)) {											// slot is empty and peer cnlB is set
			cnt ^= 2;
			setEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 3, (void*)peer);	// first 3 bytes
			setEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4)+3, 1, (void*)peer+4);// 5th byte
		
		}
	}
	return 1;
}
uint8_t EE::remPeer(uint8_t cnl, uint8_t idx) {
	clearEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4);
}
uint8_t EE::countPeerSlc(uint8_t cnl) {
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	int16_t lTmp = _pgmB(devDef.peerTbl[cnl-1].pMax) - countFreeSlots(cnl) + 1;			// get used slots and add one for terminating zeros
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
	
	uint8_t byteCnt = 0, slcCnt = 0;													// start the byte counter
	
	for (uint8_t i = 0; i < _pgmB(devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots

		getEEPromBlock(_pgmW(devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)buf);		// get peer from eeprom
		if (*(uint32_t*)buf == 0) continue;												// peer is empty therefor next

		byteCnt+=4;																		// if we are here, then it is valid and we should increase the byte counter
		dbg << i << ": " << pHex(buf,4) << ", bC: " << byteCnt  << ", sC: " << slcCnt << '\n';

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
uint8_t EE::getPeerSlots(uint8_t cnl) {													// returns the amount of possible peers
	if (cnl > devDef.cnlNbr) return 0;
	return _pgmB(devDef.peerTbl[cnl-1].pMax);											// return the max amount of peers
}

// register functions
void    EE::clearRegs(void) {
	uint8_t peerMax;
	
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {										// steps through the cnlTbl

		if ((_pgmB(devDef.cnlTbl[i].lst) == 3) || (_pgmB(devDef.cnlTbl[i].lst) == 4)) {	// list3/4 is peer based
			peerMax = getPeerSlots(_pgmB(devDef.cnlTbl[i].cnl));						// get the amount of maximum peers
		} else {
			peerMax = 1;																// otherwise no peer slots available
		}

		// calculate full length of peer indexed channels and clear the memory
		clearEEPromBlock(_pgmW(devDef.cnlTbl[i].pAddr), (peerMax * _pgmB(devDef.cnlTbl[i].sLen)));

		//dbg << i << ": " << peerMax << ", addr: " << _pgmW(devDef.cnlTbl[i].pAddr) << ", len: " \
		//    << (peerMax * _pgmB(devDef.cnlTbl[i].sLen)) << '\n';
	}
}
uint8_t EE::countRegListSlc(uint8_t cnl, uint8_t lst) {
	
	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found

	int16_t lTmp = _pgmB(devDef.cnlTbl[xI].sLen) * 2;									// get the slice len and multiply by 2 because we need regs and content
	lTmp += 2;																			// add 2 terminating bytes
		
	uint8_t bMax = 0;
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax;																		// return amount of slices
}
uint8_t EE::getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf) {
	
	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found
	if (idx >= _pgmB(devDef.peerTbl[cnl-1].pMax)) return 0;								// check if peer index is in range
	
	uint8_t slcOffset = slc * maxMsgLen;												// calculate the starting offset	
	slcOffset /= 2;																		// divided by to because of mixed message, regs + eeprom content
	uint8_t remByte = _pgmB(devDef.cnlTbl[xI].sLen)	- slcOffset;						// calculate the remaining bytes
	if (remByte >= (maxMsgLen/2)) remByte = (maxMsgLen/2);								// shorten remaining bytes if necessary
	
	uint8_t sIdx = _pgmB(devDef.cnlTbl[xI].sIdx);
	uint16_t eIdx = _pgmB(devDef.cnlTbl[xI].pAddr) + (_pgmB(devDef.cnlTbl[xI].sLen) * idx);
	//dbg << slc << ", sO:" << slcOffset << ", rB:" << remByte << ", sIdx:" << pHexB(sIdx) << ", eIdx:" << pHexB(eIdx) << '\n';
	
	for (uint8_t i = 0; i < remByte; i++) {												// count through the remaining bytes
		*buf++ = _pgmB(devDef.cnlAddr[i+sIdx+slcOffset]);								// add the register address
		getEEPromBlock(i+eIdx+slcOffset, 1, (void*)buf++);								// add the eeprom content
	}

	if ((remByte*2) < maxMsgLen) {														// if there is space for the terminating zeros
		*(uint16_t*)buf = 0;															// add them
		remByte++;																		// and increase byte counter
	}
	
	return remByte*2;																	// return the byte length
}
uint8_t EE::getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found
	if (idx >= _pgmB(devDef.peerTbl[cnl-1].pMax)) return 0;								// check if peer index is in range

	uint16_t eIdx = _pgmB(devDef.cnlTbl[xI].pAddr) + (_pgmB(devDef.cnlTbl[xI].sLen) * idx);
	
	uint8_t retByte;
	for (uint8_t j = 0; j < _pgmB(devDef.cnlTbl[xI].sLen); j++) {						// search for the right address in cnlAddr
		if (_pgmB(devDef.cnlAddr[_pgmB(devDef.cnlTbl[xI].sIdx) + j]) == addr) {			// if byte fits
			getEEPromBlock(eIdx + j, 1, (void*)&retByte);								// get the respective byte from eeprom
			return retByte;																// and exit
		}
	}
}
uint8_t EE::setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found
	if (idx >= _pgmB(devDef.peerTbl[cnl-1].pMax)) return 0;								// check if peer index is in range

	uint16_t eIdx = _pgmB(devDef.cnlTbl[xI].pAddr) + (_pgmB(devDef.cnlTbl[xI].sLen) * idx);

	for (uint8_t i = 0; i < len; i+=2) {												// step through the input array	
		
		for (uint8_t j = 0; j < _pgmB(devDef.cnlTbl[xI].sLen); j++) {					// search for the right address in cnlAddr
			if (_pgmB(devDef.cnlAddr[_pgmB(devDef.cnlTbl[xI].sIdx) + j]) == buf[i]) {	// if byte fits
				setEEPromBlock(eIdx + j, 1, (void*)&buf[i+1]);							// add the eeprom content
				//dbg << "eI:" << pHexB(eIdx + j) << ", " << pHexB(buf[i+1]) << '\n';
				break;																	// go to the next i
			}
		}
		
	}
}

// private:		//---------------------------------------------------------------------------------------------------------
uint8_t EE::getRegListIdx(uint8_t cnl, uint8_t lst) {
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {										// steps through the cnlTbl
		// check if we are in the right line by comparing channel and list, otherwise try next
		if ((_pgmB(devDef.cnlTbl[i].cnl) == cnl) && (_pgmB(devDef.cnlTbl[i].lst) == lst)) return i;
	}
	return 0xff;																		// respective line not found
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