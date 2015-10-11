//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define EE_DBG
#include "EEprom.h"

uint8_t MAID[3];


// public:		//---------------------------------------------------------------------------------------------------------
uint8_t  EE::getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf) {
	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found

	if (!checkIndex(cnl, lst, idx)) return 0;

	getEEPromBlock(cnlTbl[xI].pAddr + (cnlTbl[xI].sLen * idx), cnlTbl[xI].sLen, buf);	// get the eeprom content
	return 1;
}
/**
 * @brief Write arbitrary data as list content to EEprom
 *
 * setList() can be used to set the values of all registers in a list (see @ref basics_device_registers).
 *
 * @param cnl Channel
 * @param lst List
 * @param idx Index of peer (0 if not applicable)
 * @param buf Array with all new list values (must match size of channel-list)
 *
 * @return 1 for success, 0 for failure
 *
 * The specified channel, list and index are used to identify the eeprom section to write to
 * (see @ref section_eeprom_memory_layout).
 *
 * If a valid eeprom section can be identified, the content of buf will be written to the associated
 * EEprom memory section.
 *
 * @note Use setListArray() to write to individual registers rather than the complete list.
 *
 * In a simple example, the channel slice address definition for List0 is:
 * @code
 * const uint8_t cnlAddr[] PROGMEM = {
 *     0x02, 0x05, 0x0a, 0x0b, 0x0c, 0x14, 0x24, 0x25, 0x26, 0x27, // sIdx 0x00, 10 bytes for Channel0/List0
 *     0x01,                                                       // sIdx 0x0a,  1 byte  for Channel1/List4
 * }; // 11 byte
 *
 * EE::s_cnlTbl cnlTbl[] = {
 *     // cnl, lst, sIdx, sLen, pAddr;
 *     {  0,  0, 0x00, 10,  0x000f }, // Channel0/List0, 10 bytes at sIdx 0x00, Addr 0x000f
 *     {  1,  4, 0x0a,  1,  0x0019 }, // Channel1/List4,  1 byte  at sIdx 0x0a, Addr 0x0019
 * };
 * @endcode
 * In order to write the contents of list0 (10 bytes) to the EEprom, we simply use:
 * @code
 *     const uint8_t list0defaults[] = {
 *         0x01,           //  0x02, led-mode on
 *         0x00,           //  0x05,
 *         0x00,0x00,0x00, //  0x0a, Master-ID, leave at 000000 to allow for pairing
 *         0x00,           //  0x14,
 *         0x02,0x01       //  0x24, measureInterval = 513sec = 0x0201
 *         0x03,           //  0x26, pwmStableDelay = 3sec = 0x03
 *         0x05            //  0x27, pwmStableDelta = 2.5% = 0x05
 *     };
 *     hm.ee.setListArray(0,0,0,sizeof(list0defaults),list0defaults);
 * @endcode
 * Note how registers with data types bigger than one byte use consecutive addresses for
 * the required number of bytes.
 *
 * Commonly, it is good practice to keep the declaration of the register contents close
 * to that of the channel slice address definition to simplify book-keeping during development.
 *
 * @see setListArray(), firstTimeStart()
 */
uint8_t  EE::setList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf) {
	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found

	if (!checkIndex(cnl, lst, idx)) return 0;

	setEEPromBlock(cnlTbl[xI].pAddr + (cnlTbl[xI].sLen * idx), cnlTbl[xI].sLen, buf);	// get the eeprom content
	return 1;
}
uint8_t  EE::getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found

	if (!checkIndex(cnl, lst, idx)) return 0;											// check if peer index is in range

	uint16_t eIdx = cnlTbl[xI].pAddr + (cnlTbl[xI].sLen * idx);

	uint8_t retByte;
	for (uint8_t j = 0; j < cnlTbl[xI].sLen; j++) {										// search for the right address in cnlAddr
		if (_pgmB(devDef.cnlAddr[cnlTbl[xI].sIdx + j]) == addr) {						// if byte fits
			getEEPromBlock(eIdx + j, 1, (void*)&retByte);								// get the respective byte from eeprom
			return retByte;																// and exit
		}
	}
}
uint32_t EE::getHMID(void) {
	uint8_t a[3];
	a[0] = HMID[2];
	a[1] = HMID[1];
	a[2] = HMID[0];
	a[3] = 0;

	return *(uint32_t*)&a;
}

// private:		//---------------------------------------------------------------------------------------------------------
EE::EE() {
}

