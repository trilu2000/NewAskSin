/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin registrar functions -----------------------------------------------------------------------------------------------
* - with a lot of support from martin876 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

//#define RL_DBG																			// debug message flag
#include "cmSwitch.h"


//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
/**
* @brief Config function to set up channel specific settings, called by 
*        the regInHM function. No need to call from outside.
*
*/
void cmSwitch::config(void) { 

	l3 = (s_l3*)&lstPeer;																	// set pointer to something useful
	l3->ACTION_TYPE = ACTION::INACTIVE;														// and secure that no action will happened in polling function

	initSwitch(regCnl);																		// call external init function to set the output pins
	modStat = setStat = 0;																	// output to 0
	curStat = JT::OFF;																		// initialize the jump table value

	msgDelay = (rand() % 2000) + 1000;														// set message delay
	msgTmr.set(msgDelay);																	// wait some time to settle the device
	sendStat = INFO::ACTUATOR_STATUS;														// send the initial status info
}

/**
* @brief Function is called on messages comming from a central device.
*        No need to call the function from outside as it is
*        called from the module registrar
*
* @param setValue 1 byte containing the value we have to set
* @param rampTime Pointer to 2 byte array containing the encoded ramptime value
* @param duraTime Pointer to 2 byte array containing the encoded durationtime value
*
*/
void cmSwitch::trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *duraTime) {
	l3->ACTION_TYPE = ACTION::INACTIVE;														// action type to off otherwise the polling function will overwrite

	// convert the timer values and prepare the tr11 handling
	//rampTme = *(uint16_t*)*rampTime;														// if ramp time is given, bring it in the right format
	//duraTme = *(uint16_t*)*duraTime;														// duration time if given
	rampTme = (uint16_t)rampTime[0] << 8 | (uint16_t)rampTime[1];							// if ramp time is given, bring it in the right format
	duraTme = (uint16_t)duraTime[0] << 8 | (uint16_t)duraTime[1];							// duration time if given

	if (rampTme) active_tr11 = 1;															// indicate we are coming from trigger11
	else modStat = setValue;																// otherwise set the value directly

	if (duraTme) active_tr11 = 1;															// set tr11 flag active to be processed in the poll function

	delayTmr.set( intTimeCvt(rampTme) );													// set the timer accordingly, could be 0 or a time
	value_tr11 = setValue;																	// forward the value to be set

	#ifdef RL_DBG																			// some debug
	dbg << F("trigger11, setValue:") << setValue << F(", rampTime:") << intTimeCvt(rampTme) << F(", duraTime:") << intTimeCvt(duraTme) << '\n';
	#endif 
}

/**
* @brief Function is called on messages comming from a remote or push button.
*        No need to call the function from outside as it is
*        called from the module registrar
*
* @param msgLng 1 byte containing the long message flag but also low battery
* @param msgCnt 1 byte containing the message counter of the sender
*
*/
void cmSwitch::trigger40(uint8_t msgLng, uint8_t msgCnt) {

	// some sanity
	uint8_t isLng = (msgLng & 0x40) ? 1 : 0;												// is it a long message?
	l3 = (isLng) ? (s_l3*)&lstPeer + 11 : (s_l3*)&lstPeer;									// set short or long struct portion
	delayTmr.set(0);																		// also delay timer is not needed any more
	active_tr11 = 0;																		// stop any tr11 processing
	
	// check for multi execute flag
	if ((isLng) && (cnt == msgCnt) && (!lstPeer.LONG_MULTIEXECUTE)) return;					// trigger was a repeated long, but we have no multi execute, so return
	cnt = msgCnt;																			// remember message counter

	// check against action type
	if        (l3->ACTION_TYPE == ACTION::INACTIVE) {

	} else if (l3->ACTION_TYPE == ACTION::JUMP_TO_TARGET) {
		// set next status depending on current status
		if      (curStat == JT::ONDELAY)  nxtStat = l3->JT_ONDELAY;							// delay on
		else if (curStat == JT::ON)       nxtStat = l3->JT_ON;								// on
		else if (curStat == JT::OFFDELAY) nxtStat = l3->JT_OFFDELAY;						// delay off
		else if (curStat == JT::OFF)      nxtStat = l3->JT_OFF;								// currently off

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_TO_COUNTER) {
		modStat = (msgCnt % 2) ? 200 : 0;													// set the relay status depending on message counter

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_INV_TO_COUNTER) {
		modStat = (msgCnt % 2) ? 0 : 200;													// set the relay status depending on message counter
	}

	#ifdef RL_DBG
	dbg << F("trigger40, msgLng:") << msgLng << F(", msgCnt:") << msgCnt << F(",ACTION_TYPE:") << l3->ACTION_TYPE << F(", curStat:") << curStat << F(", nxtStat:") << nxtStat << F(", JT_ONDELAY:") << _HEXB(l3->JT_ONDELAY) << F(", JT_ON:") << _HEXB(l3->JT_ON) << F(", JT_OFFDELAY:") << _HEXB(l3->JT_OFFDELAY) << F(", JT_OFF:") << _HEXB(l3->JT_OFF) << '\n';
	#endif
}

