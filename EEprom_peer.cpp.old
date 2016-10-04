/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin peer database functions ----------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef EP_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif


#include "EEprom_peer.h"
#include "AS.h"


//EE_PEER ee_peer;																			// defined in ee_peer.h, load it once


EE_PEER::EE_PEER() {
	DBG(F("EE_PEER.\n"));
}

/**
* @brief Returns the channel where the peer was found in the peer database
*
* @param peer Pointer to byte array containing the peer/peer channel
*
* @return the channel were the peer was found, 0 if not
*/
/*uint8_t EE_PEER::isPeerValid(uint8_t *peer) {
	uint8_t retByte = 0;

	for (uint8_t i = 1; i <= cnl_max; i++) {											// step through all channels
		if (getIdxByPeer(i, peer) != 0xff) retByte = i;									// if a valid peer is found return the respective channel
	}

	DBG(F("EE:isPeerValid:"), _HEX(peer, 4), F(", ret:"), retByte, '\n');
	return retByte;																		// otherwise 0
}*/

/**
* @brief Search for a given peer/peer channel in the peer database and return the index
*
* @param cnl Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to search for the index
*
* @return the found idx or 0xff if not found
*/
/*void EE_PEER::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *ret_peer) {
	s_peer_table *peer = &pcnlModule[cnl]->peer;										// short hand to the peer table in the respective channel module
	uint16_t pAddr = peer->ee_addr + (idx * 4);											// calculate the peer address in eeprom
	getEEPromBlock(pAddr, 4, ret_peer);													// get the respective eeprom block

	//DBG( F("EE:getPeerByIdx, cnl:"), cnl, F(", idx:"), idx, F(", peer:"), _HEX(peer, 4), '\n' );
}*/


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
//uint8_t EE_PEER::addPeers(uint8_t cnl, uint8_t *add_peer) {
/*	uint8_t retByte = 0;
	cmMaster  *pCM = pcnlModule[cnl];
	s_peer_table *peer = &pcnlModule[cnl]->peer;										// short hand to the peer table in the respective channel module
	s_list_table *list = &pcnlModule[cnl]->lstP;										// short hand to the respective list

	// remove peers if exist 
	remPeers(cnl, add_peer);															// first we remove the probably existing entries in the peer database

																						// find the free slots and write peers - set the respective list 3/4 also 
	for (uint8_t i = 0; i < 2; i++) {													// standard gives 2 peer channels
		if (!add_peer[3 + i]) continue;													// if the current peer channel is empty, go to the next entry 

																						// find an empty slot
		uint8_t empty_4_byte[] = { 0,0,0,0 };											// empty peer slot or for compare... 
		uint8_t free_idx = getIdxByPeer(cnl, empty_4_byte);								// get the index of the free slot
		if (free_idx == 0xff) return retByte;											// no free index found, probably not needed while checked the free slots before

																						// write the peer - takes 12 to 14ms
		uint16_t pAddr = list->ee_addr + (free_idx * 4);								// calculate the peer address in eeprom
		setEEPromBlock(pAddr, 3, add_peer);												// write the peer to the eeprom
		setEEPromBlock(pAddr + 3, 1, add_peer + 3 + i);									// write the peer channel to the eeprom
*/
		/*
		* writing defaults take a long time, in case of cmSwitch it is 75ms peer peer list
		* but writing defaults here and then writing defaults from channel modul
		* results in a timeout. So we have to ask the channel module for its defaults
		* combine it in a list and write it to the eeprom once
		*/
/*		memcpy_P(list->val, list->def, list->len);										// copy the defaults from progmem into the peer list
		pCM->request_peer_defaults(add_peer[3 + i], add_peer[3], add_peer[4]);			// ask the channel module to update the respective peer list

		//dbg << "cnl: " << cnl << ", sLen: " << sLen << ", x: " << _HEX(pModTbl->lstPeer, sLen) << '\n';
		pAddr = list->ee_addr + (list->len * free_idx);									// calculate the right address
		setEEPromBlock(pAddr, list->len, list->val);									// write the list to eeprom
		retByte++;																		// increase our return byte
	}

	pCM->info_peer_add(add_peer, 5);													// inform user module

	DBG(F("EE:addPeers, cnl:"), cnl, F(", peer:"), _HEX(add_peer, 5), F(", add:"), retByte, '\n');// some debug
	return retByte;	*/																	// everything went fine, return success
