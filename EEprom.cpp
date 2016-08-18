//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define EE_DBG
//#define EE_DBG_TEST
#include "EEprom.h"
#include "AS.h"

uint8_t MAID[3];
uint8_t HMID[3];
uint8_t HMSR[10];
uint8_t HMKEY[16];
uint8_t hmKeyIndex[1];

// public:		//---------------------------------------------------------------------------------------------------------
/**
* @brief Read a complete channel/list from EEprom
*
* getList() can be used to read a complete list from eeprom into a byte array (see @ref basics_device_registers).
*
* @param cnl Channel
* @param lst List
* @param idx Index of peer (0 if not applicable)
* @param buf Array with all new list values (must match size of channel-list)
*
* @return 1 for success, 0 for failure
*
* The specified channel, list and index are used to identify the eeprom section to read from
* (see @ref section_eeprom_memory_layout).
*/
uint8_t  EE::getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf) {

	if (idx > peerTbl[cnl].pMax) return 0;												// peerdb index out of range
	s_cnlTbl *pCnlTbl = getRegListStruct(cnl, lst);										// get the corresponding line in cnlTbl
	if (!pCnlTbl) return 0;																// respective line in cnlTbl not found

	getEEPromBlock( pCnlTbl->pAddr + ( pCnlTbl->sLen * idx), pCnlTbl->sLen, buf);		// get the eeprom content
	return 1;																			// return success
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

	s_cnlTbl *cnlTblPtr = getRegListStruct(cnl, lst);
	return setList(cnlTblPtr, idx, buf);

	//uint8_t xI = getRegListIdx(cnl, lst);												// get the respective line in cnlTbl
	//dbg << F("setList, cnl:") << cnl << F(", lst:") << lst << F(", idx:") << idx << F(", cnlTbl:") << xI << '\n';
	//return setList(xI, idx, buf);												// call setlist shortend
}
uint8_t  EE::setList(s_cnlTbl* cnlTblPtr, uint8_t idx, uint8_t *buf) {

	if (!cnlTblPtr) return 0;
	//dbg << F("setListByTblIdx, cnlTblIdx:") << cnlTblIdx << F(", idx:") << idx << '\n';

	if (idx > peerTbl[cnlTblPtr->cnl ].pMax) return 0;									// index out of range

	uint16_t pAddr = cnlTblPtr->pAddr + (cnlTblPtr->sLen * idx);						// calculate the address
	setEEPromBlock(pAddr, cnlTblPtr->sLen, buf);										// set the eeprom content
	//uint16_t pAddr = cnlTbl[cnlTblIdx].pAddr + (cnlTbl[cnlTblIdx].sLen * idx);			// calculate the address
	//setEEPromBlock(pAddr, cnlTbl[cnlTblIdx].sLen, buf);									// set the eeprom content

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("setListByTblIdx, cnl:") << cnlTbl[cnlTblIdx].cnl << F(", lst:") << cnlTbl[cnlTblIdx].lst << F(", idx:") << idx << F(", tblRow:") << cnlTblIdx << F(", addr:") << pAddr << '\n';
	dbg << F("data:") << _HEX(buf, cnlTbl[cnlTblIdx].sLen) << '\n';
	#endif
	return 1;																			// report everything ok
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

	uint8_t xI = getRegListIdx(cnl, lst);												// get the respective line in cnlTbl
	//dbg << F("setListArray, cnl:") << cnl << F(", lst:") << lst << F(", idx:") << idx << F(", cnlTbl:") << xI << '\n';
	return setListArray(xI, idx, len, buf);												// call setlist shortend
}
uint8_t  EE::setListArray(uint8_t cnlTblIdx, uint8_t idx, uint8_t len, uint8_t *buf) {

	//dbg << F("setListArray, cnlTblIdx:") << cnlTblIdx << F(", idx:") << idx << '\n';

	if (idx > peerTbl[ cnlTbl[cnlTblIdx].cnl ].pMax) return 0;							// index out of range
	if (cnlTblIdx >= cnl_tbl_max) return 0;												// respective line not found

	uint16_t eIdx = cnlTbl[cnlTblIdx].pAddr + (cnlTbl[cnlTblIdx].sLen * idx);

	for (uint8_t i = 0; i < len; i += 2) {												// step through the input array
		for (uint8_t j = 0; j < cnlTbl[cnlTblIdx].sLen; j++) {							// search for the matching address in cnlAddr
			if (_PGM_BYTE( cnlAddr[ cnlTbl[cnlTblIdx].sIdx + j ]) == buf[i] ) {			// if byte found
				setEEPromBlock(eIdx + j, 1, (void*)&buf[i + 1]);						// add the eeprom content
				// dbg << "eI:" << _HEXB(eIdx + j) << ", " << _HEXB(buf[i+1]) << '\n';
				break;																	// go to the next i
			}
		}
	}
}