/**
* @brief Function is called on messages comming from sensors.
*        No need to call the function from outside as it is 
*        called from the module registrar
*
* @param msgBLL 1 byte containing the long message flag
* @param msgCnt 1 byte containing the message counter of the sender
* @param msgVal 1 bayte with the value of the sensor
*
*/
void cmSwitch::trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal) {

	uint8_t isLng = (msgBLL & 0x40)?1:0;													// is it a long message?
	uint8_t ctTbl;																			// to select the condition depending on current device status
	
	// set short or long
	l3 = (isLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;										// set pointer to the right part of the list3, short or long


	// set condition table in conjunction of the current jump table status
	if      (curStat == JT::ONDELAY)  ctTbl = l3->CT_ONDELAY;								// condition table delay on
	else if (curStat == JT::ON)       ctTbl = l3->CT_ON;									// condition table on
	else if (curStat == JT::OFFDELAY) ctTbl = l3->CT_OFFDELAY;								// condition table delay off
	else if (curStat == JT::OFF)      ctTbl = l3->CT_OFF;									// condition table off
	
	#ifdef RL_DBG
	dbg << F("trigger41, curStat:") << curStat  << F(", nxtStat:") << ctTbl << F(", val:") << msgVal << F(", HI:") << l3->COND_VALUE_HI << F(", LO:") << l3->COND_VALUE_LO << '\n';
	#endif

	if      (ctTbl == CT::X_GE_COND_VALUE_LO)                       
		if ( msgVal >= l3->COND_VALUE_LO) trigger40(isLng, msgCnt);
	
	else if (ctTbl == CT::X_GE_COND_VALUE_HI)
		if ( msgVal >= l3->COND_VALUE_HI) trigger40(isLng, msgCnt);
	
	else if (ctTbl == CT::X_LT_COND_VALUE_LO)
		if ( msgVal <  l3->COND_VALUE_LO) trigger40(isLng, msgCnt);
	
	else if (ctTbl == CT::X_LT_COND_VALUE_HI)
		if ( msgVal <  l3->COND_VALUE_HI) trigger40(isLng, msgCnt);
	
	else if (ctTbl == CT::COND_VALUE_LO_LE_X_LT_COND_VALUE_HI)
		if ( ( l3->COND_VALUE_LO <= msgVal) && (msgVal <  l3->COND_VALUE_HI) ) trigger40(isLng, msgCnt);
	
	else if (ctTbl == CT::X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI) 
		if ( (msgVal < l3->COND_VALUE_LO) || (msgVal >= l3->COND_VALUE_HI) ) trigger40(isLng, msgCnt);
}

void cmSwitch::adjustStatus(void) {

	if (modStat == setStat) return;															// nothing to do, return
	//dbg << "m" << modStat << " s" << setStat << '\n';

	setStat = modStat;																		// follow action
	switchSwitch(regCnl, setStat);															// calling the external function to make it happen

	msgTmr.set(0);																			// send status was set before, 0 the timer to send the status
}

void cmSwitch::sendStatus(void) {

	if (!sendStat) return;																	// nothing to do
	if (!msgTmr.done()) return;																// not the right time
	
	// prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80
	if      (modStat == setStat) modDUL  = 0;
	//else if (modStat <  setStat) modDUL  = 0x10;
	//else if (modStat >  setStat) modDUL  = 0x20;
	if (!delayTmr.done() )       modDUL |= 0x40;
	
	// check which type has to be send - if it is an ACK and modDUL != 0, then set timer for sending a actuator status
	if      (sendStat == INFO::ACK_STATUS)      hm.sendACK_STATUS(regCnl, modStat, modDUL);	
	else if (sendStat == INFO::ACTUATOR_STATUS) hm.sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);	

	// check if it is a stable status, otherwise schedule next info message
	if (modDUL)  {																			// status is currently changing
		sendStat = INFO::ACTUATOR_STATUS;													// send next time a info status message
		msgTmr.set(delayTmr.remain()+5);

	} else sendStat = INFO::NOTHING;														// no need for next time
}

