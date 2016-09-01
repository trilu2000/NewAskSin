/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

//#define CM_DBG																			// debug message flag

#include "cmMaster.h"

//21.388
//1014

//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const EE::s_cnlTbl *ptr_cnlTbl, const EE::s_cnlTbl *ptr_peerTbl) : cT(ptr_cnlTbl), pT(ptr_peerTbl) {
	chnl_list = new uint8_t[cT->sLen];
	peer_list = new uint8_t[pT->sLen];
}

void cmMaster::message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime) {
	#ifdef CM_DBG
	dbg << F("cmM, trigger11, setValue:") << value << F(", rampTime:") << intTimeCvt(*(uint16_t*)rampTime) << F(", duraTime:") << intTimeCvt(*(uint16_t*)duraTime) << '\n';
	#endif
}

void cmMaster::message_trigger3E(uint8_t msgLng, uint8_t msgCnt) {
	#ifdef CM_DBG
	dbg << F("cmM, trigger3E, msgLng:") << msgLng << F(", msgCnt:") << msgCnt << '\n';
	#endif
}

void cmMaster::message_trigger40(uint8_t msgLng, uint8_t msgCnt) {
	#ifdef CM_DBG
	dbg << F("cmM, trigger40, msgLng:") << msgLng << F(", msgCnt:") << msgCnt << '\n';
	#endif
}
void cmMaster::message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal) {
	#ifdef CM_DBG
	dbg << F("cmM, trigger41, val:") << msgLng << '\n';
	#endif
}


void cmMaster::info_config_change(void) {
	#ifdef CM_DBG
	dbg << F("cmM, config_change\n");
	#endif
}
/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st 3 bytes shows the peer address, 4th and 5th byte gives the peer channel
* no need for sending an answer here, for information only
*/
void cmMaster::info_peer_add(uint8_t *data, uint8_t len) {
	#ifdef CM_DBG
	dbg << F("cmM, peer_add: peer: ") << _HEX(data, 3) << F(", pCnl1: ") << _HEXB(data[3]) << F(", pCnl2: ") << _HEXB(data[4]) << '\n';
	#endif
}


void cmMaster::request_peer_defaults(uint8_t pIdx, uint8_t pCnl1, uint8_t pCnl2) {
	#ifdef CM_DBG
	dbg << F("cmM, peer_defaults: peerIndex: ") << _HEXB(pIdx) << F(", peerCnl1: ") << _HEXB(pCnl1) << F(", peerCnl2: ") << _HEXB(pCnl2) << '\n';
	#endif
}
void cmMaster::request_pair_status(void) {
	#ifdef CM_DBG
	dbg << F("cmM, pair_status\n");
	#endif
}


void cmMaster::poll(void) {
}
void cmMaster::set_toggle(void) {
	#ifdef CM_DBG
	dbg << F("cmM, toggle\n");
	#endif
}