/**
 * @brief Get register value from EEprom
 *
 * @param cnl Channel
 * @param lst List
 * @param idx Index of peer (0 if not applicable)
 * @param addr address of the register (@see xml device file)
 *
 * TODO: maybe we return 0xFF if value not found?
 * @return the value or 0 if not found
 */
uint8_t  EE::getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr) {
	uint8_t retByte;
	//uint8_t sIdx = getRegListIdx(cnl, lst);												// the register list index

	//if (sIdx > cnl_tbl_max) {
	//	#ifdef EE_DBG
	//	dbg << F("EE: sIdx ") << _HEXB(sIdx) << F (" not found\n");
	//	#endif
	//	return 0x00;																	// given list not found
	//}

	//if (!checkIndex(cnl, lst, idx)) {
	//	#ifdef EE_DBG
	//	dbg << F("EE: idx ") << _HEXB(idx) << F (" out of range\n");
	//	#endif
	//	return 0x00;																	// check if peer index is in range
	//}

	if (idx > peerTbl[cnl].pMax) return 0;												// peerdb index out of range
	s_cnlTbl *cnlTblPtr = getRegListStruct(cnl, lst);
	if (!cnlTblPtr) return 0;

	uint16_t eIdx = cnlTblPtr->pAddr + (cnlTblPtr->sLen * idx);
	//uint16_t eIdx = cnlTbl[sIdx].pAddr + (cnlTbl[sIdx].sLen * idx);

	for (uint8_t i = 0; i < cnlTblPtr->sLen; i++) {										// search for the right address in cnlAddr
	//for (uint8_t i = 0; i < cnlTbl[sIdx].sLen; i++) {									// search for the right address in cnlAddr
		if (_PGM_BYTE(cnlAddr[cnlTblPtr->sIdx] + i) == addr) {							// if byte fits
	//	if (_PGM_BYTE(cnlAddr[cnlTbl[sIdx].sIdx] + i) == addr) {						// if byte fits
			getEEPromBlock(eIdx + i, 1, (void*)&retByte);								// get the respective byte from eeprom
			return retByte;																// and exit
		}
	}

	#ifdef EE_DBG
	dbg << F("EE: address ") << _HEXB(addr) << (" not fount in list ") << _HEXB(lst) << (" for channel ") << _HEXB(cnl) << F("\n");
	#endif

	return 0;
}



// private:		//---------------------------------------------------------------------------------------------------------
EE::EE() {
}