// general functions
void     EE::init(void) {
	#ifdef EE_DBG																		// only if ee debug is set
	dbgStart();																			// serial setup
	dbg << F("EE.\n");																	// ...and some information
	#endif

	initEEProm();																		// init function if a i2c eeprom is used

	// check for first time run by checking magic byte, if yes then prepare eeprom and set magic byte
	uint16_t eepromCRC = 0, flashCRC = 0;												// define variable for storing crc
	uint8_t  *p = (uint8_t*)cnlTbl;														// cast devDef to char

	for (uint8_t i = 0; i < (devDef.lstNbr*6); i++) {									// step through all bytes of the channel table, one line has 6 byte
		flashCRC = crc16(flashCRC, p[i]);												// calculate the 16bit checksum for the table
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

		firstTimeStart();																// function to be placed in register.h, to setup default values on first time start
	}

	// load HMID and serial from eeprom
	if (*(uint16_t*)&HMID == NULL) getEEPromBlock(2, 3, HMID);							// check if HMID variable is set and valid, otherwise load from eeprom
	if (*(uint16_t*)&HMSR == NULL) getEEPromBlock(5, 10, HMSR);
	if (*(uint16_t*)&HMKEY == NULL) getEEPromBlock(15, 16, HMKEY);

	// load the master id
	getMasterID();

	everyTimeStart();																	// add this function in register.h to setup default values every start

}
void     EE::getMasterID(void) {
	MAID[0] = getRegAddr(0, 0, 0, 0x0a);
	MAID[1] = getRegAddr(0, 0, 0, 0x0b);
	MAID[2] = getRegAddr(0, 0, 0, 0x0c);
}
void     EE::testModul(void) {															// prints register.h content on console
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
uint8_t  EE::isHMIDValid(uint8_t *toID) {
	//dbg << "t: " << _HEX(toID, 3) << ", h: " << _HEX(HMID, 3) << '\n';
	return compArray(toID, HMID, 3);
}
uint8_t  EE::isPairValid (uint8_t *reID) {
	return compArray(reID, MAID, 3);
}
uint8_t  EE::isBroadCast(uint8_t *toID) {
	//uint8_t zero[3] = {0,0,0};
	//return mycmp(toID, zero, 3);
	return isEmpty(toID, 3);
}
uint8_t  EE::getIntend(uint8_t *reId, uint8_t *toId, uint8_t *peId) {
	if (isBroadCast(toId)) return 'b';													// broadcast message
	if (!isHMIDValid(toId)) return 'l';													// not for us, only show as log message

	if (isPairValid(reId)) return 'm';													// coming from master
	if (isPeerValid(peId)) return 'p';													// coming from a peer
	if (isHMIDValid(reId)) return 'i';													// we were the sender, internal message

	// now it could be a message from the master to us, but master is unknown because we are not paired
	if ((isHMIDValid(toId)) && isEmpty(MAID,3)) return 'x';
	return 'u';																			// should never happens
}

// peer functions
void     EE::clearPeers(void) {
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {										// step through all channels
		clearEEPromBlock(peerTbl[i].pAddr, peerTbl[i].pMax * 4);
		//dbg << F("clear eeprom, addr ") << peerTbl[i].pAddr << F(", len ") << (peerTbl[i].pMax * 4) << '\n';																	// ...and some information
	}
}
uint8_t  EE::isPeerValid (uint8_t *peer) {
	//dbg << "p: " << _HEX(peer, 4) << '\n';
	for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {										// step through all channels
		if (getIdxByPeer(i, peer) != 0xff) return i;									// if a valid peer is found return the respective channel
	}
	return 0;																			// otherwise 0
}

uint8_t  EE::countFreeSlots(uint8_t cnl) {
	uint8_t bCounter = 0;																// set counter to zero
	uint8_t lPeer[4];

	if ((!cnl) || (cnl > devDef.cnlNbr)) return 0;										// return if channel is out of range
	//dbg << F("cFS: ") << peerTbl[cnl-1].pMax << '\n';

	for (uint8_t i = 0; i < peerTbl[cnl-1].pMax; i++) {									// step through the possible peer slots
		getEEPromBlock(peerTbl[cnl-1].pAddr+(i*4), 4, lPeer);							// get peer from eeprom
		//if (!*(unsigned long*)lPeer) bCounter++;										// increase counter if peer slot is empty
		if (isEmpty(lPeer, 4)) bCounter++;												// increase counter if peer slot is empty
		//dbg << F("addr: ") << (peerTbl[cnl-1].pAddr+(i*4)) << F(", lPeer: ") << pHex(lPeer, 4) << '\n';
	}
	return bCounter;																	// return the counter
}
uint8_t  EE::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	uint8_t lPeer[4];

	if (!cnl) return 0;																	// on channel 0 there is no need to search
	if (cnl > devDef.cnlNbr) return 0xff;												// return if channel is out of range

	for (uint8_t i = 0; i < peerTbl[cnl-1].pMax; i++) {									// step through the possible peer slots
		getEEPromBlock(peerTbl[cnl-1].pAddr+(i*4), 4, lPeer);							// get peer from eeprom
		if (compArray(lPeer, peer, 4)) return i;										// if result matches then return slot index
		//dbg << i << ": " << _HEX(lPeer,4) << ", s: " << _HEX(peer, 4) << '\n';
	}
	return 0xff;
}
uint8_t  EE::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	getEEPromBlock(peerTbl[cnl-1].pAddr+(idx*4), 4, peer);
}
uint8_t  EE::addPeer(uint8_t cnl, uint8_t *peer) {
	uint8_t lPeer[4];
	uint8_t cnl1 = cnl-1;

	// check if channel exists
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	// check if one of the peers already exists
	if (getIdxByPeer(cnl, peer) != 0xff) peer[3] = 0;									// peer 1 exists, therefore write a 0 in the peer channel byte
	memcpy(lPeer, peer, 3); lPeer[3] = peer[4];											// prepare peer 2
	if (getIdxByPeer(cnl, lPeer) != 0xff) peer[4] = 0;									// peer 2 exists, therefore write a 0 in the peer channel byte

	// set bit mask against peer cnl
	uint8_t cnt = 0, ret = 0;
	if (peer[3]) cnt |= 1;
	if (peer[4]) cnt |= 2;

	// count free peer slots and check against cnt
	for (uint8_t i = 0; i < peerTbl[cnl1].pMax; i++) {									// step through the possible peer slots
		getEEPromBlock(peerTbl[cnl1].pAddr+(i*4), 4, lPeer);							// get peer from eeprom
		if (isEmpty(lPeer, 4)) ret++;													// increase counter if peer slot is empty
	}
	if (((peer[3]) && (peer[4])) && (ret < 2)) return 0;								// not enough space, return failure
	if (((peer[3]) || (peer[4])) && (ret < 1)) return 0;

	// search for free peer slots and write content
	for (uint8_t i = 0; i < peerTbl[cnl1].pMax; i++) {									// step through the possible peer slots
		getEEPromBlock(peerTbl[cnl1].pAddr+(i*4), 4, lPeer);							// get peer from eeprom

		if        (isEmpty(lPeer, 4) && (cnt & 1)) {									// slot is empty and peer cnlA is set
			cnt ^= 1;
			setEEPromBlock(peerTbl[cnl1].pAddr+(i*4), 4, peer);
			peer[5] = i;																// remember the idx position, add to the buffer

		} else if (isEmpty(lPeer, 4) && (cnt & 2)) {									// slot is empty and peer cnlB is set
			cnt ^= 2;
			setEEPromBlock(peerTbl[cnl1].pAddr+(i*4), 3, peer);						// first 3 bytes
			setEEPromBlock(peerTbl[cnl1].pAddr+(i*4)+3, 1, peer+4);					// 5th byte
			peer[6] = i;																// remember the idx position, add to the buffer

		}
	}
	return 1;																			// everything went fine, return success
}
uint8_t  EE::remPeer(uint8_t cnl, uint8_t *peer) {
	uint8_t tPeer[4], lPeer[4];

	// check if channel exists
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	// peerA is given by (uint32_t*)peer, peerB has to be constructed
	memcpy(tPeer, peer, 3);
	memcpy(tPeer+3, peer+4, 1);
	//dbg << "a: " << pHex(peer,4) << ", b: " << pHex(tPeer,4) << '\n';

	// search for peers and delete them
	for (uint8_t i = 0; i < peerTbl[cnl-1].pMax; i++) {									// step through the possible peer slots
		getEEPromBlock(peerTbl[cnl-1].pAddr+(i*4), 4, lPeer);							// get peer from eeprom

		if (compArray(lPeer, peer, 4) || compArray(lPeer ,tPeer, 4)) {					// check if something matches
			clearEEPromBlock(peerTbl[cnl-1].pAddr+(i*4), 4);							// free the slot
		}
	}
	return 1;
}
uint8_t  EE::countPeerSlc(uint8_t cnl) {
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	int16_t lTmp = peerTbl[cnl-1].pMax - countFreeSlots(cnl) + 1;						// get used slots and add one for terminating zeros
	lTmp = lTmp * 4;																	// 4 bytes per slot

	uint8_t bMax = 0;																	// size return value
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax;																		// return amount of slices
}
uint8_t  EE::getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf) {
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range

	uint8_t byteCnt = 0, slcCnt = 0;													// start the byte counter

	for (uint8_t i = 0; i < peerTbl[cnl-1].pMax; i++) {									// step through the possible peer slots

		getEEPromBlock(peerTbl[cnl-1].pAddr+(i*4), 4, buf);								// get peer from eeprom
		if (isEmpty(buf, 4)) continue;													// peer is empty therefor next

		byteCnt+=4;																		// if we are here, then it is valid and we should increase the byte counter
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

	memset(buf, 0, 4);																	// add the terminating zeros
	return byteCnt + 4;																	// return the amount of bytes
}
uint8_t  EE::getPeerSlots(uint8_t cnl) {													// returns the amount of possible peers
	if (cnl > devDef.cnlNbr) return 0;
	return peerTbl[cnl-1].pMax;															// return the max amount of peers
}

// register functions
void     EE::clearRegs(void) {
	uint8_t peerMax;

	for (uint8_t i = 0; i < devDef.lstNbr; i++) {										// steps through the cnlTbl

		if ((cnlTbl[i].lst == 3) || (cnlTbl[i].lst == 4)) {								// list3/4 is peer based
			peerMax = peerTbl[cnlTbl[i].cnl - 1].pMax;									// get the amount of maximum peers
		} else {
			peerMax = 1;																// otherwise no peer slots available
		}

		// calculate full length of peer indexed channels and clear the memory
		clearEEPromBlock(cnlTbl[i].pAddr, peerMax * cnlTbl[i].sLen);

		//dbg << i << ": " << peerMax << ", addr: " << cnlTbl[i].pAddr << ", len: " \
		//    << (peerMax * cnlTbl[i].sLen) << '\n';
	}
}
uint8_t  EE::countRegListSlc(uint8_t cnl, uint8_t lst) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found

	int16_t lTmp = cnlTbl[xI].sLen * 2;													// get the slice len and multiply by 2 because we need regs and content

	uint8_t bMax = 0;
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax+1;																		// return amount of slices
}
uint8_t  EE::getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found
	//dbg << "idx " << idx << " pT " << peerTbl[cnl-1].pMax << '\n';

	if (!checkIndex(cnl, lst, idx)) return 0;											// check if peer index is in range

	uint8_t slcOffset = slc * maxMsgLen;												// calculate the starting offset
	slcOffset /= 2;																		// divided by to because of mixed message, regs + eeprom content

	int8_t remByte = cnlTbl[xI].sLen - slcOffset;										// calculate the remaining bytes
	if (remByte <= 0) {																	// check if we are in the last slice and add terminating zeros
		*(uint16_t*)buf = 0;															// add them
		//dbg << slc << " " << slcOffset << " " << cnlTbl[xI].sLen << '\n';
		return 2;																		// nothing to do anymore
	}
	if (remByte >= (maxMsgLen/2)) remByte = (maxMsgLen/2);								// shorten remaining bytes if necessary

	uint8_t sIdx = cnlTbl[xI].sIdx;
	uint16_t eIdx = cnlTbl[xI].pAddr + (cnlTbl[xI].sLen * idx);
	//dbg << slc << ", sO:" << slcOffset << ", rB:" << remByte << ", sIdx:" << pHexB(sIdx) << ", eIdx:" << pHexB(eIdx) << '\n';

	for (uint8_t i = 0; i < remByte; i++) {												// count through the remaining bytes
		*buf++ = _pgmB(devDef.cnlAddr[i+sIdx+slcOffset]);								// add the register address
		getEEPromBlock(i+eIdx+slcOffset, 1, buf++);										// add the eeprom content
		//dbg << (i+eIdx+slcOffset) << '\n';
	}

	return remByte*2;																	// return the byte length
}

