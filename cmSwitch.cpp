/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Switch ------------------------------------------------------------------------------------------
* - with a lot of support from martin876 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module switch
*        pointer to channel table are forwarded to the master class. 
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/
#include "cmSwitch.h"

cmSwitch::cmSwitch(const uint8_t peer_max) : cmMaster(peer_max) {

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cmSwitch_ChnlReg;
	lstC.def = cmSwitch_ChnlDef;
	lstC.len = sizeof(cmSwitch_ChnlReg);

	lstP.lst = 3;																			// setup the peer list with all dependencies
	lstP.reg = cmSwitch_PeerReg;
	lstP.def = cmSwitch_PeerDef;
	lstP.len = sizeof(cmSwitch_PeerReg);

	lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	lstP.val = new uint8_t[lstP.len];


	l1 = (s_l1*)&lstC.val;																	// set list structures to something useful
	l3 = (s_l3*)&lstP.val;																	// reduced l3, description in cmSwitch.h at struct declaration
	l3F = (s_lstPeer*)&lstP.val;

	l3->ACTION_TYPE = ACTION::INACTIVE;														// and secure that no action will happened in polling function
	active_tr11 = 0;

	setStat = 0;																			// we start allways with status off
	modStat = 0;																			// output to 0
	curStat = JT::OFF;																		// initialize the jump table value

	initSwitch(lstC.cnl);																	// call external init function to set the output pins

	msgDelay = (rand() % 2000) + 1000;														// set message delay
	msgTmr.set(msgDelay);																	// wait some time to settle the device
	sendStat = INFO::SND_ACTUATOR_STATUS;													// send the initial status info

	DBG(SW, F("cmSwitch, cnl: "), lstC.cnl, '\n');
}


/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Function is called on messages comming from a central device.
*
* @param setValue 1 byte containing the value we have to set
* @param rampTime Pointer to 2 byte array containing the encoded ramptime value
* @param duraTime Pointer to 2 byte array containing the encoded durationtime value
*
*/
void cmSwitch::message_trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *duraTime) {

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

	sendStat = INFO::SND_ACK_STATUS;														// ACK should be send
	msgTmr.set(10);																			// give some time

	DBG(SW, F("trigger11, setValue:"), setValue, F(", rampTime:"), intTimeCvt(rampTme), F(", duraTime:"), intTimeCvt(duraTme), '\n' );
}

/**
* @brief Function is called on messages comming from master, simulating a remote or push button.
*
* @param msgLng 1 byte containing the long message flag
* @param msgCnt 1 byte containing the message counter of the sender
*
*/
void cmSwitch::message_trigger3E(uint8_t msgLng, uint8_t msgCnt) {
	message_trigger40(msgLng, msgCnt);
}

