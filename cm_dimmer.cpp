/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Dimmer ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"

waitTimer adj_timer;																		// timer for dim-up/down
uint32_t adj_delay;																			// calculate and store the adjustment time

static s_sum_cnl sum_cnl[4];

/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module switch
*        pointer to channel table are forwarded to the master class. 
*        Constructor of master class is processed first.
*        Setup of class specific things is done here - creating the list struct
*/
CM_DIMMER::CM_DIMMER(const uint8_t peer_max, uint8_t virtual_channel, uint8_t virtual_group) : CM_MASTER(peer_max) {

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cm_dimmer_ChnlReg;															// pointer to the list1 register definition
	lstC.def = cm_dimmer_ChnlDef;															// pointer to the list1 defaults definition
	lstC.len = sizeof(cm_dimmer_ChnlReg);													// evaluating and storing the length of list1
	lstC.val = new uint8_t[lstC.len];														// allocate the same space in memory as working area

	lstP.lst = 3;																			// setup the peer list (list3) with all dependencies
	lstP.reg = cm_dimmer_PeerReg;															// same as list1
	lstP.def = cm_dimmer_PeerDef;
	lstP.len = sizeof(cm_dimmer_PeerReg);
	lstP.val = new uint8_t[lstP.len];														// creates an empty array new uint8_t[lstP.len]();

	vrt_grp = virtual_group;																// remember the virtual group this instance belong too
	vrt_cnl = virtual_channel;																// remember the virtual channel in group this instance belong too
}


/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */

void CM_DIMMER::cm_init(void) {
	/* assign list (list1/3) structs to the list arrays */
	l1 = (s_l1*)lstC.val;																	// allign arrays with list structures
	l3 = (s_l3*)lstP.val;																	// reduced l3, description in cm_Dimmer.h at struct declaration
	jt = (s_jt*)((uint8_t*)l3 + 9);															// assign the jump table pointer to list3 array 

	/* clear the variables to avoid unexpected activity */
	jt->ACTION_TYPE = DM_ACTION::INACTIVE;													// and secure that no action will happened in polling function
	tr11.active = 0;																		// empty trigger 11 store
	tr40 = {};																				// empty the trigger 40 struct

	/* assign list1 variables and initialize the hardware */
	// todo - check which registers are set by configuring the internal key
	sum_cnl[vrt_grp].logic[vrt_cnl] = l1->LOGIC_COMBINATION;								// store the logic combination of the virtual channel in the struct

	if (l1->POWERUP_ACTION) {																// check the power up flag
		cm_sta.set_value = 200;
		tr40.cur = tr40.nxt = DM_JT::ON;
	} else {
		cm_sta.set_value = 0;
		tr40.cur = tr40.nxt = DM_JT::OFF;
	}
	init_dimmer(vrt_grp, vrt_cnl, lstC.cnl);												// call external init function to set the output pins

	/* initiate the status message of the channel */
	cm_sta.msg_delay.set((rand() % (l1->STATUSINFO_RANDOM * 1000)) + (l1->STATUSINFO_MINDELAY * 1000));	// wait some time to settle the device
	cm_sta.msg_type = STA_INFO::SND_ACTUATOR_STATUS;										// send the initial status info
	
	DBG(DM, F("DM"), lstC.cnl, F(":init- vrt_grp: "), vrt_grp, F(", vrt_cnl: "), vrt_cnl, F(", min_delay: "), l1->STATUSINFO_MINDELAY, F(", rand_delay: "), l1->STATUSINFO_RANDOM, F(", remain: "), cm_sta.msg_delay.remain(), '\n');
	//DBG(DM, F("l1: "), _HEX((uint8_t*)l1, lstC.len), '\n');
	//DBG(DM, F("l3: "), _HEX((uint8_t*)l3, lstP.len/2), '\n');
	//DBG(DM, F("jt: "), _HEX((uint8_t*)jt, 4), '\n');
}

void CM_DIMMER::cm_poll(void) {

	/* some regular poll functions for status send and set external function */
	process_send_status_poll(&cm_sta, lstC.cnl);											// check if there is some status to send, function call in cmMaster.cpp
	adjust_status();																		// check if something is to be set / forwarded to the switchDimmer function in the main sketch

	/* tr40 state machine poll, needs a filled l3 and jt */
	poll_jumptable();																		// jump table, driven by jt and tr40.cur/tr40.nxt
	poll_ondelay();																			// on delay state machine
	poll_rampon();																			// ramp on state machine
	poll_on();																				// on state machine
	poll_offdelay();																		// off delay state machine
	poll_rampoff();																			// ramp off state machine
	poll_off();																				// off state machine

	/* tr11 poll */
	poll_tr11();

}