/**
 * @brief Set individual registers of a list.
 *
 * setListArray can be used to set indivual registers of a list (see @ref basics_device_registers).
 * Any number of registers defined for a list by the respective channel slice
 * address definition can we written with a new value.
 *
 * @param cnl Channel
 * @param lst List
 * @param idx Index of peer (0 if not applicable)
 * @param len Array length (must be a multiple of 2)
 * @param buf Array with pairs of REGNUM:VALUE
 *
 * @return 1 for success, 0 for failure
 *
 * The specified channel, list and index are used to identify the eeprom section to write to
 * (see @ref section_eeprom_memory_layout).
 *
 * If a valid eeprom section is identified, each REGNUM:VALUE pair in the array
 * for validity and write will be used to write VALUE as the new content of
 * register REGNUM in the specified List.
 *
 * @note Use setList() to write to the complete list rather than individual registers.
 *
 * In a simple example, the channel slice address definition for List0 is:
 * @include docs/snippets/register-h-cnlAddr.cpp docs/snippets/register-h-cnlTbl.cpp
 * @code
 * const uint8_t cnlAddr[] PROGMEM = {
 *     0x02, 0x05, 0x0a, 0x0b, 0x0c, 0x14, 0x24, 0x25, 0x26, 0x27, // sIdx 0x00, 10 bytes for Channel0/List0
 *     0x01,                                                       // sIdx 0x0a,  1 byte  for Channel1/List4
 * }; // 11 byte
 *
 * EE::s_cnlTbl cnlTbl[] = {
 *     // cnl, lst, sIdx, sLen, pAddr;
 *     {  0,  0, 0x00, 10,  0x000f }, // Channel0/List0, 10 bytes at sIdx 0x00, Addr 0x000f
 *     {  1,  4, 0x0a,  1,  0x0019 }, // Channel1/List4,  1 byte  at sIdx 0x0a, Addr 0x0019
 * };
 * @endcode
 * In order to write the contents for only a few of the registers of List0 to the
 * EEprom, we simply use:
 * @code
 *     const uint8_t list0defaults[] = {
 *         0x01, 0x01,  // led-mode on
 *         0x24, 0x02,  // measureInterval = 513sec = 0x0201
 *         0x25, 0x01,
 *         0x26, 0x03,  // pwmStableDelay = 3sec = 0x03
 *         0x27, 0x05   // pwmStableDelta = 2.5% = 0x05
 *     };
 *     hm.ee.setListArray(0,0,0,sizeof(list0defaults),list0defaults);
 * @endcode
 *
 * @todo Add references to related methods
 */