/**
* @brief Function is called on messages comming from a remote or push button.
*
* @param msgLng 1 byte containing the long message flag
* @param msgCnt 1 byte containing the message counter of the sender
*
*/
void cmSwitch::message_trigger40(uint8_t msgLng, uint8_t msgCnt) {
	//dbg << "x: " << _HEX(peer_list,22) << '\n';

	l3 = (msgLng) ? (s_l3*)lstP.val + 11 : (s_l3*)lstP.val;									// set short or long struct portion

	delayTmr.set(0);																		// also delay timer is not needed any more
	active_tr11 = 0;																		// stop any tr11 processing
	
	// check for multi execute flag
	if ((msgLng) && (cnt == msgCnt) && (!l3F->LONG_MULTIEXECUTE)) return;					// trigger was a repeated long, but we have no multi execute, so return
	cnt = msgCnt;																			// remember message counter

	// check against action type
	if        ( l3->ACTION_TYPE == ACTION::INACTIVE ) {

	} else if ( l3->ACTION_TYPE == ACTION::JUMP_TO_TARGET ) {
		// set next status depending on current status
		if      ( curStat == JT::ONDELAY )  nxtStat = l3->JT_ONDELAY;						// delay on
		else if ( curStat == JT::ON )       nxtStat = l3->JT_ON;							// on
		else if ( curStat == JT::OFFDELAY ) nxtStat = l3->JT_OFFDELAY;						// delay off
		else if ( curStat == JT::OFF )      nxtStat = l3->JT_OFF;							// currently off

	} else if ( l3->ACTION_TYPE == ACTION::TOGGLE_TO_COUNTER ) {
		modStat = (msgCnt % 2) ? 200 : 0;													// set the relay status depending on message counter

	} else if ( l3->ACTION_TYPE == ACTION::TOGGLE_INV_TO_COUNTER ) {
		modStat = (msgCnt % 2) ? 0 : 200;													// set the relay status depending on message counter
	}

	sendStat = INFO::SND_ACK_STATUS;														// send next time a ack info message
	msgTmr.set(10);																			// wait a short time to set status
	
	DBG(SW, F("trigger40, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, F(",ACTION_TYPE:"), l3->ACTION_TYPE, F(", curStat:"), curStat, F(", nxtStat:"), nxtStat, F(", JT_ONDELAY:"), _HEXB(l3->JT_ONDELAY), F(", JT_ON:"), _HEXB(l3->JT_ON), F(", JT_OFFDELAY:"), _HEXB(l3->JT_OFFDELAY), F(", JT_OFF:"), _HEXB(l3->JT_OFF), '\n' );
}

/**
* @brief Function is called on messages comming from sensors.
*
* @param msgLng 1 byte containing the long message flag
* @param msgCnt 1 byte containing the message counter of the sender
* @param msgVal 1 bayte with the value of the sensor
*
*/
void cmSwitch::message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal) {

	l3 = (msgLng)?(s_l3*)lstP.val +11 :(s_l3*)lstP.val;										// set pointer to the right part of the list3, short or long

	// set condition table in conjunction of the current jump table status
	uint8_t ctTbl;																			// to select the condition depending on current device status

	if      ( curStat == JT::ONDELAY )  ctTbl = l3->CT_ONDELAY;								// condition table delay on
	else if ( curStat == JT::ON )       ctTbl = l3->CT_ON;									// condition table on
	else if ( curStat == JT::OFFDELAY ) ctTbl = l3->CT_OFFDELAY;							// condition table delay off
	else if ( curStat == JT::OFF )      ctTbl = l3->CT_OFF;									// condition table off
	
	DBG(SW, F("trigger41, curStat:"), curStat, F(", nxtStat:"), ctTbl, F(", val:"), msgVal, F(", HI:"), l3->COND_VALUE_HI, F(", LO:"), l3->COND_VALUE_LO, '\n' );

	if      ( ctTbl == CT::X_GE_COND_VALUE_LO )                      
		if ( msgVal >= l3->COND_VALUE_LO) message_trigger40(msgLng, msgCnt);
	
	else if ( ctTbl == CT::X_GE_COND_VALUE_HI )
		if ( msgVal >= l3->COND_VALUE_HI) message_trigger40(msgLng, msgCnt);
	
	else if ( ctTbl == CT::X_LT_COND_VALUE_LO )
		if ( msgVal <  l3->COND_VALUE_LO) message_trigger40(msgLng, msgCnt);
	
	else if ( ctTbl == CT::X_LT_COND_VALUE_HI )
		if ( msgVal <  l3->COND_VALUE_HI) message_trigger40(msgLng, msgCnt);
	
	else if ( ctTbl == CT::COND_VALUE_LO_LE_X_LT_COND_VALUE_HI )
		if ( ( l3->COND_VALUE_LO <= msgVal) && (msgVal <  l3->COND_VALUE_HI) ) message_trigger40(msgLng, msgCnt);
	
	else if ( ctTbl == CT::X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI ) 
		if ( (msgVal < l3->COND_VALUE_LO) || (msgVal >= l3->COND_VALUE_HI) ) message_trigger40(msgLng, msgCnt);
}

void cmSwitch::adjustStatus(void) {

	if (modStat == setStat) return;															// nothing to do, return
	//dbg << "m" << modStat << " s" << setStat << '\n';

	setStat = modStat;																		// follow action
	switchSwitch(lstC.cnl, setStat);														// calling the external function to make it happen

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
	if      ( sendStat == INFO::SND_ACK_STATUS )      send_ACK_STATUS(lstC.cnl, modStat, modDUL);	
	else if ( sendStat == INFO::SND_ACTUATOR_STATUS ) hm.sendINFO_ACTUATOR_STATUS(lstC.cnl, modStat, modDUL);

	// check if it is a stable status, otherwise schedule next info message
	if (modDUL)  {																			// status is currently changing
		sendStat = INFO::SND_ACTUATOR_STATUS;												// send next time a info status message
		msgTmr.set(delayTmr.remain()+5);

	} else sendStat = INFO::NOTHING;														// no need for next time
}

void cmSwitch::cm_poll(void) {

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
	if ( l3->ACTION_TYPE != ACTION::JUMP_TO_TARGET ) return;								// only valid for jump table
	if ( curStat == nxtStat ) return;														// no status change, leave
	curStat = nxtStat;																		// seems next status is different to current, remember for next poll

	// check the different status changes
	if        ( nxtStat == JT::ONDELAY ) {	
		DBG(SW, F("dlyOn\n") );
		
		nxtStat = l3->JT_ONDELAY;															// get next status from jump table
		if (l3->ONDELAY_TIME) {																// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ONDELAY_TIME));									// activate the timer and set next status
		}

	} else if ( nxtStat == JT::ON ) {	
		DBG(SW, F("on\n") );
		
		nxtStat = l3->JT_ON;																// get next status from jump table
		modStat = 200;																		// switch relay on
		if ((l3->ON_TIME) && (l3->ON_TIME != NOT_USED)) {									// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ON_TIME));											// activate the timer and set next status
			nxtStat = l3->JT_OFF = JT::OFF;													// go to off and keep status till we get the next trigger40/41 message
		} 

	} else if ( nxtStat == JT::OFFDELAY ) {
		DBG(SW, F("dlyOff\n") );

		nxtStat = l3->JT_OFFDELAY;															// get jump table for next status
		if (l3->OFFDELAY_TIME) {															// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFFDELAY_TIME));									// activate the timer and set next status
		}

	} else if ( nxtStat == JT::OFF ) {
		DBG(SW, F("off\n") );

		nxtStat = l3->JT_OFF;																// get the next status from jump table
		modStat = 0;																		// switch off relay
		if ((l3->OFF_TIME) && (l3->OFF_TIME != NOT_USED)) {									// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFF_TIME));										// activate the timer and set next status
			nxtStat = l3->JT_ON = JT::ON;													// go to on and keep till we get the next trigger40/41
		}
	}
}


void cmSwitch::set_toggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	DBG(SW, F("set_toggle\n") );

	modStat ^= 200;																			// xor the relay status
	curStat = (modStat) ? JT::ON : JT::OFF;
	sendStat = INFO::SND_ACTUATOR_STATUS;													// send next time a info status message
}