// general functions
void     EE::init(void) {
	#ifdef EE_DBG																		// only if ee debug is set
		dbgStart();																			// serial setup
		dbg << F("EE.\n");																// ...and some information
	#endif

	initEEProm();																		// init function if a i2c eeprom is used

	// check for first time run by checking magic byte, if yes then prepare eeprom and set magic byte
	uint16_t eepromCRC = 0;																// eMagicByte in EEprom
	uint16_t flashCRC = 0;																// define variable for storing crc
	uint8_t  *p = (uint8_t*)cnlTbl;														// cast devDef to char

	for (uint8_t i = 0; i < ( cnl_tbl_max * sizeof(s_cnlTbl)); i++) {					// step through all bytes of the channel table
		flashCRC = crc16(flashCRC, p[i]);												// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eepromCRC);												// get magic byte from eeprom

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("crc, flash: ") << flashCRC << F(", eeprom: ") << eepromCRC << '\n';		// ...and some information
	#endif

	if(flashCRC != eepromCRC) {															// first time detected, format eeprom, load defaults and write magicByte
		// write magic byte
		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("writing magic byte\n");												// ...and some information
		#endif
		setEEPromBlock(0, 2, (void*)&flashCRC);											// write eMagicByte to eeprom

		firstTimeStart();																// function to be placed in register.h, to setup default values on first time start
	}

	getEEPromBlock(15, 16, HMKEY);														// get HMKEY from EEprom
	getEEPromBlock(14, 1, hmKeyIndex);													// get hmKeyIndex from EEprom
	if (HMKEY[0] == 0x00) {																// if HMKEY in EEPROM invalid
		initHMKEY();
	}

	// load the master id
	getMasterID();

	everyTimeStart();																	// add this function in register.h to setup default values every start
}

void     EE::initHMKEY(void) {
	memcpy_P(HMKEY, HMSerialData+13, 16);												// get default HMKEY
	EE:setEEPromBlock(15, 16, HMKEY);													// store default HMKEY to EEprom
	memcpy_P(hmKeyIndex, HMSerialData+29, 1);												// get default HMKEY
	setEEPromBlock(14, 1, hmKeyIndex);													// get hmKeyIndex from EEprom
}

void     EE::getMasterID(void) {
	MAID[0] = getRegAddr(0, 0, 0, 0x0a);
	MAID[1] = getRegAddr(0, 0, 0, 0x0b);
	MAID[2] = getRegAddr(0, 0, 0, 0x0c);
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
inline uint8_t  EE::isHMIDValid(uint8_t *toID) {
	//dbg << "t: " << _HEX(toID, 3) << ", h: " << _HEX(HMID, 3) << '\n';
	return (!memcmp(toID, HMID, 3));
}
inline uint8_t  EE::isPairValid (uint8_t *reID) {
	return (!memcmp(reID, MAID, 3));
}
inline uint8_t  EE::isBroadCast(uint8_t *toID) {
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
/**
* @brief Clears the complete defined peer database in eeprom
*        by writing 0's. Used on first time start or device reset 
*        to defaults. 
*
*/
void     EE::clearPeers(void) {
	for (uint8_t i = 0; i <= cnl_max; i++) {											// step through all channels
		EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[i];								// pointer to the peer table line

		//setEEPromBlock(peerTbl[i].pAddr, peerTbl[i].pMax * 4, EMPTY4BYTE);
		clearEEPromBlock( pPeerTbl->pAddr, pPeerTbl->pMax * 4);

		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("clearPeers, addr:") << pPeerTbl->pAddr << F(", len ") << (pPeerTbl->pMax * 4) << '\n';																	// ...and some information
		#endif
	}
}

/**
* @brief Returns the channel where the peer was found in the peer database
*
* @param peer Pointer to byte array containing the peer/peer channel
*
* @return the channel were the peer was found, 0 if not
*/
uint8_t  EE::isPeerValid (uint8_t *peer) {
	uint8_t retByte = 0;

	for (uint8_t i = 1; i <= cnl_max; i++) {											// step through all channels
		if (getIdxByPeer(i, peer) != 0xff) retByte = i;									// if a valid peer is found return the respective channel
	}

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("isPeerValid:") << _HEX(peer, 4) << F(", ret:") << retByte << '\n';
	#endif
	return retByte;																		// otherwise 0
}

/**
* @brief Returns the amount of free peer slots of the peer database 
*        of the given channel.
*
* @param cnl Channel
*
* @return the amount of free slots
*/
uint8_t  EE::countFreeSlots(uint8_t cnl) {
	uint8_t lPeer[4];																	// temp byte array to load peer addresses
	uint8_t bCounter = 0;																// set counter to zero
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line

	for (uint8_t i = 0; i < pPeerTbl->pMax; i++) {										// step through the possible peer slots
		getPeerByIdx(cnl, i, lPeer);													// get peer from eeprom
		if (isEmpty(lPeer, 4)) bCounter++;												// increase counter if peer slot is empty
	}

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("countFreeSlots, cnl:") << cnl << F(", total:") << peerTbl[cnl].pMax << F(", free:") << bCounter << '\n';
	#endif
	return bCounter;																	// return the counter
}

/**
* @brief Search for a given peer/peer channel in the peer database and return the index 
*
* @param cnl Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to search for the index
*
* @return the found idx or 0xff if not found
*/
uint8_t  EE::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	uint8_t lPeer[4];																	// byte array to load peer from eeprom
	uint8_t retByte = 0xff;																// default return value
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line

	for (uint8_t i = 0; i < pPeerTbl->pMax; i++) {										// step through the possible peer slots
		getPeerByIdx(cnl, i, lPeer);													// get peer from eeprom
		//dbg << "ee:" << _HEX(lPeer, 4) << ", gv:" << _HEX(peer, 4) << "\n";
		if (!memcmp(lPeer, peer, 4)) { retByte = i; break; };							// if result matches then return slot index
	}

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("getIdxByPeer, cnl:") << cnl << F(", peer:") << _HEX(peer, 4) << F(", ret:") << retByte << '\n';
	#endif
	return retByte;
}

/**
* @brief Search for a given peer/peer channel in the peer database and return the index
*
* @param cnl Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to search for the index
*
* @return the found idx or 0xff if not found
*/
void     EE::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line
	getEEPromBlock( pPeerTbl->pAddr + (idx * 4), 4, peer);								// get the respective eeprom block
	#ifdef EE_DBG																		// only if ee debug is set
	//dbg << F("getPeerByIdx, cnl:") << cnl << F(", idx:") << idx << F(", peer:") << _HEX(peer, 4) << '\n';
	#endif
}

/**
* @brief Add 2 peers incl peer channels to the peer database by the given channel
*        and returns a byte array with the two peer index numbers for further processing
*
* @param cnl Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to search for the index
*             { 01, 02, 03, 04, 05 } 01 - 03 peer address, 04 peer channel 1, 05 peer channel 2
* @param retIdx Pointer to a 2 byte array, function writes the index of the added peers incl. 
*               peer channel into the array
*
* @return the amount of added peers
*/
uint8_t  EE::addPeers(uint8_t cnl, uint8_t *peer) {
	uint8_t retByte = 0;
	RG::s_modTable *pModTbl = &modTbl[cnl];												// pointer to the respective line in the module table
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line
	EE::s_cnlTbl   *pCnlTbl  = (s_cnlTbl*)&cnlTbl[ pPeerTbl->pLink ];					// easier to work with a pointer to the channel table

	// remove peers if exist and check if we have enough free slots
	remPeers(cnl, peer);																// first we remove the probably existing entries in the peer database
	uint8_t needSlots = 0; if (peer[3]) needSlots++; if (peer[4]) needSlots++;			// count the needed slots
	uint8_t freeSlots = countFreeSlots(cnl);											// get the amount of free slots
	if (needSlots > freeSlots) return retByte;											// check if we have enough space

	// find the free slots and write peers - set the respective list 3/4 also 
	for (uint8_t i = 0; i < 2; i++) {													// standard gives 2 peer channels
		if (!peer[3+i]) continue;														// if the current peer channel is empty, go to the next entry 

		// find an empty slot
		uint8_t free_idx = getIdxByPeer(cnl, EMPTY4BYTE);								// get the index of the free slot
		if ( free_idx >= pPeerTbl->pMax) return retByte;								// no free index found, probably not needed while checked the free slots before
		
		// write the peer - takes 12 to 14ms
		uint16_t peerAddr = pPeerTbl->pAddr + (free_idx * 4);							// calculate the peer address in eeprom
		setEEPromBlock(peerAddr, 3, peer);												// write the peer to the eeprom
		setEEPromBlock(peerAddr +3, 1, peer+3+i);										// write the peer channel to the eeprom

		/* 
		* writing defaults take a long time, in case of cmSwitch it is 75ms peer peer list
		* but writing defaults here and then writing defaults from channel modul
		* results in a timeout. So we have to ask the channel module for its defaults
		* combine it in a list and write it to the eeprom once
		*/

		// check if we have a registered channel module, otherwise write only defaults from register.h
		if (pModTbl->isActive) {														// copy the respective defaults from cnlDefs into the list3/4 of the usermodule
			memcpy(pModTbl->lstPeer, (uint8_t*)&cnlDefs[ pCnlTbl->sIdx ], pCnlTbl->sLen); // ask to update the peer list
			pModTbl->mDlgt(0x00, 0x02, peer[3 + i], peer + 3, 2);						// contact the user module

		} else pModTbl->lstPeer = (uint8_t*)&cnlDefs[ pCnlTbl->sIdx ];					// no channel module, therefore we use the definition string in register.h

		//dbg << "cnl: " << cnl << ", sLen: " << sLen << ", x: " << _HEX(pModTbl->lstPeer, sLen) << '\n';
		setList(pCnlTbl, free_idx, pModTbl->lstPeer);									// set defaults in the respective list 
		retByte++;																		// increase our return byte
	}

	if (pModTbl->isActive) pModTbl->mDlgt(0x01, 0x00, 0x01, peer, 5);					// inform user module

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("addPeers, cnl:") << cnl << F(", peer:") << _HEX(peer, 5) << '\n';
	#endif
	return retByte;																		// everything went fine, return success
}

/**
* @brief Removes up to 2 peers incl peer channels from the peer database by the given channel
*        and returns the amount of deleted peers incl peer channels
*
* @param cnl  Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to remove from the database
*             { 01, 02, 03, 04, 05 } 01 - 03 peer address, 04 peer channel 1, 05 peer channel 2
*
* @return the amount of removed peers
*/
uint8_t  EE::remPeers(uint8_t cnl, uint8_t *peer) {
	uint8_t lPeer[4];																	// temp array to build peer and peer channel
	uint8_t retByte = 0;																// return value set to 0 while nothing removed
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line

	for (uint8_t i = 0; i < 2; i++) {													// standard gives 2 peer channels
		if (!peer[3 + i]) continue;														// if the current peer channel is empty, go to the next entry 

		memcpy(lPeer, peer, 3);															// copy the peer address into the temp array
		memcpy(lPeer+3, peer+3+i, 1);													// copy the peer channel byte into the array
		uint8_t rem_idx = getIdxByPeer(cnl, lPeer);										// get the index of the peer to remove
		if (rem_idx >= pPeerTbl->pMax) continue;										// peer not found, process next value

		clearEEPromBlock( pPeerTbl->pAddr + (rem_idx * 4), 4);							// clear the peer in the eeprom
		retByte++;
	}

	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("remPeers, cnl:") << cnl << F(", peer:") << _HEX(peer, 5) << F(", removed:") << retByte << '\n';
	#endif
	return retByte;
}

uint8_t  EE::countPeerSlc(uint8_t cnl) {
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line

	if (cnl > cnl_max) return 0;														// return if channel is out of range

	int16_t lTmp = pPeerTbl->pMax - countFreeSlots(cnl) + 1;							// get used slots and add one for terminating zeros
	lTmp = lTmp * 4;																	// 4 bytes per slot

	uint8_t bMax = 0;																	// size return value
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax;																		// return amount of slices
}
uint8_t  EE::getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf) {
	EE::s_peerTbl  *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];								// pointer to the peer table line
	uint8_t byteCnt = 0, slcCnt = 0;													// start the byte counter

	if (cnl > cnl_max) return 0;														// return if channel is out of range

	for (uint8_t i = 0; i < pPeerTbl->pMax; i++) {										// step through the possible peer slots

		getEEPromBlock( pPeerTbl->pAddr + (i*4), 4, buf);								// get peer from eeprom
		if (isEmpty(buf, 4)) continue;													// peer is empty therefor next

		byteCnt+=4;																		// if we are here, then it is valid and we should increase the byte counter
		dbg << i << ": " << _HEX(buf,4) << ", bC: " << byteCnt  << ", sC: " << slcCnt << '\n';

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


// register functions
uint8_t  EE::countRegListSlc(uint8_t cnl, uint8_t lst) {

	if (cnl > cnl_max) return 0;														// channel not valid
	EE::s_cnlTbl *pCnlTbl = getRegListStruct(cnl, lst);										// get the corresponding line in cnlTbl
	if (!pCnlTbl) return 0;																// respective line in cnlTbl not found

	int16_t lTmp = pCnlTbl->sLen * 2;													// get the slice len and multiply by 2 because we need regs and content

	uint8_t bMax = 0;
	while (lTmp > 0) {																	// loop until lTmp gets 0
		lTmp = lTmp - maxMsgLen;														// reduce by max message len
		bMax++;																			// count the loops
	}
	return bMax+1;																		// return amount of slices
}
uint8_t  EE::getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf) {

	if (cnl > cnl_max) return 0;														// channel not valid

	EE::s_peerTbl *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];									// get the corresponding line in peerTbl
	if (idx > pPeerTbl->pMax) return 0;													// peerdb index out of range

	EE::s_cnlTbl *pCnlTbl = getRegListStruct(cnl, lst);										// get the corresponding line in cnlTbl
	if (!pCnlTbl) return 0;																// respective line in cnlTbl not found

	uint8_t slcOffset = slc * maxMsgLen;												// calculate the starting offset
	slcOffset /= 2;																		// divided by to because of mixed message, regs + eeprom content

	int8_t remByte = pCnlTbl->sLen - slcOffset;											// calculate the remaining bytes
	if (remByte <= 0) {																	// check if we are in the last slice and add terminating zeros
		*(uint16_t*)buf = 0;															// add them
		//dbg << slc << ' ' << slcOffset << ' ' << pCnlTbl->sLen << '\n';
		return 2;																		// nothing to do anymore
	}
	if (remByte >= (maxMsgLen/2)) remByte = (maxMsgLen/2);								// shorten remaining bytes if necessary

	uint8_t sIdx  = pCnlTbl->sIdx;
	uint16_t eIdx = pCnlTbl->pAddr + (pCnlTbl->sLen * idx);
	//dbg << slc << ", sO:" << slcOffset << ", rB:" << remByte << ", sIdx:" << pHexB(sIdx) << ", eIdx:" << pHexB(eIdx) << '\n';

	for (uint8_t i = 0; i < remByte; i++) {												// count through the remaining bytes
		*buf++ = _PGM_BYTE( cnlAddr[i + sIdx + slcOffset]);								// add the register address
		getEEPromBlock(i+eIdx+slcOffset, 1, buf++);										// add the eeprom content
		//dbg << (i+eIdx+slcOffset) << '\n';
	}

	return remByte*2;																	// return the byte length
}