void cmSwitch::pollModule(void) {

	adjustStatus();																			// check if something is to be set on the Relay channel
	sendStatus();																			// check if there is some status to send

	// check if something is to do on the switch
	if (!delayTmr.done() ) return;															// timer not done, wait until then

	// - trigger11, check if rampTime or onTimer is set
	if (active_tr11) {
		if (rampTme) {																		// ramp timer was set, now we have to set the value
			modStat = value_tr11;															// set the value we had stored
			active_tr11 = 0;																// reset tr11, if dura time is set, we activate again
			rampTme = 0;																	// not necessary to do it again
		}

		if (duraTme) {																		// coming from trigger 11, we should set the duration period
			delayTmr.set(intTimeCvt(duraTme));												// set the duration time
			active_tr11 = 1;
			duraTme = 0;																	// but indicate, it was done

		} else {																			// check if something is to do from trigger11
			modStat = value_tr11 ^ 200;														// invert the status
			active_tr11 = 0;																// trigger11 ready
		}
	}

	

	// - jump table section for trigger40/41
	if (l3->ACTION_TYPE != ACTION::JUMP_TO_TARGET) return;									// only valid for jump table
	if (curStat == nxtStat) return;															// no status change, leave
	curStat = nxtStat;																		// seems next status is different to current, remember for next poll

	// check the different status changes
	if        (nxtStat == JT::ONDELAY) {	
		#ifdef RL_DBG
		dbg << F("dlyOn\n");
		#endif
		
		nxtStat = l3->JT_ONDELAY;															// get next status from jump table
		if (l3->ONDELAY_TIME) {																// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ONDELAY_TIME));									// activate the timer and set next status
		}

	} else if (nxtStat == JT::ON) {	
		#ifdef RL_DBG
		dbg << F("on\n");
		#endif
		
		nxtStat = l3->JT_ON;																// get next status from jump table
		modStat = 200;																		// switch relay on
		if ((l3->ON_TIME) && (l3->ON_TIME != NOT_USED)) {									// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ON_TIME));											// activate the timer and set next status
			nxtStat = l3->JT_OFF = JT::OFF;													// go to off and keep status till we get the next trigger40/41 message
		} 

	} else if (nxtStat == JT::OFFDELAY) {
		#ifdef RL_DBG
		dbg << F("dlyOff\n");
		#endif

		nxtStat = l3->JT_OFFDELAY;															// get jump table for next status
		if (l3->OFFDELAY_TIME) {															// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFFDELAY_TIME));									// activate the timer and set next status
		}

	} else if (nxtStat == JT::OFF) {
		#ifdef RL_DBG
		dbg << F("off\n");
		#endif

		nxtStat = l3->JT_OFF;																// get the next status from jump table
		modStat = 0;																		// switch off relay
		if ((l3->OFF_TIME) && (l3->OFF_TIME != NOT_USED)) {									// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFF_TIME));										// activate the timer and set next status
			nxtStat = l3->JT_ON = JT::ON;													// go to on and keep till we get the next trigger40/41
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void cmSwitch::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef RL_DBG
	dbg << F("RL\n");
	#endif

	modStat ^= 200;																			// xor the relay status
	sendStat = INFO::ACTUATOR_STATUS;														// send next time a info status message
}

void cmSwitch::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef RL_DBG
	dbg << F("CCE, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif
}

void cmSwitch::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd and 3rd byte = ramp time; 4th and 5th byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef RL_DBG
	dbg << F("PSE, value:") << _HEXB(data[0]);
	if (len > 1) { dbg << F(", rampTime: ") << _HEX((data+1), 2); };
	if (len > 3) { dbg << F(", duraTime: ") << _HEX((data+3), 2); };
	dbg << '\n';
	#endif

	trigger11(data[0], (len > 1)?data+1:NULL, (len > 3)?data+3:NULL);

	sendStat = INFO::ACK_STATUS;															// ACK should be send
	msgTmr.set(10);																			// give some time

}

void cmSwitch::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef RL_DBG
	dbg << F("PSR\n");
	#endif
	
	sendStat = INFO::ACTUATOR_STATUS;														// send next time a info status message
	msgTmr.set(10);																			// wait a short time to set status

}

