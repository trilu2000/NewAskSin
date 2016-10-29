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


	l1 = (s_l1*)lstC.val;																	// set list structures to something useful
	l3 = (s_l3*)lstP.val;																	// reduced l3, description in cmSwitch.h at struct declaration
	l3F = (s_lstPeer*)lstP.val;

	l3->ACTION_TYPE = ACTION::INACTIVE;														// and secure that no action will happened in polling function
	tr11.active = 0;																		// empty trigger 11 store
	tr40.cur = JT::OFF;																		// default off
	tr40.nxt = JT::OFF;																		// default off

	cm_status.value = 0;																	// output to 0
	cm_status.set_value = 0;

	initSwitch(lstC.cnl);																	// call external init function to set the output pins

	cm_status.message_delay.set((rand() % 2000) + 1000);									// wait some time to settle the device
	cm_status.message_type = INFO::SND_ACTUATOR_STATUS;										// send the initial status info

	DBG(SW, F("cmSwitch, cnl: "), lstC.cnl, '\n');
}


/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */


void cmSwitch::adjustStatus(void) {

	if (cm_status.value == cm_status.set_value) return;										// nothing to do, return
	//dbg << "m" << modStat << " s" << setStat << '\n';

	switchSwitch(lstC.cnl, cm_status.set_value);											// calling the external function to make it happen
	cm_status.value = cm_status.set_value;													// remember that it was done

}

void cmSwitch::cm_poll(void) {

	adjustStatus();																			// check if something is to be set on the Relay channel

	// check if something is to do on the switch
	if (!cm_status.delay.done() ) return;													// timer not done, wait until then

	// - trigger11, check if rampTime or onTimer is set
	if (tr11.active) {
		if (tr11.ramp_time) {																// ramp timer was set, now we have to set the value
			cm_status.set_value = tr11.value;												// set the value we had stored
			tr11.active = 0;																// reset tr11, if dura time is set, we activate again
			tr11.ramp_time = 0;																// not necessary to do it again
		} 
		
		if (tr11.dura_time) {																// coming from trigger 11, we should set the duration period
			cm_status.delay.set(intTimeCvt(tr11.dura_time));								// set the duration time
			tr11.active = 1;																// we have set the timer so remember that it was from tr11
			tr11.dura_time = 0;																// but indicate, it was done

		} else {																			// check if something is to do from trigger11
			cm_status.set_value = tr11.value ^ 200;											// invert the status
			tr11.active = 0;																// trigger11 ready
		}
		tr40.cur = (cm_status.set_value) ? JT::ON : JT::OFF;								// set tr40 status, otherwise a remote will not work
	}


	// - jump table section for trigger3E/40/41
	if ( l3->ACTION_TYPE != ACTION::JUMP_TO_TARGET ) return;								// only valid for jump table
	if (tr40.cur == tr40.nxt) return;														// no status change, leave
	tr40.cur = tr40.nxt;																	// seems next status is different to current, remember for next poll

	// check the different status changes
	if        (tr40.nxt == JT::ONDELAY ) {
		DBG(SW, F("dlyOn\n") );
		if (l3->ONDELAY_TIME != NOT_USED) {													// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->ONDELAY_TIME));								// activate the timer and set next status
			tr40.nxt = l3->JT_ONDELAY;														// get next status from jump table
		}

	} else if (tr40.nxt == JT::ON ) {
		DBG(SW, F("on\n") );
		cm_status.set_value = 200;															// switch relay on
		if (l3->ON_TIME != NOT_USED) {														// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->ON_TIME));									// set the timer while not for ever
			tr40.nxt = l3->JT_ON;															// set next status
		}
		 

	} else if (tr40.nxt == JT::OFFDELAY ) {
		DBG(SW, F("dlyOff\n") );
		if (l3->OFFDELAY_TIME != NOT_USED) {												// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->OFFDELAY_TIME));							// activate the timer and set next status
			tr40.nxt = l3->JT_OFFDELAY;														// get jump table for next status
		}

	} else if (tr40.nxt == JT::OFF ) {
		DBG(SW, F("off\n") );
		cm_status.set_value = 0;															// switch off relay
		if (l3->OFF_TIME != NOT_USED) {														// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->OFF_TIME));									// activate the timer and set next status
			tr40.nxt = l3->JT_OFF;															// get the next status from jump table
		}
	}
}


