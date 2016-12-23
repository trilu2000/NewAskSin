/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Dimmer ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"

#include "cmDimmer.h"
#include "as_main.h"

waitTimer adj_timer;																		// timer for dim-up/down
uint32_t adj_delay;																			// calculate and store the adjustment time


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module switch
*        pointer to channel table are forwarded to the master class. 
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/
cmDimmer::cmDimmer(const uint8_t peer_max) : cmMaster(peer_max) {

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cmDimmer_ChnlReg;
	lstC.def = cmDimmer_ChnlDef;
	lstC.len = sizeof(cmDimmer_ChnlReg);

	lstP.lst = 3;																			// setup the peer list with all dependencies
	lstP.reg = cmDimmer_PeerReg;
	lstP.def = cmDimmer_PeerDef;
	lstP.len = sizeof(cmDimmer_PeerReg);

	lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	lstP.val = new uint8_t[lstP.len];

	l1 = (s_l1*)lstC.val;																	// set list structures to something useful
	l3 = (s_l3*)lstP.val;																	// reduced l3, description in cmSwitch.h at struct declaration
	l3F = (s_lstPeer*)lstP.val;

}


/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */


void cmDimmer::adjustStatus(void) {

	if (cm_status.value == cm_status.set_value) return;										// nothing to do, return
	if (!adj_timer.done()) return;															// timer not done, wait until then

	// calculate next step
	if (cm_status.value < cm_status.set_value) cm_status.value++;							// do we have to increase
	else cm_status.value--;																	// or decrease

	if (l1->CHARACTERISTIC) {																// check if we should use quadratic approach
		uint16_t calc_value = cm_status.value * cm_status.value;							// recalculate the value
		calc_value /= 200;
		if ((cm_status.value) && (!calc_value)) calc_value = 1;								// till 15 it is below 1
		switchDimmer(lstC.cnl, calc_value);													// calling the external function to make it happen

	} else switchDimmer(lstC.cnl, cm_status.value);											// calling the external function to make it happen

	adj_timer.set(adj_delay);																// set timer for next action
	DBG(DM, F("DM"), lstC.cnl, F(":adj val: "), cm_status.value, F(", set: "), cm_status.set_value, F(", quad: "), l1->CHARACTERISTIC, '\n';)
}

void cmDimmer::cm_init(void) {

	l3->ACTION_TYPE = ACTION::INACTIVE;														// and secure that no action will happened in polling function
	tr11.active = 0;																		// empty trigger 11 store
	tr40.cur = JT::OFF;																		// default off
	tr40.nxt = JT::OFF;																		// default off

	cm_status.value = 0;																	// output to 0
	cm_status.set_value = 0;

	initDimmer(lstC.cnl);																	// call external init function to set the output pins

	cm_status.message_delay.set((rand() % 2000) + 1000);									// wait some time to settle the device
	cm_status.message_type = INFO::SND_ACTUATOR_STATUS;										// send the initial status info
	
	DBG(DM, F("DM"), lstC.cnl, F(":init\n"));
}
void cmDimmer::cm_poll(void) {

	send_status(&cm_status, lstC.cnl);														// check if there is some status to send, function call in cmMaster.cpp
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
	if (tr40.nxt == JT::ONDELAY) {
		DBG(DM, F("DM"), lstC.cnl, F(":ONDELAY\n"));
		if (l3->ONDELAY_TIME != NOT_USED) {													// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->ONDELAY_TIME));								// activate the timer and set next status
			tr40.nxt = l3->JT_ONDELAY;														// get next status from jump table
		}

	} else if (tr40.nxt == JT::RAMPON) {
		DBG(DM, F("DM"), lstC.cnl, F(":RAMPON\n"));
		cm_status.set_value = l3->ON_LEVEL;
		adj_delay = 5;
		if (l3->RAMPON_TIME  != NOT_USED) {													// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->RAMPON_TIME));								// activate the timer and set next status
			tr40.nxt = l3->JT_RAMPON;														// get next status from jump table
		}

	} else if (tr40.nxt == JT::ON ) {
		DBG(DM, F("DM"), lstC.cnl, F(":ON\n") );
		if (l3->ON_TIME != NOT_USED) {														// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->ON_TIME));									// set the timer while not for ever
			tr40.nxt = l3->JT_ON;															// set next status
		}
		 

	} else if (tr40.nxt == JT::OFFDELAY ) {
		DBG(DM, F("DM"), lstC.cnl, F(":OFFDELAY\n") );
		if (l3->OFFDELAY_TIME != NOT_USED) {												// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->OFFDELAY_TIME));							// activate the timer and set next status
			tr40.nxt = l3->JT_OFFDELAY;														// get jump table for next status
		}

	} else if (tr40.nxt == JT::RAMPOFF) {
		DBG(DM, F("DM"), lstC.cnl, F(":RAMPOFF\n"));
		cm_status.set_value = l3->OFF_LEVEL;
		adj_delay = 5;
		if (l3->RAMPOFF_TIME != NOT_USED) {													// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->RAMPOFF_TIME));								// activate the timer and set next status
			tr40.nxt = l3->JT_RAMPOFF;														// get jump table for next status
		}

	} else if (tr40.nxt == JT::OFF ) {
		DBG(DM, F("DM"), lstC.cnl, F(":OFF\n") );
		cm_status.set_value = 0;															// switch off relay
		if (l3->OFF_TIME != NOT_USED) {														// if time is 255, we stay forever in the current status
			cm_status.delay.set(byteTimeCvt(l3->OFF_TIME));									// activate the timer and set next status
			tr40.nxt = l3->JT_OFF;															// get the next status from jump table
		}
	}
}