void cmSwitch::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef RL_DBG
	dbg << F("PME, type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
	if      (type == 0x3E) trigger40( data[4], data[5]);
	else if (type == 0x40) trigger40((data[0] & 0x40), data[1]);
	else if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);
	
	sendStat = INFO::ACK_STATUS;															// send next time a ack info message
	msgTmr.set(10);																			// wait a short time to set status
}

void cmSwitch::poll(void) {
	pollModule();
}


//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------

void cmSwitch::regInHM(uint8_t cnl) {
	RG::s_modTable *pModTbl = &modTbl[cnl];													// pointer to the respective line in the module table
	pModTbl->isActive = 1;
	pModTbl->mDlgt = myDelegate::from_function<CLASS_NAME, &CLASS_NAME::hmEventCol>(this);
	pModTbl->lstCnl = (uint8_t*)&lstCnl;
	pModTbl->lstPeer = (uint8_t*)&lstPeer;

	hm.ee.getList(cnl, 1, 0, pModTbl->lstCnl);												// load list1 in the respective buffer
	regCnl = cnl;																			// stores the channel we are responsible fore
	
	config();																				// initialize the module settings
}

void cmSwitch::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	// dbg << "by3:" << by3 << " by10:" << by10 << " d:" << pHex(data, len) << '\n'; _delay_ms(100);
	if (by3 == 0x00) {				// system call
		if (by10 == 0x00) poll();
		if (by10 == 0x01) setToggle();
		if (by10 == 0x02) updatePeerDefaults(by11, data, len);

	} else if (by3 == 0x01) {
		if (by11 == 0x06) configCngEvent();
		if (by11 == 0x0E) pairStatusReq();
		if (by11 == 0x01) peerAddEvent(data, len);

	} else if (by3 == 0x11) {
		if (by10 == 0x02) pairSetEvent(data, len);

	} else if (by3 >= 0x3E) {
		peerMsgEvent(by3, data, len);
	}
}


/**
* This function will be called by the eeprom module as a request to update the
* list3 structure by the default values per peer channel for the user module.
* Overall defaults are already set to the list3/4 by the eeprom class, here it 
* is only about peer channel specific deviations.
* As we get this request for each peer channel we don't need the peer index.
* Setting defaults could be done in different ways but this should be the easiest...
*
* Byte     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21
* ADDRESS  02 03 04 05 06 07 08 09 0a 0b 0c 82 83 84 85 86 87 88 89 8a 8b 8c
* DEFAULT  00 00 32 64 00 ff 00 ff 01 44 44 00 00 32 64 00 ff 00 ff 21 44 44
* to Change                           14 63                            14 63
* OFF                                 64 66                            64  66
* ON                                  13 33                            13  33
* TOGGLE   0B 14  0C 63  8B 14  8C 63
* As we have to change only 4 bytes, we can map the struct to a byte array point
* and address/change the bytes direct. EEprom gets updated by the eeprom class
* automatically.
*/
void cmSwitch::updatePeerDefaults(uint8_t by11, uint8_t *data, uint8_t len) {
	
	uint8_t *lstP = (uint8_t*)&lstPeer;														// cast the struct in a byte array

	// if both peer channels are given, peer channel 01 default is the off dataset, peer channel 02 default is the on dataset
	// if only one peer channel is given, then the default dataset is toogle
	if ((data[0]) && (data[1])) {	// dual peer add

		if (by11 % 2) {				// odd (1,3,5..) means OFF								// if this returns 1 it has detected an odd number(1,3,5..)
			lstP[9] = lstP[20] = 0x64;														// set some byte
			lstP[10] = lstP[21] = 0x66;
		} else {					// even (2,4,6..) means ON
			lstP[9] = lstP[20] = 0x13;														// set some byte
			lstP[10] = lstP[21] = 0x33;
		}

	} else  {						// toggle peer channel
		lstP[9]  = lstP[20] = 0x14;															// set some byte
		lstP[10] = lstP[21] = 0x63;
	} 

	#ifdef RL_DBG
	dbg << F("updatePeerDefaults: pCnl1: ") << _HEXB(data[0]) << F(", pCnl2: ") << _HEXB(data[1]) << F(", cur_pCnl: ") << _HEXB(by11) << '\n';
	#endif
}

/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
* no need for sending an answer here, for information only
*/
void cmSwitch::peerAddEvent(uint8_t *data, uint8_t len) {
	#ifdef RL_DBG
	dbg << F("peerAddEvent: peer: ") << _HEX(data, 3) << F(", pCnl1: ") << _HEXB(data[3]) << F(", pCnl2: ") << _HEXB(data[4]) << '\n';
	#endif
}
