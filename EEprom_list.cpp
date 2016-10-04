/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin eeprom list functions ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef EL_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif


//#include "EEprom_peer.h"
#include "AS.h"

EE_LIST ee_list;																			// defined in ee_list.h, load it once
#define maxMsgLen 16																		// define max message length in byte


EE_LIST::EE_LIST() {
	DBG(F("EE_LIST.\n"));
}

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
uint8_t EE_LIST::getList(uint8_t cnl, uint8_t lst, uint8_t peer_idx, uint8_t *buf) {

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
	if (!list) return 0;																	// return if not found

	uint16_t pAddr = list->ee_addr + (list->len * peer_idx);								// calculate the appropiate address
	getEEPromBlock(pAddr, list->len, buf);													// get the eeprom content

	DBG(F("EE:getList cnl:"), list->cnl, F(", lst:"), list->lst, F(", idx:"), idx, F(", addr:"), pAddr, F(", data:"), _HEX(buf, list->len), '\n');
	return 1;																				// report everything ok
}

/*
* @brief Get register value from EEprom
*
* @param cnl Channel
* @param lst List
* @param idx Index of peer (0 if not applicable)
* @param addr address of the register (@see xml device file)
*
* @return the value or 0 if not found
*/
uint8_t EE_LIST::getRegAddr(uint8_t cnl, uint8_t lst, uint8_t peer_idx, uint8_t addr) {

	uint8_t retByte = 0;																	// set the return byte to default

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
	if (!list) return retByte;																// return if not found

	uint16_t pAddr = list->ee_addr + (list->len * peer_idx);								// calculate the appropiate address
	
	for (uint8_t i = 0; i < list->len; i++) {												// step through the register bytes and search for the right address
		if (_PGM_BYTE(list->reg[i]) == addr) {												// compare the bytes till we found the right one
			getEEPromBlock(pAddr + i, 1, (void*)&retByte);									// get the respective byte from eeprom
			break;																			// and exit the for counter
		}
	}

	DBG(F("EE:getRegAddr cnl:"), list->cnl, F(", lst:"), list->lst, F(", idx:"), peer_idx, F(", addr:"), pAddr, F(", reg:"), addr, F(", ret:"), retByte, '\n');

	return retByte;
}


/*
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
uint8_t EE_LIST::setList(uint8_t cnl, uint8_t lst, uint8_t peer_idx, uint8_t *buf) {

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
	if (!list) return 0;																	// return if not found

	uint16_t pAddr = list->ee_addr + (list->len * peer_idx);								// calculate the appropiate address
	setEEPromBlock(pAddr, list->len, buf);													// get the eeprom content

	DBG(F("EE:setList cnl:"), list->cnl, F(", lst:"), list->lst, F(", idx:"), idx, F(", addr:"), pAddr, F(", data:"), _HEX(buf, list->len), '\n');
	return 1;																				// report everything ok
}

/*
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
uint8_t EE_LIST::setListArray(uint8_t cnl, uint8_t lst, uint8_t peer_idx, uint8_t len, uint8_t *buf) {

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
	if (!list) return 0;																	// return if not found

	uint16_t pAddr = list->ee_addr + (list->len * peer_idx);								// calculate the appropiate address

	DBG( F("EE:setList cnl:"), cnl, F(", lst:"), lst, F(", idx:"), peer_idx, F(", data:"), _HEX(buf, len), '\n' );

	for (uint8_t i = 0; i < len; i += 2) {													// step through the input array

		for (uint8_t j = 0; j < list->len; j++) {											// search for the matching address in cnlAddr

			if (_PGM_BYTE(list->reg[j]) == buf[i]) {										// if byte found
				setEEPromBlock(pAddr + j, 1, (void*)&buf[i + 1]);							// add the eeprom content
				DBG( F("reg:"), _HEXB(buf[i]), F(", val:"), _HEXB(buf[i]), F(", addr:"), pAddr + j, '\n');
				break;																		// go to the next i
			}
		}
	}
}


/* 
* @brief Calculates the amount of slices needed to transmit a complete list
*
* @param cnl Requested channel
* @param lst Requested list
*
* @return The amount of slices
*/
uint8_t  EE_LIST::countRegListSlc(uint8_t cnl, uint8_t lst) {
	uint8_t needed_slice = 0;																// set the return value to 0

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
		if (!list) return 0;																// return if not found

	int16_t total_bytes = list->len * 2;													// get the slice len and multiply by 2 because we need regs and content

	while (total_bytes > 0) {																// loop until total_bytes gets 0
		total_bytes -= maxMsgLen;															// reduce by max message len
		needed_slice++;																		// count the slices
	}
	needed_slice++;
	DBG( F("EE:countRegListSlc cnl:"), cnl, F(", lst:"), lst, F(", slc:"), needed_slice, '\n');
	return needed_slice;																	// return amount of slices
}

uint8_t  EE_LIST::getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf) {

	s_list_table *list = find_list(cnl, lst);												// search the channel module for the appropiate list
	if (!list) return 0;																	// return if not found

	uint8_t slcOffset = slc * maxMsgLen;													// calculate the starting offset
	slcOffset /= 2;																			// divided by to because of mixed message, regs + eeprom content

	int8_t remByte = list->len - slcOffset;													// calculate the remaining bytes
	if (remByte <= 0) {																		// check if we are in the last slice and add terminating zeros
		*(uint16_t*)buf = 0;																// add them
		//dbg << slc << ' ' << slcOffset << ' ' << list->len << '\n';
		return 2;																			// nothing to do anymore
	}
	if (remByte >= (maxMsgLen / 2)) remByte = (maxMsgLen / 2);								// shorten remaining bytes if necessary

	uint16_t pAddr = list->ee_addr + (list->len * idx);
	//dbg << slc << ", sO:" << slcOffset << ", rB:" << remByte << ", sIdx:" << pHexB(sIdx) << ", eIdx:" << pHexB(eIdx) << '\n';

	for (uint8_t i = 0; i < remByte; i++) {													// count through the remaining bytes
		*buf++ = _PGM_BYTE(list->reg[i + slcOffset]);										// add the register address
		getEEPromBlock(i + pAddr + slcOffset, 1, buf++);									// add the eeprom content
		//dbg << (i+eIdx+slcOffset) << '\n';
	}

	DBG(F("EE:getRegListSlc cnl:"), cnl, F(", lst:"), lst, F(", idx:"), idx, F(", slc:"), slc, F(", data:"), _HEX(buf, remByte * 2), '\n');
	return remByte * 2;																	// return the byte length
}

/*
* @brief Finds and returns the requested channel or peer list.
*        Helper function for ee_peer and ee_list...
*/
s_list_table* EE_LIST::find_list(uint8_t cnl, uint8_t lst) {
	cmMaster *pCM = pcnlModule[cnl];													// short hand to channel module

	if      (pCM->lstC.lst == lst) return &pCM->lstC;									// check if required list is available
	else if (pCM->lstP.lst == lst) return &pCM->lstP;									// and set the pointer to it

	return NULL;																		// if not, return
}



