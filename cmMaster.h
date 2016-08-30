/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmMaster_H
#define _cmMaster_H

#include "AS.h"
#include "Eeprom.h"
#include "HAL.h"


class cmMaster {
public://------------------------------------------------------------------------------------------------------------------
	const EE::s_cnlTbl *cT, *pT;															// pointer to channel table for list0/1 and list3/4 information
	uint8_t *chnl_list, *peer_list;															// array for list0/1 and list3/4

	cmMaster(const EE::s_cnlTbl *ptr_cnlTbl, const EE::s_cnlTbl *ptr_peerTbl);				// constructor

	virtual void message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime);	// pair set message
	virtual void message_trigger3E(uint8_t msgLng, uint8_t msgCnt);							// switch message, also config test button in HM
	virtual void message_trigger40(uint8_t msgLng, uint8_t msgCnt);							// remote messages from peer
	virtual void message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal);			// sensor messages from peer

	virtual void info_config_change(void);													// list1 on registered channel had changed
	virtual void info_peer_add(uint8_t *data, uint8_t len);									// peer was added to the specific channel, 1st 3 bytes shows peer address, 4th and 5th the peer channel

	virtual void request_peer_defaults(uint8_t pIdx, uint8_t pCnl1, uint8_t pCnl2);			// add peer channel defaults to list3/4
	virtual void request_pair_status(void);													// event on status request

	virtual void poll(void);																// poll function, driven by HM loop
	virtual void set_toggle(void);															// toggle the module initiated by config button

};



#endif