void cmSwitch::set_toggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	DBG(SW, F("set_toggle\n") );

	if (cm_status.value)  cm_status.set_value = 0;											// if its on, we switch off
	else cm_status.set_value = 200;

	//tr40.cur = (send_stat.modStat) ? JT::ON : JT::OFF;
	cm_status.message_type = INFO::SND_ACTUATOR_STATUS;										// send next time a info status message
	cm_status.message_delay.set(50);
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
			lstP.val[9] = lstP.val[20] = 0x64;												// set some byte
			lstP.val[10] = lstP.val[21] = 0x66;
		} else {											// even (2,4,6..) means ON
			lstP.val[9] = lstP.val[20] = 0x13;												// set some byte
			lstP.val[10] = lstP.val[21] = 0x33;
		}

	} else  {												// toggle peer channel
		lstP.val[9]  = lstP.val[20] = 0x14;													// set some byte
		lstP.val[10] = lstP.val[21] = 0x63;
	} 

	DBG(SW, F("cmSwitch:request_peer_defaults CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", idx:"), _HEXB(idx), '\n' );
}

/*
* @brief Received message handling forwarded by AS::processMessage
*/
void cmSwitch::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	cm_status.message_type = INFO::SND_ACTUATOR_STATUS;										// send next time a info status message
	cm_status.message_delay.set(50);														// wait a short time to set status

	DBG(SW, F("SW:CONFIG_STATUS_REQUEST\n"));
}
/*
* @brief INSTRUCTION_SET is called on messages comming from a central device to setup a channel status
* The message contains at least the value which has to be set, but there a two further 2 byte values which 
* represents a ramp timer and a duration timer. We differentiate the messages by the len byte
* 12 byte - value
* 14 byte - value, ramp_time
* 16 byte - value, ramp_time, dura_time
*/ 
void cmSwitch::INSTRUCTION_SET(s_m1102xx *buf) {
	l3->ACTION_TYPE = ACTION::INACTIVE;														// action type to off otherwise the polling function will overwrite

	/* fill the struct depending on the message length */
	tr11.value = buf->VALUE;
	tr11.ramp_time = (buf->MSG_LEN >= 14) ? buf->RAMP_TIME : 0;								// get the ramp time if message len indicates that it is included
	tr11.dura_time = (buf->MSG_LEN >= 16) ? buf->DURA_TIME : 0;								// get the dura time if message len indicates that it is included

	if (tr11.ramp_time) tr11.active = 1;													// indicate we are coming from trigger11
	else cm_status.set_value = tr11.value;													// otherwise set the value directly
	cm_status.delay.set(intTimeCvt(tr11.ramp_time));										// set the timer accordingly, could be 0 or a time

	if (tr11.dura_time) tr11.active = 1;													// set tr11 flag active to be processed in the poll function

	cm_status.message_type = INFO::SND_ACK_STATUS;											// ACK should be send
	cm_status.message_delay.set(100);														// give some time

	DBG(SW, F("INSTRUCTION_SET, setValue:"), tr11.value, F(", rampTime:"), intTimeCvt(tr11.ramp_time), F(", duraTime:"), intTimeCvt(tr11.dura_time), '\n');
}
/**
* @brief Function is called on messages comming from master, simulating a remote or push button.
* restructure the message and forward it to the local cmSwitch::REMOTE(s_m40xxxx *buf) function...
* -> 0F 09 B0 3E 63 19 64 33 11 22 23 70 D8 40 01 1D 
*/
void cmSwitch::SWITCH(s_m3Exxxx *buf) {
	/* as we receive a s_m3Exxxx message and have to forward this message to the REMOTE function,
	* we have to adjust the content, could be done by generating a new string or casting the message 
	* message cast seems to be easier and more efficient... */
	//s_m40xxxx *x = (s_m40xxxx*)(((uint8_t*)buf)+4);
	REMOTE((s_m40xxxx*)(((uint8_t*)buf) + 4));
}
/**
* @brief Function is called on messages comming from a remote or push button.
* within the message we will find one byte, BLL, which reflects a bit array.
* Bit 1 to 6 are called Button, which reflects the channel of the sender device
* Bit 7 indicates if the button was pressed long
* Bit 8 is not interesting for us, because it reflects the battery status of the peered devices, 
* more interesting for the master...
*/
void cmSwitch::REMOTE(s_m40xxxx *buf) {
	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)lstP.val + 11 : (s_l3*)lstP.val;							// set short or long struct portion
	//dbg << "remote string: " << _HEX((uint8_t*)buf, 12) << '\n';

	cm_status.delay.set(0);																	// also delay timer is not needed any more
	tr11.active = 0;																		// stop any tr11 processing

	// check for multi execute flag
	if ((buf->BLL.LONG) && (tr40.cnt == buf->COUNTER) && (!l3F->LONG_MULTIEXECUTE)) return;	// trigger was a repeated long, but we have no multi execute, so return
	tr40.cnt = buf->COUNTER;																// remember message counter

	// check against action type
	if (l3->ACTION_TYPE == ACTION::INACTIVE) {

	} else if (l3->ACTION_TYPE == ACTION::JUMP_TO_TARGET) {
		// set next status depending on current status
		if      (tr40.cur == JT::ONDELAY)  tr40.nxt = l3->JT_ONDELAY;						// delay on
		else if (tr40.cur == JT::ON)       tr40.nxt = l3->JT_ON;							// on
		else if (tr40.cur == JT::OFFDELAY) tr40.nxt = l3->JT_OFFDELAY;						// delay off
		else if (tr40.cur == JT::OFF)      tr40.nxt = l3->JT_OFF;							// currently off

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_TO_COUNTER) {
		cm_status.set_value = (buf->COUNTER % 2) ? 200 : 0;									// set the relay status depending on message counter

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_INV_TO_COUNTER) {
		cm_status.set_value = (buf->COUNTER % 2) ? 0 : 200;									// set the relay status depending on message counter
	}

	cm_status.message_type = INFO::SND_ACK_STATUS;											// send next time a ack info message
	cm_status.message_delay.set(100);														// wait a short time to set status

	/* some debug */
	DBG(SW, F("trigger40, msgLng:"), buf->BLL.LONG, F(", msgCnt:"), buf->COUNTER, F(", ACTION_TYPE:"), l3->ACTION_TYPE, F(", curStat:"), tr40.cur, F(", nxtStat:"), tr40.nxt, '\n');
	DBG(SW, F("JT_ONDELAY:"), _HEXB(l3->JT_ONDELAY), F(", ONDELAY_T:"), _HEXB(l3->ONDELAY_TIME), F(", JT_ON:"), _HEXB(l3->JT_ON), F(", ON_T:"), _HEXB(l3->ON_TIME), F(", JT_OFFDELAY:"), _HEXB(l3->JT_OFFDELAY), F(", OFFDELAY_T:"), _HEXB(l3->OFFDELAY_TIME), F(", JT_OFF:"), _HEXB(l3->JT_OFF), F(", OFF_T:"), _HEXB(l3->OFF_TIME), '\n');
	DBG(SW, F("lst3: "), _HEX(lstP.val, lstP.len), '\n');
}
/**
* @brief Function is called on messages comming from sensors.
*/
void cmSwitch::SENSOR_EVENT(s_m41xxxx *buf) {
	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)lstP.val + 11 : (s_l3*)lstP.val;							// set short or long struct portion

	/* set condition table in conjunction of the current jump table status */
	uint8_t ctTbl;																			// to select the condition depending on current device status

	if      (tr40.cur == JT::ONDELAY)  ctTbl = l3->CT_ONDELAY;								// condition table delay on
	else if (tr40.cur == JT::ON)       ctTbl = l3->CT_ON;									// condition table on
	else if (tr40.cur == JT::OFFDELAY) ctTbl = l3->CT_OFFDELAY;								// condition table delay off
	else if (tr40.cur == JT::OFF)      ctTbl = l3->CT_OFF;									// condition table off

	/* sort out the condition table */
	uint8_t bll_cnt[2] = { *(uint8_t*)&buf->BLL, buf->COUNTER };							// as REMOTE message has no VALUE and a different byte order
	uint8_t do_or_not = 0;																	// to avoid multiple function calls

	if     (ctTbl == CT::X_GE_COND_VALUE_LO)
		if (buf->VALUE >= l3->COND_VALUE_LO) do_or_not = 1;

	else if (ctTbl == CT::X_GE_COND_VALUE_HI)
		if (buf->VALUE >= l3->COND_VALUE_HI) do_or_not = 1;

	else if (ctTbl == CT::X_LT_COND_VALUE_LO)
		if (buf->VALUE <  l3->COND_VALUE_LO) do_or_not = 1;

	else if (ctTbl == CT::X_LT_COND_VALUE_HI)
		if (buf->VALUE <  l3->COND_VALUE_HI) do_or_not = 1;

	else if (ctTbl == CT::COND_VALUE_LO_LE_X_LT_COND_VALUE_HI)
		if ((l3->COND_VALUE_LO <= buf->VALUE) && (buf->VALUE <  l3->COND_VALUE_HI)) do_or_not = 1;

	else if (ctTbl == CT::X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI)
		if ((buf->VALUE < l3->COND_VALUE_LO) || (buf->VALUE >= l3->COND_VALUE_HI)) do_or_not = 1;

	/* some debug */
	DBG(SW, F("trigger41, value:"), buf->VALUE, F(", cond_table:"), ctTbl, F(", curStat:"), tr40.cur, F(", nxtStat:"), tr40.nxt, '\n');
	DBG(SW, F("CT_ONDELAY:"), _HEXB(l3->CT_ONDELAY), F(", CT_ON:"), _HEXB(l3->CT_ON), F(", CT_OFFDELAY:"), _HEXB(l3->CT_OFFDELAY), F(", CT_OFF:"), _HEXB(l3->CT_OFF), '\n');

	/* forward the request if needed, if not we answer with an ACK */
	if (do_or_not) REMOTE((s_m40xxxx*)(bll_cnt - 10));
	//if (do_or_not) REMOTE((s_m40xxxx*)(((uint8_t*)bll_cnt) - 10));
	else {
		cm_status.message_type = INFO::SND_ACK_STATUS;										// send next time a ack info message
		cm_status.message_delay.set(100);													// wait a short time to set status
	}
}
