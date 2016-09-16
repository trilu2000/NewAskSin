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
const cmMaster *pCM;

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

	const s_cnlTbl *cnlTblPtr = getRegListStruct(cnl, lst);
	return getList(cnlTblPtr, idx, buf);
}
uint8_t  EE::getList(const s_cnlTbl* cnlTblPtr, uint8_t idx, uint8_t *buf) {

	if (!cnlTblPtr) return 0;

	if (idx > peerTbl[cnlTblPtr->cnl].pMax) return 0;									// index out of range

	uint16_t pAddr = cnlTblPtr->pAddr + (cnlTblPtr->sLen * idx);						// calculate the address
	getEEPromBlock(pAddr, cnlTblPtr->sLen, buf);										// get the eeprom content

	#ifdef EE_DBG																		// only if ee debug is set
	//dbg << F("setListByTblIdx, cnl:") << cnlTbl[cnlTblIdx].cnl << F(", lst:") << cnlTbl[cnlTblIdx].lst << F(", idx:") << idx << F(", tblRow:") << cnlTblIdx << F(", addr:") << pAddr << '\n';
	//dbg << F("data:") << _HEX(buf, cnlTbl[cnlTblIdx].sLen) << '\n';
	#endif
	return 1;																			// report everything ok
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

	const s_cnlTbl *cnlTblPtr = getRegListStruct(cnl, lst);
	return setList(cnlTblPtr, idx, buf);

	//uint8_t xI = getRegListIdx(cnl, lst);												// get the respective line in cnlTbl
	//dbg << F("setList, cnl:") << cnl << F(", lst:") << lst << F(", idx:") << idx << F(", cnlTbl:") << xI << '\n';
	//return setList(xI, idx, buf);												// call setlist shortend
}
uint8_t  EE::setList(const s_cnlTbl* cnlTblPtr, uint8_t idx, uint8_t *buf) {

	if (!cnlTblPtr) return 0;
	//dbg << F("setListByTblIdx, cnlTblIdx:") << cnlTblIdx << F(", idx:") << idx << '\n';

	if (idx > peerTbl[cnlTblPtr->cnl ].pMax) return 0;									// index out of range

	uint16_t pAddr = cnlTblPtr->pAddr + (cnlTblPtr->sLen * idx);						// calculate the address
	setEEPromBlock(pAddr, cnlTblPtr->sLen, buf);										// set the eeprom content
	//uint16_t pAddr = cnlTbl[cnlTblIdx].pAddr + (cnlTbl[cnlTblIdx].sLen * idx);			// calculate the address
	//setEEPromBlock(pAddr, cnlTbl[cnlTblIdx].sLen, buf);									// set the eeprom content

	#ifdef EE_DBG																		// only if ee debug is set
	//dbg << F("setListByTblIdx, cnl:") << cnlTbl[cnlTblIdx].cnl << F(", lst:") << cnlTbl[cnlTblIdx].lst << F(", idx:") << idx << F(", tblRow:") << cnlTblIdx << F(", addr:") << pAddr << '\n';
	//dbg << F("data:") << _HEX(buf, cnlTbl[cnlTblIdx].sLen) << '\n';
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
	const s_cnlTbl *cnlTblPtr = getRegListStruct(cnl, lst);
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
	DBG( F("EE:init\n"));																// some debug

	initEEProm();																		// init function if a i2c eeprom is used

	/*
	* @brief First time start check is done via comparing a magic number at the start of the eeprom
	*        with the CRC of the cnlTbl information. The idea behind is, that every time there was a
	*        change in the cnlTbl some addresses are changed and we have to rewrite the eeprom content.
	*/
	uint16_t eepromCRC, flashCRC;														// define two variables to compare the CRC
	uint8_t  *p = (uint8_t*)cnlTbl;														// cast the cnlTbl to char

	for (uint8_t i = 0; i < ( cnl_tbl_max * sizeof(s_cnlTbl)); i++) {					// step through all bytes of the channel table
		flashCRC = crc16(flashCRC, p[i]);												// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eepromCRC);												// get magic byte from eeprom
	DBG( F("EE:crc, flash:"), flashCRC, F(", eeprom: "), eepromCRC, '\n');				// some debug

	if(flashCRC != eepromCRC) {															// first time detected, format eeprom, load defaults and write magicByte
		DBG( F("writing new magic byte\n") );											// some debug
		setEEPromBlock(0, 2, (void*)&flashCRC);											// write eMagicByte to eeprom

		/*
		* @brief Write the defaults defined in register.h into the respective slot in the eeprom.
		*        We have to write only list0 or list 1 content, while list 3 or list 4 is written at the peering process
		*        Content is defined in cnlDefs as PROGMEM. So we have to copy the content first into the channel modules
		*        and write it back to the eeprom.
		*/
		for (uint8_t i = 0; i <= cnl_max; i++) {										// write the defaults in respective list0/1
			cmMaster *pCM = pcnlModule[i];												// short hand to respective channel master	
			memcpy_P( pCM->chnl_list, cnlDefs + pCM->cLT->sIdx, pCM->cLT->sLen );		// copy from progmem into array
			setList( pCM->cLT, 0, pCM->chnl_list );										// write it into the eeprom
			DBG(F("EE:write defaults, cnl:"), pCM->cLT->cnl, F(", lst:"), pCM->cLT->lst, F(", sIdx:"), pCM->cLT->sIdx, F(", sLen:"), pCM->cLT->sLen, F(", data:"), _HEX( pCM->chnl_list, pCM->cLT->sLen), '\n');
		}

		/*
		* @brief Format the peer db
		*/
		ee_peer.clearPeers();

		/*
		* @brief Function to be placed in register.h, to setup default values on first time start
		*/
		firstTimeStart();
	}


	/*
	* @brief Read the defaults from the respective slot in the eeprom and store it in the channel module space.
	*        We have to read only list0 or list 1 content, while list 3 or list 4 is read while received a peer message.
	*        After loading the defaults we inform the modules of this by calling the info_config_change function.
	*/
	for (uint8_t i = 0; i <= cnl_max; i++) {											// step through all channels
		cmMaster *pCM = pcnlModule[i];													// short hand to respective channel master	
		getList(pCM->cLT, 0, pCM->chnl_list );											// read the defaults in respective list0/1
		pCM->info_config_change();														// inform the channel modules
		DBG(F("EE:read defaults, cnl:"), pCM->cLT->cnl, F(", lst:"), pCM->cLT->lst, F(", sIdx:"), pCM->cLT->sIdx, F(", sLen:"), pCM->cLT->sLen, F(", data:"), _HEX(pCM->chnl_list, pCM->cLT->sLen), '\n');
	}


	getEEPromBlock(15, 16, HMKEY);														// get HMKEY from EEprom
	getEEPromBlock(14, 1, hmKeyIndex);													// get hmKeyIndex from EEprom
	if (HMKEY[0] == 0x00) {																// if HMKEY in EEPROM invalid
		initHMKEY();
	}

	/*
	* @brief Add this function in register.h to setup default values every start
	*/
	everyTimeStart();
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


// peer functions

// register functions
uint8_t  EE::countRegListSlc(uint8_t cnl, uint8_t lst) {

	if (cnl > cnl_max) return 0;														// channel not valid
	s_cnlTbl *pCnlTbl = getRegListStruct(cnl, lst);									// get the corresponding line in cnlTbl
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

	s_peerTbl *pPeerTbl = (s_peerTbl*)&peerTbl[cnl];									// get the corresponding line in peerTbl
	if (idx > pPeerTbl->pMax) return 0;													// peerdb index out of range

	s_cnlTbl *pCnlTbl = getRegListStruct(cnl, lst);									// get the corresponding line in cnlTbl
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
		s_cnlTbl *pCnlTbl = (s_cnlTbl*)&cnlTbl[i];										// get the corresponding line in cnlTbl
		if (pCnlTbl->cnl != cnl) continue;												// next if channel not fit
		if (pCnlTbl->lst != lst) continue;												// next if list not fit
		return i;
	}
	return 0xff;
}
s_cnlTbl* EE::getRegListStruct(uint8_t cnl, uint8_t lst) {
	for (uint8_t i = 0; i < cnl_tbl_max; i++) {											// steps through the cnlTbl
		s_cnlTbl *pCnlTbl = (s_cnlTbl*)&cnlTbl[i];										// get the corresponding line in cnlTbl
		if (pCnlTbl->cnl != cnl) continue;												// next if channel not fit
		if (pCnlTbl->lst != lst) continue;												// next if list not fit
		dbg << "ls:" << i << '\n';
		return pCnlTbl;																	// if we are here we found a line which fits
		//return (s_cnlTbl*)&cnlTbl[i];													// if we are here we found a line which fits
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
		if ( *((uint8_t*)ptr+len) ) return 0;
	}
	return 1;
}