void cmSwitch::request_pair_status(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	DBG(SW, F("request_pair_status\n") );
	
	sendStat = INFO::SND_ACTUATOR_STATUS;													// send next time a info status message
	msgTmr.set(10);																			// wait a short time to set status

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
* OFF                                 64 66                            64 66
* ON                                  13 33                            13 33
* TOGGLE   0B 14  0C 63  8B 14  8C 63
* As we have to change only 4 bytes, we can map the struct to a byte array point
* and address/change the bytes direct. EEprom gets updated by the eeprom class
* automatically.
*/

void cmSwitch::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {

	// if both peer channels are given, peer channel 01 default is the off dataset, peer channel 02 default is the on dataset
	// if only one peer channel is given, then the default dataset is toogle
	if (( buf->PEER_CNL[0] ) && ( buf->PEER_CNL[1] )) {		// dual peer add

		if (idx % 2) {										// odd (1,3,5..) means OFF
			lstP.val[9] = lstP.val[20] = 0x64;														// set some byte
			lstP.val[10] = lstP.val[21] = 0x66;
		} else {											// even (2,4,6..) means ON
			lstP.val[9] = lstP.val[20] = 0x13;														// set some byte
			lstP.val[10] = lstP.val[21] = 0x33;
		}

	} else  {												// toggle peer channel
		lstP.val[9]  = lstP.val[20] = 0x14;															// set some byte
		lstP.val[10] = lstP.val[21] = 0x63;
	} 

	DBG(SW, F("cmSwitch:request_peer_defaults CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", idx:"), _HEXB(idx), '\n' );
}