//}

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
//uint8_t EE_PEER::remPeers(uint8_t cnl, uint8_t *rem_peer) {
	/*	s_peer_table *peer = &pcnlModule[cnl]->peer;										// short hand to the peer table in the respective channel module

	uint8_t *lPeer = new uint8_t[4];													// temp byte array to load peer addresses
	uint8_t retByte = 0;																// return value set to 0 while nothing removed

	for (uint8_t i = 0; i < 2; i++) {													// standard gives 2 peer channels
		if (!rem_peer[3 + i]) continue;													// if the current peer channel is empty, go to the next entry 

		memcpy(lPeer, rem_peer, 3);														// copy the peer address into the temp array
		lPeer[3] = rem_peer[3 + i];														// write the peer channel byte into the array

		uint8_t rem_idx = getIdxByPeer(cnl, lPeer);										// get the index of the peer to remove
		if (rem_idx == 0xff) continue;													// peer not found, process next value

		uint16_t pAddr = peer->ee_addr + (rem_idx * 4);									// calculate the peer address in eeprom
		clearEEPromBlock(pAddr, 4);														// clear the peer in the eeprom
		retByte++;
	}

	DBG(F("EE:remPeers, cnl:"), cnl, F(", peer:"), _HEX(rem_peer, 5), F(", removed:"), retByte, '\n');
	return retByte;*/
//}


/*
* @brief Returns the number of slices needed to report all peers by a specific channel
*        Within the HM protocol there is a request CONFIG_PEER_LIST_REQ request. The answer
*        to this request is a INFO_PEER_LIST message with 4 peers in the payload.
*        This functions returns the number of messages needed to send all 4 peer long strings.
*
*/
/*uint8_t EE_PEER::countPeerSlc(uint8_t cnl) {
	s_peer_table *pDB = &pcnlModule[cnl]->peer;											// short hand to the peer table in the respective channel module

	/*
	* @brief calculate the amound of needed slots for all 4 byte peers in the peer database per channel
	*/
	//uint8_t used_slots = pDB->max - countFreeSlots(cnl) + 1;							// get used slots and add one for terminating zeros, not sure if a INT is needed while a byte could indicate 63 peers

	/*
	* @brief calculate the amount of slices depending on the message length
	* message length = 4 peers per string
	* needed slots = 4 / 4 = 1 - works
	* needed slots = 3 / 4 = 0 - didn't work, need 1
	* needed slots = 5 / 4 = 1 - didn't work, need 2
	*/
/*	uint8_t slices = 0;																	// declare the return value
	while (used_slots > 0) {															// loop until used_slots gets 0
		used_slots -= 4;																// reduce by max message len
		slices++;																		// count the loops
	}
	return slices;																		// return amount of slices
}*/

/*
* @brief Copies all peers from the peer database per channel in a buffer array
*        Function is needed to answer the INFO_PEER_LIST message. All peers per channel
*        are copied into the given buffer pointer.
*
* @param cnl  The channel where we want to get the peer list
* @param slc  As we have limited space in the answer, we slice the content
* @param *buf A pointer to the buffer array
*
* @return     The amount of bytes copied into the buffer array
*
*/
/*uint8_t EE_PEER::getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf) {
	s_peer_table *pDB = &pcnlModule[cnl]->peer;											// short hand to the peer table in the respective channel module
	*/
	/*
	* All peers a copied into the buffer array, so we start every time with slice 0.
	* If slice 1 or above is requested we roll over to the required slice
	*/