/*
* @brief setToggle will be addressed by config button in mode 2 by a short key press here we can toggle the status of the actor
*/
void CM_DIMMER::set_toggle(void) {
	DBG(DM, F("DM"), lstC.cnl, F(":set_toggle\n") );

	/* check for inhibit flag */
	if (cm_sta.inhibit) return;																// nothing to do while inhibit is set

	if (cm_sta.value)  cm_sta.set_value = 0;												// if its on, we switch off
	else cm_sta.set_value = 200;

	//tr40.cur = (send_stat.modStat) ? DM_JT::ON : DM_JT::OFF;
	cm_sta.msg_type = STA_INFO::SND_ACTUATOR_STATUS;										// send next time a info status message
	cm_sta.msg_delay.set(50);
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
void CM_DIMMER::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {
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

	DBG(DM, F("DM"), lstC.cnl, F(":request_peer_defaults CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), F(", idx:"), _HEX(idx), '\n' );
}

/*
* @brief Received message handling forwarded by AS::processMessage
*/
void CM_DIMMER::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	cm_sta.msg_type = STA_INFO::SND_ACTUATOR_STATUS;										// send next time a info status message
	cm_sta.msg_delay.set(50);																// wait a short time to set status

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
void CM_DIMMER::INSTRUCTION_SET(s_m1102xx *buf) {
	/* stop all ongoing actions */
	jt->ACTION_TYPE = DM_ACTION::INACTIVE;													// action type to off otherwise the polling function will overwrite
	tr40.nxt = tr40.cur;																	// stay in the current position

	/* fill the trigger 11 struct depending on the message length and set the active flag accordingly */
	tr11.value = buf->VALUE;
	tr11.ramp_time = (buf->MSG_LEN >= 14) ? buf->RAMP_TIME : 0;								// get the ramp time if message len indicates that it is included
	tr11.dura_time = (buf->MSG_LEN >= 16) ? buf->DURA_TIME : 0;								// get the dura time if message len indicates that it is included

	if (tr11.ramp_time) tr11.active = 1;													// indicate we are coming from trigger11
	else cm_sta.set_value = tr11.value;														// otherwise set the value directly
	cm_sta.fsm_delay.set(intTimeCvt(tr11.ramp_time));										// set the timer accordingly, could be 0 or a time

	if (tr11.dura_time) tr11.active = 1;													// set tr11 flag active to be processed in the poll function

	cm_sta.msg_type = STA_INFO::SND_ACK_STATUS;												// ACK should be send
	cm_sta.msg_delay.set(5);																// give some time

	DBG(DM, F("DM"), lstC.cnl, F(":INSTRUCTION_SET, setValue:"), tr11.value, F(", rampTime:"), intTimeCvt(tr11.ramp_time), F(", duraTime:"), intTimeCvt(tr11.dura_time), '\n');
}
/*
* @brief INSTRUCTION_INHIBIT_OFF avoids any status change via Sensor, Remote or set_toogle
* Answer to a sensor or remote message is an NACK
*/
void CM_DIMMER::INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf) {
	cm_sta.inhibit = 0;
	hm->send_ACK();
}
/*
* @brief INSTRUCTION_INHIBIT_ON, see INSTRUCTION_INHIBIT_OFF
**/
void CM_DIMMER::INSTRUCTION_INHIBIT_ON(s_m1101xx *buf) {
	cm_sta.inhibit = 1;
	hm->send_ACK();
}
/*
* @brief Function is called on messages comming from master, simulating a remote or push button.
* restructure the message and forward it to the local cmSwitch::REMOTE(s_m40xxxx *buf) function...
* -> 0F 09 B0 3E 63 19 64 33 11 22 23 70 D8 40 01 1D 
*/
void CM_DIMMER::SWITCH(s_m3Exxxx *buf) {
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
void CM_DIMMER::REMOTE(s_m40xxxx *buf) {
	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)((uint8_t*)lstP.val + 30) : (s_l3*)lstP.val;				// set short or long struct portion
	tr11.active = 0;																		// stop any tr11 processing

	// check for multi execute flag
	if ((buf->BLL.LONG) && (tr40.cnt == buf->COUNTER) && (!l3->MULTIEXECUTE)) return;		// trigger was a repeated long, but we have no multi execute, so return
	tr40.cnt = buf->COUNTER;																// remember message counter

	DBG(DM, F("DM"), lstC.cnl, F(":trigger40, msg_long: "), buf->BLL.LONG, F(", msg_cnt: "), buf->COUNTER, F(", action_type: "), l3->ACTION_TYPE, F(", cur: "), tr40.cur, F(", nxt:"), tr40.nxt, '\n');
	//DBG(DM, F("JT_ONDELAY:"), _HEX(l3->JT_ONDELAY), F(", ONDELAY_T:"), _HEX(l3->ONDELAY_TIME), F(", DM_JT_RAMPON:"), _HEX(l3->JT_RAMPON), F(", RAMPON_T:"), _HEX(l3->RAMPON_TIME), F(", DM_JT_ON:"), _HEX(l3->JT_ON), F(", ON_T:"), _HEX(l3->ON_TIME), F(", DM_JT_OFFDELAY:"), _HEX(l3->JT_OFFDELAY), F(", OFFDELAY_T:"), _HEX(l3->OFFDELAY_TIME), F(", DM_JT_RAMPOFF:"), _HEX(l3->JT_RAMPOFF), F(", RAMPOFF_T:"), _HEX(l3->RAMPOFF_TIME), F(", DM_JT_OFF:"), _HEX(l3->JT_OFF), F(", OFF_T:"), _HEX(l3->OFF_TIME), '\n');
	//DBG(DM, F("lst3: "), _HEX(lstP.val, lstP.len), '\n');
	//DBG(DM, F("lst3: "), _HEX((uint8_t*)l3, lstP.len/2), '\n');

	/* forward the request to evaluate the action type; based on the true_or_not flag we use the jump table (lstP + 10) or the else jump table (lstP + 27) */
	jt = (s_jt*)((uint8_t*)l3 + 9);
	do_jump_table(&buf->COUNTER);
}
/**
* @brief Function is called on messages comming from sensors.
*/
void CM_DIMMER::SENSOR_EVENT(s_m41xxxx *buf) {
	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)((uint8_t*)lstP.val + 30) : (s_l3*)lstP.val;				// set short or long struct portion
	tr11.active = 0;																		// stop any tr11 processing

	/* set condition table in conjunction of the current jump table status */
	uint8_t cond_tbl;
	if      (tr40.cur == DM_JT::ONDELAY)  cond_tbl = l3->CT_ONDELAY;						// delay on
	else if (tr40.cur == DM_JT::RAMPON)   cond_tbl = l3->CT_RAMPON;							// ramp on
	else if (tr40.cur == DM_JT::ON)       cond_tbl = l3->CT_ON;								// on
	else if (tr40.cur == DM_JT::OFFDELAY) cond_tbl = l3->CT_OFFDELAY;						// delay off
	else if (tr40.cur == DM_JT::RAMPOFF)  cond_tbl = l3->CT_RAMPOFF;						// ramp off
	else if (tr40.cur == DM_JT::OFF)      cond_tbl = l3->CT_OFF;							// currently off


	/* sort out the condition table */
	uint8_t true_or_else = 0;																// to avoid multiple function calls

	if     (cond_tbl == DM_CT::X_GE_COND_VALUE_LO)											// based on the current state machine we check the condition
		if (buf->VALUE >= l3->COND_VALUE_LO) true_or_else = 1;

	else if (cond_tbl == DM_CT::X_GE_COND_VALUE_HI)
		if (buf->VALUE >= l3->COND_VALUE_HI) true_or_else = 1;

	else if (cond_tbl == DM_CT::X_LT_COND_VALUE_LO)
		if (buf->VALUE <  l3->COND_VALUE_LO) true_or_else = 1;

	else if (cond_tbl == DM_CT::X_LT_COND_VALUE_HI)
		if (buf->VALUE <  l3->COND_VALUE_HI) true_or_else = 1;

	else if (cond_tbl == DM_CT::COND_VALUE_LO_LE_X_LT_COND_VALUE_HI)
		if ((l3->COND_VALUE_LO <= buf->VALUE) && (buf->VALUE <  l3->COND_VALUE_HI)) true_or_else = 1;

	else if (cond_tbl == DM_CT::X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI)
		if ((buf->VALUE < l3->COND_VALUE_LO) || (buf->VALUE >= l3->COND_VALUE_HI)) true_or_else = 1;

	/* some debug */
	DBG(DM, F("DM"), lstC.cnl, F(":trigger41, value: "), buf->VALUE, F(", cond_table: "), cond_tbl, F(",msg_long: "), buf->BLL.LONG, F(", msg_cnt: "), buf->COUNTER, F(", action_type: "), l3->ACTION_TYPE, F(", cur: "), tr40.cur, F(", nxt:"), tr40.nxt, '\n');
	//DBG(DM, F("CT_ONDELAY:"), _HEX(l3->CT_ONDELAY), F(", DM_CT_RAMPON:"), _HEX(l3->CT_RAMPON), F(", DM_CT_ON:"), _HEX(l3->CT_ON), F(", DM_CT_OFFDELAY:"), _HEX(l3->CT_OFFDELAY), F(", DM_CT_RAMPOFF:"), _HEX(l3->CT_RAMPOFF), F(", DM_CT_OFF:"), _HEX(l3->CT_OFF), '\n');

	/* forward the request to evaluate the action type; based on the true_or_not flag we use the jump table (lstP + 10) or the else jump table (lstP + 27) */
	jt = (s_jt*)((uint8_t*)l3 + (true_or_else) ? 9 : 26);
	do_jump_table(&buf->COUNTER);
}


void CM_DIMMER::adjust_status(void) {

	/* check if something is to do */
	if (cm_sta.value == cm_sta.set_value) return;											// nothing to do, return

	/* calculate the next step, based on the set_value */
	if (cm_sta.set_value > 200) cm_sta.set_value = 200;										// value cannot be higher than 200
	cm_sta.value = cm_sta.set_value;														// no need to enter the function again, value and set_value now similar

	/* summerize up in the mixer value by taking care of LOGIC_COMBINATION and call afterwards the external function */
	//<option id = "LOGIC_INACTIVE", "LOGIC_OR" default = "true", "LOGIC_AND", "LOGIC_XOR", "LOGIC_NOR", "LOGIC_NAND", "LOGIC_ORINVERS"
	//             "LOGIC_ANDINVERS", "LOGIC_PLUS", "LOGIC_MINUS", "LOGIC_MUL", "LOGIC_PLUSINVERS", "LOGIC_MINUSINVERS", "LOGIC_MULINVERS"
	//             "LOGIC_INVERSPLUS", "LOGIC_INVERSMINUS", "LOGIC_INVERSMUL"
	// phyLevel = (((0% o Ch1) o Ch2) o Ch3) 
	int32_t calc_value = 0;																	// define a temp variable to calculate the summary channel
	sum_cnl[vrt_grp].value[vrt_cnl] = cm_sta.value;											// store the status value of the channel in the virtual channel struct
	for (uint8_t i = 0; i < 3; i++) {
		if (sum_cnl[vrt_grp].logic[vrt_cnl] == 0) {				/* LOGIC_INACTIVE */
			continue;																		// inaktiv: Der Kanal wird bei der Verkn¸pfung ignoriert

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 1) {		/* LOGIC_OR */
			calc_value |= sum_cnl[vrt_grp].value[vrt_cnl];									// OR: Das Verkn¸pfungsergebnis ist der hˆhere von beiden Pegeln.

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 2) {		/* LOGIC_AND */
			calc_value &= sum_cnl[vrt_grp].value[vrt_cnl];									// AND: Das Verkn¸pfungsergebnis ist der niedrigere von beiden Pegeln.

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 3) {		/* LOGIC_XOR */
			calc_value ^= sum_cnl[vrt_grp].value[vrt_cnl];									// XOR: Ist nur einer der Pegel grˆﬂer als 0 %, ist dieser Pegel auch das Verkn¸pfungsergebnis. In den anderen F‰llen ist das Verkn¸pfungsergebnis 0 %.

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 4) {		/* LOGIC_NOR */
			calc_value = ~(calc_value | sum_cnl[vrt_grp].value[vrt_cnl]);					// NOR: Es wird die Verkn¸pfung OR ausgef¸hrt und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 5) {		/* LOGIC_NAND */
			calc_value = ~(calc_value & sum_cnl[vrt_grp].value[vrt_cnl]);					// NAND: Es wird die Verkn¸pfung AND ausgef¸hrt und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 6) {		/* LOGIC_ORINVERS */
			calc_value |= ~(sum_cnl[vrt_grp].value[vrt_cnl]);								// OR_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung OR ausgef¸hrt.
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 7) {		/* LOGIC_ANDINVERS */
			calc_value &= ~(sum_cnl[vrt_grp].value[vrt_cnl]);								// AND_INVERS: Der zu verkn¸pfende Kanal(rechts vom Ñoì) wird zuerst invertiert(100 % -Pegel) und anschlieﬂend die Verkn¸pfung AND ausgef¸hrt.
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 8) {		/* LOGIC_PLUS */
			calc_value += sum_cnl[vrt_grp].value[vrt_cnl];									// PLUS: Die beiden Pegel werden addiert (max. 100 %).

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 9) {		/* LOGIC_MINUS */
			calc_value -= sum_cnl[vrt_grp].value[vrt_cnl];									// MINUS: Die beiden Pegel werden subtrahiert (min. 0 %).
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 10) {		/* LOGIC_MUL */
			calc_value *= sum_cnl[vrt_grp].value[vrt_cnl];									// MULTI: Die beiden Pegel werden multipliziert.

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 11) {		/* LOGIC_PLUSINVERS */
			calc_value += ~(sum_cnl[vrt_grp].value[vrt_cnl]);								// PLUS_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung PLUS ausgef¸hrt.
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 12) {		/* LOGIC_MINUSINVERS */
			calc_value -= ~(sum_cnl[vrt_grp].value[vrt_cnl]);								// MINUS_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung MINUS ausgef¸hrt.
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 13) {		/* LOGIC_MULINVERS */
			calc_value *= ~(sum_cnl[vrt_grp].value[vrt_cnl]);								// MULTI_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung MULTI ausgef¸hrt.
		
		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 14) {		/* LOGIC_INVERSPLUS */
			calc_value += sum_cnl[vrt_grp].value[vrt_cnl];									// INVERS_PLUS: Die beiden Pegel werden addiert (max. 100 %) und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).
			if (calc_value > 200) calc_value = 200;
			calc_value = (uint8_t)~calc_value;

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 15) {		/* LOGIC_INVERSMINUS */
			calc_value -= sum_cnl[vrt_grp].value[vrt_cnl];									// INVERS_MINUS: Die beiden Pegel werden subtrahiert (min. 0 %) und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).
			if (calc_value < 0) calc_value = 0;
			calc_value = (uint8_t)~calc_value;

		} else if (sum_cnl[vrt_grp].logic[vrt_cnl] == 16) {		/* LOGIC_INVERSMUL */
			calc_value *= sum_cnl[vrt_grp].value[vrt_cnl];									// INVERS_MULTI: Die beiden Pegel werden multipliziert und das Ergebnis anschlieﬂend invertiert(100 % -Pegel).
			if (calc_value > 200) calc_value = 200;
			calc_value = (uint8_t)~calc_value;

		}

		if (calc_value > 200) calc_value = 200;												// after calculation we need to ensure that max not too big
		if (calc_value < 0) calc_value = 0;													// or if below it should be 0

		dbg << "v: " << sum_cnl[vrt_grp].value[vrt_cnl] << ", l: " << sum_cnl[vrt_grp].logic[vrt_cnl] << ", c: " << calc_value << '\n';
	}

	/* check if we have a quadratic approach to follow and call the main sketch to set the value on the HW */
	//uint16_t calc_value = sum_cnl[vrt_grp].value[vrt_cnl];								// copy value in a bigger variable to calculate the quadratic value
	if (l1->CHARACTERISTIC) {																// check if we should use quadratic approach
		calc_value *= calc_value;															// recalculate the value
		calc_value /= 200;
		if ((cm_sta.value) && (!calc_value)) calc_value = 1;								// till 15 it is below 1, set to 1
	} 
	switch_dimmer(vrt_grp, vrt_cnl, lstC.cnl, calc_value);									// calling the external function to make it happen
	//DBG(DM, F("DM"), lstC.cnl, F(":adj val: "), cm_status.value, F(", set: "), cm_status.set_value, F(", quad: "), l1->CHARACTERISTIC, '\n';)
}

/* trigger 11 poll function, similar to trigger 40 state machine,
* but all required information stored in tr11 struct 
*/
void CM_DIMMER::poll_tr11(void) {
	if (!tr11.active) return;																// nothing to do, leave

	if (!cm_sta.fsm_delay.done()) return;													// timer not done, wait until then

	// - trigger11, check if rampTime or onTimer is set
	if (tr11.active) {
		if (tr11.ramp_time) {																// ramp timer was set, now we have to set the value
			cm_sta.set_value = tr11.value;													// set the value we had stored
			tr11.active = 0;																// reset tr11, if dura time is set, we activate again
			tr11.ramp_time = 0;																// not necessary to do it again
		}

		if (tr11.dura_time) {																// coming from trigger 11, we should set the duration period
			cm_sta.fsm_delay.set(intTimeCvt(tr11.dura_time));								// set the duration time
			tr11.active = 1;																// we have set the timer so remember that it was from tr11
			tr11.dura_time = 0;																// but indicate, it was done

		} else {																			// check if something is to do from trigger11
			cm_sta.set_value = tr11.value ^ 200;											// invert the status
			tr11.active = 0;																// trigger11 ready
		}
		tr40.cur = (cm_sta.set_value) ? DM_JT::ON : DM_JT::OFF;								// set tr40 status, otherwise a remote will not work
	}
}

/* trigger 3E, 40, 41 action type related functions
* the trigger is calling the do_jump_table function, which sets the the necassary parameters or calls up/downdim
*/
void CM_DIMMER::do_jump_table(uint8_t *counter) {
	uint8_t toogle_cnt = *counter & 1;

	/* check for inhibit flag */
	if (cm_sta.inhibit) {
		hm->send_NACK();
		DBG(DM, F("DM"), lstC.cnl, F(":inhibit is set\n"));
		return;
	}
	DBG(DM, F("DM"), lstC.cnl, F(":action_type: "), jt->ACTION_TYPE, '\n');

	/* stop any action started by trigger 11 */
	tr11.active = 0;

	/* look after the requested action and progress accordingly */
	if (jt->ACTION_TYPE == DM_ACTION::INACTIVE) {
		// nothing to do

	} else if (jt->ACTION_TYPE == DM_ACTION::JUMP_TO_TARGET) {
		// set next status depending on current status
		if      (tr40.cur == DM_JT::ONDELAY)  tr40.nxt = jt->JT_ONDELAY;					// delay on
		else if (tr40.cur == DM_JT::RAMPON)   tr40.nxt = jt->JT_RAMPON;						// ramp on
		else if (tr40.cur == DM_JT::ON)       tr40.nxt = jt->JT_ON;							// on
		else if (tr40.cur == DM_JT::OFFDELAY) tr40.nxt = jt->JT_OFFDELAY;					// delay off
		else if (tr40.cur == DM_JT::RAMPOFF)  tr40.nxt = jt->JT_RAMPOFF;					// ramp off
		else if (tr40.cur == DM_JT::OFF)      tr40.nxt = jt->JT_OFF;						// currently off

		DBG(DM, F("DM"), lstC.cnl, F(":action_type: "), jt->ACTION_TYPE, F(", cur: "), tr40.cur, F(", nxt: "), tr40.nxt, '\n');

		/* if flag is set, means prio of the action is low - so do not overrule current status if state machine is in on-mode */
		if (l3->ON_LEVEL_PRIO) {
			if ((tr40.cur == DM_JT::ONDELAY) || (tr40.cur == DM_JT::RAMPON) || (tr40.cur == DM_JT::ON)) tr40.nxt = tr40.cur;
		}

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLE_TO_COUNTER) {
		cm_sta.set_value = (toogle_cnt) ? 200 : 0;											// set the dimmer status depending on message counter

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLE_INV_TO_COUNTER) {
		cm_sta.set_value = (toogle_cnt) ? 0 : 200;											// set the dimmer status depending on message counter

	} else if (jt->ACTION_TYPE == DM_ACTION::UPDIM) {	
		/* increase brightness by one step */ 
		do_updim();																			// call updim 

	} else if (jt->ACTION_TYPE == DM_ACTION::DOWNDIM) {									
		/* decrease brightness by one step */
		do_downdim();																		// call down dim

	} else if (jt->ACTION_TYPE == DM_ACTION::TOOGLEDIM) {
		/* increase or decrease, direction change after each use. ideal if a dimmer is driven by only one key */
		static uint8_t r_counter;
		static uint8_t direction;
		//dbg << "tgl, cnt: " << r_counter << ", dir: " << direction << '\n';
		if (r_counter != *counter) direction ^= 1;
		r_counter = *counter;
		(direction) ? do_updim() : do_downdim();

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLEDIM_TO_COUNTER) {
		/* increase or decrease brightness by one step, direction comes from the last bit of the counter */
		(toogle_cnt) ? do_updim() : do_downdim();

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLEDIM_INVERS_TO_COUNTER) {
		/* same as TOGGLEDIM_TO_COUNTER but invers */
		(toogle_cnt) ? do_downdim() : do_updim();

	}

	/* prepare an answer message to be sent */
	cm_sta.msg_type = STA_INFO::SND_ACK_STATUS;												// send next time a ack info message
	cm_sta.msg_delay.set(5);																// wait a short time to set status
}
void CM_DIMMER::do_updim(void) {
	/* increase brightness by DIM_STEP but not over DIM_MAX_LEVEL */
	if (cm_sta.value < l3->DIM_MAX_LEVEL - l3->DIM_STEP) cm_sta.set_value = cm_sta.value + l3->DIM_STEP;
	else cm_sta.set_value = l3->DIM_MAX_LEVEL;
	if (cm_sta.set_value < l3->ON_MIN_LEVEL) cm_sta.set_value = l3->ON_MIN_LEVEL;			// take care of the on_min_level flag

	tr40.cur = tr40.nxt = DM_JT::ON;
	DBG(DM, F("DM"), lstC.cnl, F(":updim val: "), cm_sta.value, F(", set: "), cm_sta.set_value, F(", step: "), l3->DIM_STEP, '\n');
}
void CM_DIMMER::do_downdim(void) {
	/* decrease brightness by DIM_STEP  but not lower than DIM_MIN_LEVEL */
	if (cm_sta.value > l3->DIM_MIN_LEVEL + l3->DIM_STEP) cm_sta.set_value = cm_sta.value - l3->DIM_STEP;
	else cm_sta.set_value = l3->DIM_MIN_LEVEL;

	/* set the state machine accordingly */
	if (cm_sta.value) tr40.cur = tr40.nxt = DM_JT::ON;										// set the state machine accordingly
	else tr40.cur = tr40.nxt = DM_JT::OFF;

	DBG(DM, F("DM"), lstC.cnl, F(":downdim val: "), cm_sta.value, F(", set: "), cm_sta.set_value, cm_sta.value, F(", step: "), l3->DIM_STEP, '\n');
}

/* trigger 3E, 40, 41 state machine functions 
* mainly all hm devices are designed as a state machine. state is defined within the jump table and this 
* functions are working through the different states based on the list3 register values
*/
void CM_DIMMER::poll_jumptable(void) {

	// - jump table section for trigger3E/40/41
	if (jt->ACTION_TYPE != DM_ACTION::JUMP_TO_TARGET) return;								// only valid for jump table
	if (tr40.cur == tr40.nxt) return;														// no status change, leave
	tr40.cur = tr40.nxt;																	// seems next status is different to current, remember for next poll
	DBG(DM, F("DM"), lstC.cnl, F(":jumptable- cur: "), tr40.cur, F(", nxt: "), tr40.nxt, '\n');

	if (tr40.nxt == DM_JT::ONDELAY) tr40.ondelay = 1;										// set flag accordingly
	else if (tr40.nxt == DM_JT::RAMPON) tr40.rampon = 1;
	else if (tr40.nxt == DM_JT::ON) tr40.on = 1;
	else if (tr40.nxt == DM_JT::OFFDELAY) tr40.offdelay = 1;
	else if (tr40.nxt == DM_JT::RAMPOFF) tr40.rampoff = 1;
	else if (tr40.nxt == DM_JT::OFF) tr40.off = 1;
}
void CM_DIMMER::poll_ondelay(void) {
	/* if poll_ondelay is active it gets polled by the main poll function
	* ondelay has 3 stati, 0 off, 1 set mode and timer, 2 done */

	if (!tr40.ondelay) return;																// check if ondelay is active
	if (!cm_sta.fsm_delay.done()) return;													// check if we need to wait

	if (tr40.ondelay == 1) {																// set up mode, first time call
		// <option id = "SET_TO_OFF" default = "true" / > value 0
		// <option id = "NO_CHANGE" / >                   value 1
		if (l3->ONDELAY_MODE == 0) {														// check if set_to_off is required 
			cm_sta.set_value = l3->OFF_LEVEL;												// set off level
		}
		cm_sta.fsm_delay.set(byteTimeCvt(l3->ONDELAY_TIME));								// set the respective delay
		tr40.ondelay = 2;																	// next step is cleaning up the function, while delay is passed
		DBG(DM, F("DM"), lstC.cnl, F(":ondelay- mode: "), (l3->ONDELAY_MODE)? F("no_change") : F("set_to_off") , F(":ondelay- time: "), byteTimeCvt(l3->ONDELAY_TIME), ' ', _TIME, '\n');

	} else if (tr40.ondelay == 2) {
		tr40.ondelay = 0;																	// ondelay is processed
		tr40.nxt = jt->JT_ONDELAY;															// set next state, otherwise we stay here forever 
		DBG(DM, F("DM"), lstC.cnl, F(":ondelay- done "), _TIME, '\n');

	}
}
void CM_DIMMER::poll_rampon(void) {
	/* if poll_rampon is active it gets polled by the main poll function, we differentiate between first poll, while set the ramp start step
	* and normal ramp on poll to increase brightness over the given time by l3->RAMPON_TIME 
	* do_rampon has 5 stati, 0 for off, 1 for set the start step, 2 for set the value, 3 for regular and for done */
	static uint8_t target_value;

	if (!tr40.rampon) return;																// check if active
	if (!cm_sta.set_delay.done()) return;													// not ready with setting the value

	/* mode 3 means we decrease brightness over time by checking the remaining time and setting the adj_timer direct */
	if (tr40.rampon == 3) {
		uint8_t delta = (target_value - cm_sta.value) / l3->DIM_STEP;
		cm_sta.set_delay.set(cm_sta.fsm_delay.remain() / delta);							// calculate the slots needed to increase set_value within the remaining rampon time into 
		if (delta) cm_sta.set_value += l3->DIM_STEP;										// increase till delta is gone
		else cm_sta.set_value = target_value;
		DBG(DM, F("DM"), lstC.cnl, F(":rampon- cur: "), cm_sta.value, F(", set: "), cm_sta.set_value, F(", remain: "), cm_sta.fsm_delay.remain(), F(", delta: "), delta, ' ', _TIME, '\n');
	}

	if (!cm_sta.fsm_delay.done()) return;													// leave while wait timer is active

	if (tr40.rampon == 1) {
		if (cm_sta.value + l3->RAMP_START_STEP < l3->ON_LEVEL) cm_sta.set_value = cm_sta.value + l3->RAMP_START_STEP;
		else cm_sta.set_value = l3->ON_LEVEL;
		tr40.rampon = 2;																	// we will follow up in mode 2
		DBG(DM, F("DM"), lstC.cnl, F(":rampon- cur: "), cm_sta.value, F(", set: "), cm_sta.set_value, F(", ramp_start_step: "), l3->RAMP_START_STEP, ' ', _TIME, '\n');

	} else if (tr40.rampon == 2) {
		target_value = (l3->ON_LEVEL == 201)? last_on_value : l3->ON_LEVEL;					// which value to be set, last or register?
		cm_sta.fsm_delay.set(byteTimeCvt(l3->RAMPON_TIME));									// set the state machine timer
		tr40.rampon = 3;																	// we will follow up in mode 3
		DBG(DM, F("DM"), lstC.cnl, F(":rampon- cur: "), cm_sta.value, F(", set: "), target_value, F(", on_level: "), l3->ON_LEVEL, F(", timer: "), byteTimeCvt(l3->RAMPON_TIME), ' ', _TIME, '\n');

	} else if (tr40.rampon == 3) {
		/* mode 3 was processed above, we are here while the rampon time was done */
		cm_sta.set_delay.set(0);															// in case of unwanted value in the state machine timer
		tr40.rampon = 0;																	// rampon is done
		tr40.nxt = jt->JT_RAMPON;															// set next state while we should not stay forever here
		DBG(DM, F("DM"), lstC.cnl, F(":rampon- done "), _TIME, '\n');

	}
}
void CM_DIMMER::poll_on(void) {
	/* if poll_on is active it gets polled by the main poll function
	* poll_on has 3 stati, 0 off, 1 set timer, 2 done */
	if (!tr40.on) return;
	if (!cm_sta.fsm_delay.done()) return;

	if (tr40.on == 1) {
		if (l3->ON_TIME != NOT_USED) cm_sta.fsm_delay.set(byteTimeCvt(l3->ON_TIME));		// set the respective delay, but only if it is not 255
		tr40.on = 2;																		// next time will be entered after the delay
		last_on_value = tr40.cur;															// remember the last on value
		DBG(DM, F("DM"), lstC.cnl, F(":on- time: "), byteTimeCvt(l3->ON_TIME), ' ', _TIME, '\n');

	} else if (tr40.on == 2) {
		tr40.on = 0;																		// done here
		if (l3->ON_TIME < NOT_USED) tr40.nxt = jt->JT_ON;									// stay forever is the timer indicates it with 255, otherwise set the next state
		DBG(DM, F("DM"), lstC.cnl, F(":on- done "), _TIME, '\n');

	}
}
void CM_DIMMER::poll_offdelay(void) {
	/* if poll_offdelay is active it gets polled by the main poll function
	* poll_offdelay has 4 stati, 0 off, 1 set timer, 2 process wait, 3 done */
	static uint8_t old_new_flag;															// flag to remember the initial value while blink on low level

	if (!tr40.offdelay) return;																// offdelay is not active 
	if (!cm_sta.set_delay.done()) return;													// blink timer not done yet

	if (tr40.offdelay == 2) {																// 2 is a special mode, let the dimmer signalize with blinking the led that it is in offdelay mode
		/* check if light is on and set the next blink delay accordingly */

		if (old_new_flag) {		// high value
			// <conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			cm_sta.set_delay.set((l3->OFFDELAY_OLDTIME + 1) * 50);							// set the time based on the conversation type
			cm_sta.set_value = old_new_flag;												// restore the initial brightness
			old_new_flag = 0;																// on next poll we enter the low brightness state

		} else {				// low value
			cm_sta.set_delay.set((l3->OFFDELAY_NEWTIME + 1) * 50);							// set the time based on the conversation type
			old_new_flag = cm_sta.value;													// remember the initial brightness
			//if (cm_sta.value - l3->OFFDELAY_STEP > l3->ON_MIN_LEVEL) cm_sta.set_value = cm_sta.value - l3->OFFDELAY_STEP;// check if we are above the minimum level
			//else cm_sta.set_value = l3->ON_MIN_LEVEL;										// set the brightness level
			if (cm_sta.value - l3->OFFDELAY_STEP > 0) cm_sta.set_value = cm_sta.value - l3->OFFDELAY_STEP;// check if we are above the minimum level
			else cm_sta.set_value = 0;														// set the brightness level

		}
		DBG(DM, F("DM"), lstC.cnl, F(":offdelay- cur: "), cm_sta.value, F(", set: "), cm_sta.set_value, F(", new_old: "), old_new_flag, F(", off_step: "), l3->OFFDELAY_STEP, F(", new_time: "), l3->OFFDELAY_NEWTIME, F(", old_time: "), l3->OFFDELAY_OLDTIME, ' ', _TIME, '\n');
	}

	if (!cm_sta.fsm_delay.done()) return;													// leave while wait timer is active
	if (old_new_flag) cm_sta.set_value = old_new_flag;										// restore the initial brightness, if blink was ended with a low phase

	if (tr40.offdelay == 1) {
		/* there is a special option which let the led blink while in offdelay state, so check the flag and set further procedure accordingly */
		cm_sta.fsm_delay.set(byteTimeCvt(l3->OFFDELAY_TIME));								// set the respective delay
		if (l3->OFFDELAY_BLINK) tr40.offdelay = 2;											// 2 is a special mode, let the led blink while in offdelay
		else tr40.offdelay = 3;																// to be entered after the delay is done
		old_new_flag = 0;																	// start offdelay blink from start
		adj_delay = 0;																		// set the adjustment timer to zero
		DBG(DM, F("DM"), lstC.cnl, F(":offdelay- time: "), byteTimeCvt(l3->OFFDELAY_TIME), F(", blink: "), l3->OFFDELAY_BLINK, ' ', _TIME, '\n');

	} else if (tr40.offdelay == 2) {
		tr40.offdelay = 3;																	// on next poll goto close the action
		DBG(DM, F("DM"), lstC.cnl, F(":offdelay- blink end "), _TIME, '\n');

	} else if (tr40.offdelay == 3) {
		tr40.offdelay = 0;																	// set the flag inactive
		tr40.cur = DM_JT::OFFDELAY;															// remember where we are in the state machine
		tr40.nxt = jt->JT_OFFDELAY;															// set the next state
		DBG(DM, F("DM"), lstC.cnl, F(":offdelay- done "), _TIME, '\n');

	}
}
void CM_DIMMER::poll_rampoff(void) {
	/* if do_rampoff is active it gets polled by the main poll function
	*  we differentiate between first poll, while set the ramp stop step
	* and normal ramp off poll to decrease brightness over the given time by l3->RAMPOFF_TIME
	* do_rampoff has 4 stati, 0 for off, 1 for initial step down, 2 for set polling, 3 for done */
	static uint8_t target_value;

	if (!tr40.rampoff) return;																// check if active
	if (!cm_sta.set_delay.done()) return;													// not ready with setting the value

	/* mode 3 means we decrease brightness over time by checking the remaining time and setting the set_delay direct */
	if (tr40.rampoff == 3) {
		uint8_t delta = (cm_sta.value - target_value) / l3->DIM_STEP;						// evaluate the delta between current and target value
		cm_sta.set_delay.set(cm_sta.fsm_delay.remain() / delta);							// calculate the slots needed to decrease set_value within the remaining rampon time into 
		if (delta) cm_sta.set_value -= l3->DIM_STEP;
		else cm_sta.set_value = target_value;
		DBG(DM, F("DM"), lstC.cnl, F(":rampoff- cur: "), cm_sta.value, F(", set: "), cm_sta.set_value, F(", remain: "), cm_sta.fsm_delay.remain(), ' ', _TIME, '\n');
	}

	if (!cm_sta.fsm_delay.done()) return;													// leave while wait timer is active


	if (tr40.rampoff == 1) {
		/* first we check what we can set as value for adjust_status */
		if (cm_sta.value > l3->RAMP_START_STEP + l3->OFF_LEVEL) cm_sta.set_value = cm_sta.value - l3->RAMP_START_STEP;
		else cm_sta.set_value = l3->OFF_LEVEL;
		if (l3->RAMPOFF_TIME != NOT_USED) cm_sta.fsm_delay.set(byteTimeCvt(l3->RAMPOFF_TIME));// set the state machine timer
		tr40.rampoff = 2;																	// we will follow up in mode 2
		DBG(DM, F("DM"), lstC.cnl, F(":rampoff- cur: "), cm_sta.value, F(", ramp_start_step: "), l3->RAMP_START_STEP, ' ', _TIME, '\n');

	} else if (tr40.rampoff == 2) {
		target_value = l3->OFF_LEVEL;														// set the target value which will get processed above
		cm_sta.fsm_delay.set(byteTimeCvt(l3->RAMPOFF_TIME));								// set the state machine timer
		tr40.rampoff = 3;																	// we will follow up in mode 3
		DBG(DM, F("DM"), lstC.cnl, F(":rampoff- cur: "), cm_sta.value, F(", set: "), target_value, F(", on_level: "), l3->ON_LEVEL, F(", timer: "), byteTimeCvt(l3->RAMPON_TIME), ' ', _TIME, '\n');

	} else if (tr40.rampoff == 3) {
		/* mode 3 was processed above, we are here while the rampoff time was done */
		cm_sta.set_delay.set(0);															// in case of unwanted value in the state machine timer
		tr40.rampoff = 0;																	// rampoff is done now
		tr40.nxt = jt->JT_RAMPOFF;															// set next state while otherwise we stay forever here
		DBG(DM, F("DM"), lstC.cnl, F(":rampoff- done "), _TIME, '\n');

	}
}
void CM_DIMMER::poll_off(void) {
	/* if poll_on is active it gets polled by the main poll function
	* poll_on has 3 stati, 0 off, 1 set timer, 2 done */

	if (!tr40.off) return;
	if (!cm_sta.fsm_delay.done()) return;

	if (tr40.off == 1) {
		if (l3->OFF_TIME != NOT_USED) cm_sta.fsm_delay.set(byteTimeCvt(l3->OFF_TIME));		// set the respective delay
		tr40.off = 2;																		// next time will be entered after the delay
		DBG(DM, F("DM"), lstC.cnl, F(":off- time: "), byteTimeCvt(l3->OFF_TIME), ' ', _TIME, '\n');

	} else if (tr40.off == 2) {
		tr40.off = 0;
		if (l3->OFF_TIME < NOT_USED) tr40.nxt = jt->JT_RAMPOFF;								// set next state while otherwise we stay forever here
		DBG(DM, F("DM"), lstC.cnl, F(":off- done "), _TIME, '\n');

	}
}
