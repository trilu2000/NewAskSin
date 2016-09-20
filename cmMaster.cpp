/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef CM_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif

#include "cmMaster.h"


uint8_t cnl_max = 0;																			// defined in cmMaster.h, increased by every instance which is initialized

//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const uint8_t peer_max) {
	peer.max = peer_max;

	lstC.cnl = cnl_max;																			// set the channel to the lists
	lstP.cnl = cnl_max++;

	lstC.val = new uint8_t[lstC.len];
	lstP.val = new uint8_t[lstP.len];															// doesn't matter, list0 is normally 6 to 8 byte

	DBG( F("cmM, cnl/lst: "), lstC.cnl, '/', lstC.lst, F(", cnl/lst: "), lstP.cnl, '/', lstP.lst, '\n' );
}


void cmMaster::message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime) {
	DBG( F("cmM, trigger11, setValue:"), value, F(", rampTime:"), intTimeCvt(*(uint16_t*)rampTime), F(", duraTime:"), intTimeCvt(*(uint16_t*)duraTime), '\n' );
}

void cmMaster::message_trigger3E(uint8_t msgLng, uint8_t msgCnt) {
	DBG( F("cmM, trigger3E, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger40(uint8_t msgLng, uint8_t msgCnt) {
	DBG( F("cmM, trigger40, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal) {
	DBG( F("cmM, trigger41, val:"), msgLng, '\n' );
}


void cmMaster::info_config_change(void) {
	DBG( F("cmM, config_change\n") );
}
/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st 3 bytes shows the peer address, 4th and 5th byte gives the peer channel
* no need for sending an answer here, for information only
*/
void cmMaster::info_peer_add(uint8_t *data, uint8_t len) {
	DBG( F("cmM, peer_add: peer: "), _HEX(data, 3), F(", pCnl1: "), _HEXB(data[3]), F(", pCnl2: "), _HEXB(data[4]), '\n' );
}


void cmMaster::request_peer_defaults(uint8_t pIdx, uint8_t pCnl1, uint8_t pCnl2) {
	DBG( F("cmM, peer_defaults: peerIndex: "), _HEXB(pIdx), F(", peerCnl1: "), _HEXB(pCnl1), F(", peerCnl2: "), _HEXB(pCnl2), '\n' );
}
void cmMaster::request_pair_status(void) {
	DBG( F("cmM, pair_status\n") );
}


void cmMaster::poll(void) {
}
void cmMaster::set_toggle(void) {
	DBG( F("cmM, toggle\n") );
}




//- helpers ---------------------------------------------------------------------------------------------------------------
/*
* @brief Prepare defaults and read the defaults from the eeprom in the channel module space.
*        We have to read only list0 or list 1 content, while list 3 or list 4 is read while received a peer message.
*        After loading the defaults we inform the modules of this by calling the info_config_change function.
*/
uint16_t cm_prep_default(uint16_t ee_start_addr) {

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = pcnlModule[i];													// short hand to respective channel master	

		pCM->lstC.ee_addr = ee_start_addr;												// write the eeprom address in the channel list
		ee_start_addr += pCM->lstC.len;													// create new address by adding the length of the list before
		pCM->lstP.ee_addr = ee_start_addr;												// write the eeprom address in the peer list
		ee_start_addr += (pCM->lstP.len * pCM->peer.max);								// create new address by adding the length of the list before but while peer list, multiplied by the amount of possible peers

		getEEPromBlock(pCM->lstC.ee_addr, pCM->lstC.len, pCM->lstC.val);				// read the defaults in respective list0/1
		pCM->info_config_change();														// inform the channel modules
		DBG(F("cmM:prep_defaults, cnl:"), pCM->lstC.cnl, F(", lst:"), pCM->lstC.lst, F(", len:"), pCM->lstC.len, F(", data:"), _HEX(pCM->chnl_list, pCM->lstC.len), '\n');
	}

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = pcnlModule[i];													// short hand to respective channel master	
		pCM->peer.ee_addr = ee_start_addr;												// write eeprom address into the peer table
		ee_start_addr += pCM->peer.max * 4;												// create nwe eeprom start address depending on the space for max peers are used
	}
	return ee_start_addr;
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
		cmMaster *pCM = pcnlModule[i];													// short hand to channel module
		flashCRC = crc16_P(flashCRC, pCM->lstC.len, pCM->lstC.reg);						// and calculate the crc - arrays are in PROGMEM
		flashCRC = crc16_P(flashCRC, pCM->lstC.len, pCM->lstC.def);
		flashCRC = crc16_P(flashCRC, pCM->lstP.len, pCM->lstP.reg);
		flashCRC = crc16_P(flashCRC, pCM->lstP.len, pCM->lstP.def);
		flashCRC = crc16(flashCRC, pCM->peer.max);
		DBG(F("cmM:calc_crc cnl:"), i, F(", crc:"), flashCRC, '\n');					// some debug
	}
	return flashCRC;
}
inline uint16_t crc16_P(uint16_t crc, uint8_t len, uint8_t *buf) {
	for (uint8_t i = 0; i < len; i++) {												// step through all channels
		crc = crc16(crc, _PGM_BYTE(buf[i]));
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