/*	uint8_t byteCnt = 0, slcCnt = 0;													// start the byte and slice counter
	for (uint8_t i = 0; i < pDB->max; i++) {											// step through the possible peer slots

		uint8_t *lPeer = pDB->get_peer(i);
		memcpy(buf, lPeer, 4);
		//getPeerByIdx(cnl, i, buf);														// get peer from eeprom
		if (isEmpty(buf, 4)) continue;													// peer is empty therefor next

		byteCnt += 4;																	// if we are here, then we got a valid peer and we should increase the byte counter
		DBG(i, ": ", _HEX(buf, 4), ", bC:", byteCnt, ", sC:", slcCnt, '\n');

		if (slcCnt == slc) {															// we are in the asked slice
			buf += 4;																	// peer was added, therefore we should increase the buffer counter for the next i
			if (byteCnt >= 16) goto end_getPeerListSlc;									// we are in the right slice and string is full, return the amount of bytes in the string

		}
		else if (byteCnt >= 16) {														// we are in the wrong slice, but the byte counter is full
			slcCnt++;																	// increase the slice counter, while we need to start with the next slice
			byteCnt = 0;																// and reset the byte counter
		}
	}

	// if we are here, we had processed all peer entries in the peer database and there is still place to
	// add the terminating zeros to the string
	memset(buf, 0, 4); byteCnt += 4;													// add the terminating zeros

end_getPeerListSlc:
	DBG(F("EE:getPeerListSlc, cnl:"), cnl, F(", slc:"), slc, F(", data:"), _HEX(buf, byteCnt), '\n');
	return byteCnt;																		// return the amount of bytes
}*/


/**
* @brief Returns the amount of free peer slots of the peer database
*        of the given channel.
*
* @param cnl Channel
*
* @return the amount of free slots
*/
/*uint8_t EE_PEER::countFreeSlots(uint8_t cnl) {
	s_peer_table *pDB = &pcnlModule[cnl]->peer;											// short hand to the peer table in the respective channel module

	uint8_t *lPeer = new uint8_t[4];													// temp byte array to load peer addresses
	uint8_t bCounter = 0;																// set counter to zero

	for (uint8_t i = 0; i < pDB->max; i++) {											// step through the possible peer slots
		getPeerByIdx(cnl, i, lPeer);
		if (isEmpty(lPeer, 4)) bCounter++;												// increase counter if peer slot is empty
	}

	DBG(F("EE:countFreeSlots, cnl:"), cnl, F(", total:"), pDB->max, F(", free:"), bCounter, '\n');
	return bCounter;																	// return the counter
}*/

/**
* @brief Clears the complete defined peer database in eeprom
*        by writing 0's. Used on first time start or device reset
*        to defaults.
*
*/
/*void EE_PEER::clearPeers(void) {
	for (uint8_t i = 0; i <= cnl_max; i++) {											// step through all channels
		s_peer_table *pDB = &pcnlModule[i]->peer;										// short hand to the peer table in the respective channel module
		clearEEPromBlock(pDB->ee_addr, pDB->max * 4);
		DBG(F("EE:clearPeers, cnl:"), i, F(", addr:"), pDB->ee_addr, F(", len:"), (pDB->max * 4), '\n');																	// ...and some information
	}
}*/

/**
* @brief Search for a given peer/peer channel in the peer database and return the index
*
* @param cnl Channel
* @param peer Pointer to a byte array with the peer and respective
*             peerchannel to search for the index
*
* @return the found idx or 0xff if not found
*/
/*inline uint8_t EE_PEER::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	s_peer_table *pDB = &pcnlModule[cnl]->peer;											// short hand to the peer table in the respective channel module

	uint8_t *lPeer ;													// temp byte array to load peer addresses
	uint8_t retByte = 0xff;																// default return value

	for (uint8_t i = 0; i < pDB->max; i++) {											// step through the possible peer slots
		lPeer = pDB->get_peer(i);
		//getPeerByIdx(cnl, i, lPeer);
		//dbg << "ee:" << _HEX(lPeer, 4) << ", gv:" << _HEX(peer, 4) << "\n";
		if (isEqual(lPeer, peer, 4)) { retByte = i; break; };							// if result matches then return slot index
	}

	DBG(F("EE:getIdxByPeer, cnl:"), cnl, F(", peer:"), _HEX(peer, 4), F(", ret:"), retByte, '\n');
	return retByte;
}*/


