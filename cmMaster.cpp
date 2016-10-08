/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "cmMaster.h"


uint8_t cnl_max = 0;																			// defined in cmMaster.h, increased by every instance which is initialized

//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const uint8_t peer_max) {
	peerDB.max = peer_max;

	//list[lstC.lst] = &lstC;																	// moved into cm_init
	//if (lstP.lst < 5 ) list[lstP.lst] = &lstP;
	lstC.cnl = cnl_max;																			// set the channel to the lists
	lstP.cnl = cnl_max++;
	
	DBG_START(CM, F("CM["), lstC.cnl, F("].\n"));
}


void cmMaster::message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime) {
	DBG(CM, F("CM:trigger11, setValue:"), value, F(", rampTime:"), intTimeCvt(*(uint16_t*)rampTime), F(", duraTime:"), intTimeCvt(*(uint16_t*)duraTime), '\n' );
}

void cmMaster::message_trigger3E(uint8_t msgLng, uint8_t msgCnt) {
	DBG(CM, F("CM:trigger3E, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger40(uint8_t msgLng, uint8_t msgCnt) {
	DBG(CM, F("CM:trigger40, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal) {
	DBG(CM, F("CM:trigger41, val:"), msgLng, '\n' );
}


void cmMaster::info_config_change(void) {
	DBG(CM, F("CM:config_change\n") );
}
/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st 3 bytes shows the peer address, 4th and 5th byte gives the peer channel
* no need for sending an answer here, for information only
*/
void cmMaster::info_peer_add(s_m01xx01 *buf) {
	DBG(CM, F("CM:info_peer_add, peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n');
}


void cmMaster::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {
	DBG(CM, F("CM:request_peer_defaults, idx:"), _HEXB(idx), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n' );
}
void cmMaster::request_pair_status(void) {
	DBG(CM, F("CM:pair_status\n") );
}


void cmMaster::poll(void) {
}
void cmMaster::set_toggle(void) {
	DBG(CM, F("CM:toggle\n") );
}



/*
* @brief Received message handling forwarded by AS::processMessage
*/


void cmMaster::CONFIG_PEER_ADD(s_m01xx01 *buf) {
	uint8_t *temp_peer = new uint8_t[4];													// temp byte array to load peer addresses
	uint8_t ret_byte = 0;																	// prepare a placeholder for success reporting 

	for (uint8_t i = 0; i < 2; i++) {														// standard gives 2 peer channels
		if (!buf->PEER_CNL[i]) continue;													// if the current peer channel is empty, go to the next entry 

		memcpy(temp_peer, buf->PEER_ID, 3);													// copy the peer address into the temp array
		temp_peer[3] = buf->PEER_CNL[i];													// write the peer channel byte into the array

		uint8_t idx = peerDB.get_idx(temp_peer);											// search if we have already the peer in the database
		if (idx == 0xff) idx = peerDB.get_free_slot();										// not in the in the database, search a free slot

		if (idx != 0xff) {																	// free slot available
			peerDB.set_peer(idx, temp_peer);												// write the peer into the database

			lstP.load_default();															// copy the defaults from progmem into the peer list, index doesn't matter
			request_peer_defaults(idx, buf);												// ask the channel module to load the defaults
			lstP.save_list(idx);															// and save the list, index is important while more choices in the peer table
			ret_byte++;																		// increase success
		}
	}
	info_peer_add(buf);																		// inform the user module of the added peer

	DBG(CM, F("CM:CONFIG_PEER_ADD, cnl:"), lstP.cnl, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", RET:"), ret_byte, '\n');
	hm.checkSendACK(ret_byte);
}

void cmMaster::CONFIG_PEER_REMOVE(s_m01xx02 *buf) {
	uint8_t *temp_peer = new uint8_t[4];													// temp byte array to load peer addresses
	uint8_t ret_byte = 0;																	// prepare a placeholder for success reporting 

	for (uint8_t i = 0; i < 2; i++) {														// standard gives 2 peer channels
		if (!buf->PEER_CNL[i]) continue;													// if the current peer channel is empty, go to the next entry 

		memcpy(temp_peer, buf->PEER_ID, 3);													// copy the peer address into the temp array
		temp_peer[3] = buf->PEER_CNL[i];													// write the peer channel byte into the array
		uint8_t idx = peerDB.get_idx(temp_peer);											// find the peer in the database

		if (idx != 0xff) {																	// found it
			peerDB.clear_peer(idx);															// delete the peer in the database
			ret_byte++;																		// increase success
		}

	}

	DBG(CM, F("CM:CONFIG_PEER_REMOVE, cnl:"), lstC.cnl, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n');
	hm.checkSendACK(ret_byte);
}






//- helpers ---------------------------------------------------------------------------------------------------------------
/*
* @brief Prepare defaults and read the defaults from the eeprom in the channel module space.
*        We have to read only list0 or list 1 content, while list 3 or list 4 is read while received a peer message.
*        After loading the defaults we inform the modules of this by calling the info_config_change function.
*/
uint16_t cm_prep_default(uint16_t ee_start_addr) {

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		
		pCM->lstC.val = new uint8_t[pCM->lstC.len];										// create and allign the value arrays
		pCM->lstP.val = new uint8_t[pCM->lstP.len];

		pCM->list[pCM->lstC.lst] = &pCM->lstC;											// allign lstC to the list array
		if (pCM->lstP.lst < 5) pCM->list[pCM->lstP.lst] = &pCM->lstP;					// because of the absence of lstP in channel0

		pCM->lstC.ee_addr = ee_start_addr;												// write the eeprom address in the channel list
		ee_start_addr += pCM->lstC.len;													// create new address by adding the length of the list before
		pCM->lstP.ee_addr = ee_start_addr;												// write the eeprom address in the peer list
		ee_start_addr += (pCM->lstP.len * pCM->peerDB.max);								// create new address by adding the length of the list before but while peer list, multiplied by the amount of possible peers

		// defaults loaded in the AS module init, on every time start
		DBG(CM, F("CM:prep_defaults, cnl:"), pCM->lstC.cnl, F(", lst:"), pCM->lstC.lst, F(", len:"), pCM->lstC.len, F(", data:"), _HEX(pCM->lstC.val, pCM->lstC.len), '\n');
	}

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		pCM->peerDB.ee_addr = ee_start_addr;											// write eeprom address into the peer table
		ee_start_addr += pCM->peerDB.max * 4;											// create nwe eeprom start address depending on the space for max peers are used
	}
	return ee_start_addr;
}

/*
* @brief Search for a 4 byte peer address in all channel instances and returns
*        the channel number where the peer was found. Returns 0 if nothing was found.
*/
uint8_t  is_peer_valid(uint8_t *peer) {
	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		if (pCM->peerDB.get_idx(peer) != 0xff) return i;								// ask the peer table to find the peer, if found, return the cnl
	}
	return 0;																			// nothing was found, return 0
}

/*
* @brief Calculates and returns the crc number for all channel module lists.
* This information is needed for the first time check. Within this check, we compare a stored
* magic number with the calculated one and if they differ, we have a good indication that something
* was changed in the configuration.
*/
uint16_t cm_calc_crc(void) {
	uint16_t flashCRC = 0;																// define and set the return value

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		s_peer_table *pPeer = &ptr_CM[i]->peerDB;										// short hand to the peer database
		for (uint8_t j = 0; j < list_max; j++) {										// step through all lists
			s_list_table *pList = ptr_CM[i]->list[j];									// short hand to list module
			if (!pList) continue;														// skip on empty pointer
			flashCRC = crc16_P(flashCRC, pList->len, pList->reg);						// and calculate the crc - arrays are in PROGMEM
			flashCRC = crc16_P(flashCRC, pList->len, pList->def);
			flashCRC = crc16(flashCRC, pPeer->max);
			DBG(CM, F("CM:calc_crc cnl:"), i, F(", crc:"), flashCRC, '\n');				// some debug
		}
	}
	return flashCRC;
}
inline uint16_t crc16_P(uint16_t crc, uint8_t len, const uint8_t *buf) {
	for (uint8_t i = 0; i < len; i++) {													// step through all channels
		uint8_t t = _PGM_BYTE(buf[i]);
		crc = crc16(crc, t);
	}
	return crc;
}
inline uint16_t crc16(uint16_t crc, uint8_t a) {
	crc ^= a;
	for (uint8_t i = 0; i < 8; ++i) {
		if (crc & 1) crc = (crc >> 1) ^ 0xA001;
		else crc = (crc >> 1);
	}
	return crc;
}