/**
* @brief Searches the appropiate line in cnlTbl by comparing cnl and lst
*        parameter and returns the array index or 0xff if nothing found
*
*/
uint8_t  EE::getRegListIdx(uint8_t cnl, uint8_t lst) {
	for (uint8_t i = 0; i < cnl_tbl_max; i++) {											// steps through the cnlTbl
		EE::s_cnlTbl *pCnlTbl = (s_cnlTbl*)&cnlTbl[i];									// get the corresponding line in cnlTbl
		if (pCnlTbl->cnl != cnl) continue;												// next if channel not fit
		if (pCnlTbl->lst != lst) continue;												// next if list not fit
		return i;
	}
	return 0xff;
}
EE::s_cnlTbl* EE::getRegListStruct(uint8_t cnl, uint8_t lst) {
	for (uint8_t i = 0; i < cnl_tbl_max; i++) {											// steps through the cnlTbl
		EE::s_cnlTbl *pCnlTbl = (s_cnlTbl*)&cnlTbl[i];									// get the corresponding line in cnlTbl
		if (pCnlTbl->cnl != cnl) continue;												// next if channel not fit
		if (pCnlTbl->lst != lst) continue;												// next if list not fit
		return (s_cnlTbl*)&cnlTbl[i];													// if we are here we found a line which fits
	}
	return NULL;																		// respective line not found
}


//- some helpers ----------------------------------------------------------------------------------------------------------
inline uint16_t crc16(uint16_t crc, uint8_t a) {
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

uint8_t  isEmpty(void *ptr, uint8_t len) {
	while (len > 0) {
		len--;
		if (*((uint8_t*)ptr+len) != 0) return 0;
	}
	return 1;
}