/*
* @brief setToggle will be addressed by config button in mode 2 by a short key press here we can toggle the status of the actor
*/
void cmDimmer::set_toggle(void) {
	DBG(DM, F("DM"), lstC.cnl, F(":set_toggle\n") );

	/* check for inhibit flag */
	if (cm_status.inhibit) return;															// nothing to do while inhibit is set

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
* Byte     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59
* ADDRESS  01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 26 27 28 29 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96 97 98 99 9a a6 a7 a8 a9
* DEFAULT  00 00 00 32 64 00 ff 00 ff 00 11 11 11 20 00 14 c8 0a 00 00 00 c8 00 0a 04 04 00 11 11 11 00 00 00 32 64 00 ff 00 ff 00 11 11 11 20 00 14 c8 0a 00 00 00 c8 00 0a 04 04 00 11 11 11

* 2,4,6    00 00 00 32 64 00 FF 00 FF 01 12 22 23 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 00 14 52 63 00 00 00 32 64 00 FF 00 FF 24 12 22 23 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 20 14 52 63
* 1,3,5    00 00 00 32 64 00 FF 00 FF 01 44 54 64 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 00 14 52 63 00 00 00 32 64 00 0A 00 FF A5 44 54 64 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 20 14 52 63
* 0,1,2    00 00 00 32 64 00 ff 00 ff 01 14 52 63 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 00 14 52 63 00 00 00 32 64 00 FF 00 FF 26 14 52 63 20 00 14 C8 0A 05 05 00 C8 0A 0A 04 04 20 14 52 63,   
*                                        xx xx xx                                                                      xx       xx xx xx xx                                 
* on/off        0B 14 0C 52 0D 63               87 FF 8A 26 8B 14 8C 52 8D 63
*  on           0B 12 0C 22 0D 23               87 FF 8A 24 8B 12 8C 22 8D 23
*  off          0B 44 0C 54 0D 64               87 0A 8A A5 8B 44 8C 54 8D 64
*
* As we have to change only 4 bytes, we can map the struct to a byte array point
* and address/change the bytes direct. EEprom gets updated by the eeprom class
* automatically.
*/
void cmDimmer::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {
	// if both peer channels are given, peer channel 01 default is the off dataset, peer channel 02 default is the on dataset
	// if only one peer channel is given, then the default dataset is toogle
	if (( buf->PEER_CNL[0] ) && ( buf->PEER_CNL[1] )) {		// dual peer add

		if (idx % 2) {										// odd (1,3,5..) means OFF
			lstP.val[10] = lstP.val[40] = 0x44;
			lstP.val[11] = lstP.val[41] = 0x54;
			lstP.val[12] = lstP.val[42] = 0x64;

			lstP.val[36] = 0x0a;
			lstP.val[39] = 0xa5;
			
		} else {											// even (2,4,6..) means ON
			lstP.val[10] = lstP.val[40] = 0x12;
			lstP.val[11] = lstP.val[41] = 0x22;
			lstP.val[12] = lstP.val[42] = 0x23;

			lstP.val[36] = 0xff;
			lstP.val[39] = 0x24;
		}

	} else  {												// toggle peer channel
		lstP.val[10] = lstP.val[40] = 0x14;
		lstP.val[11] = lstP.val[41] = 0x52;
		lstP.val[12] = lstP.val[42] = 0x63;

		lstP.val[36] = 0xff;
		lstP.val[39] = 0x26;
	}

	DBG(DM, F("DM"), lstC.cnl, F(":request_peer_defaults CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", idx:"), _HEXB(idx), '\n' );
}

/*
* @brief Received message handling forwarded by AS::processMessage
*/
void cmDimmer::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	cm_status.message_type = INFO::SND_ACTUATOR_STATUS;										// send next time a info status message
	cm_status.message_delay.set(50);														// wait a short time to set status

	DBG(DM, F("DM"), lstC.cnl, F(":CONFIG_STATUS_REQUEST\n"));
}
/*
* @brief INSTRUCTION_SET is called on messages comming from a central device to setup a channel status
* The message contains at least the value which has to be set, but there a two further 2 byte values which 
* represents a ramp timer and a duration timer. We differentiate the messages by the len byte
* 12 byte - value
* 14 byte - value, ramp_time
* 16 byte - value, ramp_time, dura_time
*/ 
void cmDimmer::INSTRUCTION_SET(s_m1102xx *buf) {
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

	DBG(DM, F("DM"), lstC.cnl, F(":INSTRUCTION_SET, setValue:"), tr11.value, F(", rampTime:"), intTimeCvt(tr11.ramp_time), F(", duraTime:"), intTimeCvt(tr11.dura_time), '\n');
}
/*
* @brief INSTRUCTION_INHIBIT_OFF avoids any status change via Sensor, Remote or set_toogle
* Answer to a sensor or remote message is an NACK
*/
void cmDimmer::INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf) {
	cm_status.inhibit = 0;
	send_ACK();
}
/*
* @brief INSTRUCTION_INHIBIT_ON, see INSTRUCTION_INHIBIT_OFF
**/
void cmDimmer::INSTRUCTION_INHIBIT_ON(s_m1101xx *buf) {
	cm_status.inhibit = 1;
	send_ACK();
}
/*
* @brief Function is called on messages comming from master, simulating a remote or push button.
* restructure the message and forward it to the local cmSwitch::REMOTE(s_m40xxxx *buf) function...
* -> 0F 09 B0 3E 63 19 64 33 11 22 23 70 D8 40 01 1D 
*/
void cmDimmer::SWITCH(s_m3Exxxx *buf) {
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
void cmDimmer::REMOTE(s_m40xxxx *buf) {
	/* check for inhibit flag */
	if (cm_status.inhibit) {
		send_NACK();
		return;
	}

	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)lstP.val + 30 : (s_l3*)lstP.val;							// set short or long struct portion

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
		else if (tr40.cur == JT::RAMPON)   tr40.nxt = l3->JT_RAMPON;						// ramp on
		else if (tr40.cur == JT::ON)       tr40.nxt = l3->JT_ON;							// on
		else if (tr40.cur == JT::OFFDELAY) tr40.nxt = l3->JT_OFFDELAY;						// delay off
		else if (tr40.cur == JT::RAMPOFF)  tr40.nxt = l3->JT_RAMPOFF;						// ramp off
		else if (tr40.cur == JT::OFF)      tr40.nxt = l3->JT_OFF;							// currently off

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_TO_COUNTER) {
		cm_status.set_value = (buf->COUNTER % 2) ? 200 : 0;									// set the dimmer status depending on message counter

	} else if (l3->ACTION_TYPE == ACTION::TOGGLE_INV_TO_COUNTER) {
		cm_status.set_value = (buf->COUNTER % 2) ? 0 : 200;									// set the dimmer status depending on message counter

	} else if (l3->ACTION_TYPE == ACTION::UPDIM) {

	} else if (l3->ACTION_TYPE == ACTION::DOWNDIM) {

	} else if (l3->ACTION_TYPE == ACTION::TOOGLEDIM) {

	} else if (l3->ACTION_TYPE == ACTION::TOGGLEDIM_TO_COUNTER) {

	} else if (l3->ACTION_TYPE == ACTION::TOGGLEDIM_INVERS_TO_COUNTER) {

	}

	cm_status.message_type = INFO::SND_ACK_STATUS;											// send next time a ack info message
	cm_status.message_delay.set(100);														// wait a short time to set status

	/* some debug */
	DBG(DM, F("DM"), lstC.cnl, F(":trigger40, msgLng:"), buf->BLL.LONG, F(", msgCnt:"), buf->COUNTER, F(", ACTION_TYPE:"), l3->ACTION_TYPE, F(", curStat:"), tr40.cur, F(", nxtStat:"), tr40.nxt, '\n');
	DBG(DM, F("JT_ONDELAY:"), _HEXB(l3->JT_ONDELAY), F(", ONDELAY_T:"), _HEXB(l3->ONDELAY_TIME), F(", JT_RAMPON:"), _HEXB(l3->JT_RAMPON), F(", RAMPON_T:"), _HEXB(l3->RAMPON_TIME), F(", JT_ON:"), _HEXB(l3->JT_ON), F(", ON_T:"), _HEXB(l3->ON_TIME), F(", JT_OFFDELAY:"), _HEXB(l3->JT_OFFDELAY), F(", OFFDELAY_T:"), _HEXB(l3->OFFDELAY_TIME), F(", JT_RAMPOFF:"), _HEXB(l3->JT_RAMPOFF), F(", RAMPOFF_T:"), _HEXB(l3->RAMPOFF_TIME), F(", JT_OFF:"), _HEXB(l3->JT_OFF), F(", OFF_T:"), _HEXB(l3->OFF_TIME), '\n');
	DBG(DM, F("lst3: "), _HEX(lstP.val, lstP.len), '\n');
}
/**
* @brief Function is called on messages comming from sensors.
*/
void cmDimmer::SENSOR_EVENT(s_m41xxxx *buf) {
	/* check for inhibit flag */
	if (cm_status.inhibit) {
		send_NACK();
		return;
	}

	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)lstP.val + 11 : (s_l3*)lstP.val;							// set short or long struct portion

	/* set condition table in conjunction of the current jump table status */
	uint8_t ctTbl;																			// to select the condition depending on current device status

	if      (tr40.cur == JT::ONDELAY)  ctTbl = l3->CT_ONDELAY;								// condition table delay on
	else if (tr40.cur == JT::RAMPON)   ctTbl = l3->CT_RAMPON;								// condition table ramp on
	else if (tr40.cur == JT::ON)       ctTbl = l3->CT_ON;									// condition table on
	else if (tr40.cur == JT::OFFDELAY) ctTbl = l3->CT_OFFDELAY;								// condition table delay off
	else if (tr40.cur == JT::RAMPOFF)  ctTbl = l3->CT_RAMPOFF;								// condition table ramp off
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
	DBG(DM, F("DM"), lstC.cnl, F(":trigger41, value:"), buf->VALUE, F(", cond_table:"), ctTbl, F(", curStat:"), tr40.cur, F(", nxtStat:"), tr40.nxt, '\n');
	DBG(DM, F("CT_ONDELAY:"), _HEXB(l3->CT_ONDELAY), F(", CT_RAMPON:"), _HEXB(l3->CT_RAMPON), F(", CT_ON:"), _HEXB(l3->CT_ON), F(", CT_OFFDELAY:"), _HEXB(l3->CT_OFFDELAY), F(", CT_RAMPOFF:"), _HEXB(l3->CT_RAMPOFF), F(", CT_OFF:"), _HEXB(l3->CT_OFF), '\n');

	/* forward the request if needed, if not we answer with an ACK */
	if (do_or_not) REMOTE((s_m40xxxx*)(bll_cnt - 10));
	//if (do_or_not) REMOTE((s_m40xxxx*)(((uint8_t*)bll_cnt) - 10));
	else {
		cm_status.message_type = INFO::SND_ACK_STATUS;										// send next time a ack info message
		cm_status.message_delay.set(100);													// wait a short time to set status
	}
}
