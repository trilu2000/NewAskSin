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


//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const EE::s_cnlTbl *ptr_cnlTbl, const EE::s_cnlTbl *ptr_peerTbl, const EE::s_peerTbl *ptr_peerDB) : cLT(ptr_cnlTbl), cPT(ptr_peerTbl), pDB(ptr_peerDB) {
	chnl_list = new uint8_t[cLT->sLen];
	//if ((uint16_t)&*pT) peer_list = new uint8_t[pT->sLen];									// array only needed if there is a list3/4
	peer_list = new uint8_t[cPT->sLen];															// doesn't matter, list0 is normally 6 to 8 byte

	DBG( F("cmM, cnl/lst: "), cLT->cnl, '/', cLT->lst, F(", cnl/lst: "), cPT->cnl, '/', cPT->lst, '\n' );
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