uint8_t  EE::setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf) {

	uint8_t xI = getRegListIdx(cnl, lst);
	if (xI == 0xff) return 0;															// respective line not found
	if ((cnl > 0) && (idx >=peerTbl[cnl-1].pMax)) return 0;								// check if peer index is in range

	uint16_t eIdx = cnlTbl[xI].pAddr + (cnlTbl[xI].sLen * idx);

	for (uint8_t i = 0; i < len; i+=2) {												// step through the input array

		for (uint8_t j = 0; j < cnlTbl[xI].sLen; j++) {									// search for the right address in cnlAddr
			if (_pgmB(devDef.cnlAddr[cnlTbl[xI].sIdx + j]) == buf[i]) {					// if byte fits
				setEEPromBlock(eIdx + j, 1, (void*)&buf[i+1]);									// add the eeprom content
				//dbg << "eI:" << pHexB(eIdx + j) << ", " << pHexB(buf[i+1]) << '\n';
				break;																	// go to the next i
			}
		}

	}
}
uint8_t  EE::getRegListIdx(uint8_t cnl, uint8_t lst) {
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {										// steps through the cnlTbl
		// check if we are in the right line by comparing channel and list, otherwise try next
		if ((cnlTbl[i].cnl == cnl) && (cnlTbl[i].lst == lst)) return i;
	}
	return 0xff;																		// respective line not found
}
uint8_t  EE::checkIndex(uint8_t cnl, uint8_t lst, uint8_t idx) {
	//dbg << "cnl: " << cnl << " lst: " << lst << " idx: " << idx << '\n';
	if ((cnl) && ((lst == 3) || (lst == 4)) && (idx >= peerTbl[cnl-1].pMax) ) return 0;
	return 1;
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
uint8_t  compArray(void *ptr1, void *ptr2, uint8_t len) {
	//while (len > 0) {
	//	len--;
	//	if (*((uint8_t*)ptr1+len) != *((uint8_t*)ptr2+len)) return 0;
	//}
	//return 1;
	return memcmp(ptr1, ptr2, len)?0:1;
}
uint8_t  isEmpty(void *ptr, uint8_t len) {
	while (len > 0) {
		len--;
		if (*((uint8_t*)ptr+len) != 0) return 0;
	}
	return 1;
}
