/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Dimmer ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


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
	static uint8_t t_lstC[sizeof(cm_dimmer_ChnlReg)];										// allocate the same space in memory as working area
	lstC.val = t_lstC;																		// pointer to ram array

	lstP.lst = 3;																			// setup the peer list (list3) with all dependencies
	lstP.reg = cm_dimmer_PeerReg;															// same as list1
	lstP.def = cm_dimmer_PeerDef;
	lstP.len = sizeof(cm_dimmer_PeerReg);
	static uint8_t t_lstP[sizeof(cm_dimmer_PeerReg)];
	lstP.val = t_lstP;

	/* as the dimmer has virtual channels, we need to take care in the setup */
	vrt_grp = virtual_group;																// remember the virtual group this instance belong too
	vrt_cnl = virtual_channel;																// remember the virtual channel in group this instance belong too
	cms.sum_value = &dm_sum_cnl_value;														// set the sum value to the pointer in the state machine struct
}

/**------------------------------------------------------------------------------------------------------------------------
*- overlay functions -
* ------------------------------------------------------------------------------------------------------------------------- */

void CM_DIMMER::cm_init(void) {
	/* assign list (list1/3) structs to the list arrays */
	l1 = (s_l1*)lstC.val;																	// allign arrays with list structures
	l3 = (s_l3*)lstP.val;																	// reduced l3, description in cm_Dimmer.h at struct declaration
	jt = (s_jt*)((uint8_t*)l3 + 9);															// assign the jump table pointer to list3 array 

	/* clear the variables to avoid unexpected activity */
	jt->ACTION_TYPE = DM_ACTION::INACTIVE;													// and secure that no action will happened in polling function
	tr11.active = 0;																		// set trigger 11 inactive
	cms.sm_active = 0;																		// set state machine inactive
	last_on_value = 200;																	// set last on value default

	/* assign list1 variables and initialize the hardware */
	if (l1->POWERUP_ACTION) {																// check the power up flag
		cms.set_value = 200;
		cms.sm_stat = cms.sm_set = DM_JT::ON;
	} else {
		cms.set_value = 0;
		cms.sm_stat = cms.sm_set = DM_JT::OFF;
	}
	init_dimmer(vrt_grp, vrt_cnl, lstC.cnl);												// call external init function to set the output pins

	/* initiate the status message of the channel */
	cms.msg_delay.set(cms.status_delay);													// wait some time to settle the device
	cms.msg_type = STA_INFO::SND_ACTUATOR_STATUS;											// send the initial status info
	
	DBG(DM, F("DM"), lstC.cnl, F(":CM_INIT-\t\tvrt_grp/cnl: "), vrt_grp, '/', vrt_cnl, F(", send status in: "), cms.msg_delay.remain(), '\n');
	//dbg << F("l1: ") << _HEX((uint8_t*)l1, lstC.len) << '\n';
	//dbg << F("l3: ") << _HEX((uint8_t*)l3, lstP.len/2) << '\n';
	//dbg << F("jt: ") << _HEX((uint8_t*)jt, 4) << '\n';
}

void CM_DIMMER::cm_poll(void) {

	/* some regular poll functions for status send and set external function */
	process_send_status_poll(&cms, lstC.cnl);												// check if there is some status to send, function call in cmMaster.cpp
	adjust_status();																		// check if something is to be set / forwarded to the switchDimmer function in the main sketch

	/* state machine poll, needs a filled l3 and jt */
	if (cms.sm_active) {
		if      (cms.sm_set == DM_JT::ONDELAY)  poll_ondelay();								// on delay state machine
		else if (cms.sm_set == DM_JT::RAMPON)   poll_rampon();								// ramp on state machine
		else if (cms.sm_set == DM_JT::ON)       poll_on();									// on state machine
		else if (cms.sm_set == DM_JT::OFFDELAY) poll_offdelay();							// off delay state machine
		else if (cms.sm_set == DM_JT::RAMPOFF)  poll_rampoff();								// ramp off state machine
		else if (cms.sm_set == DM_JT::OFF)      poll_off();									// off state machine
	}
	/* tr11 poll */
	if (tr11.active) poll_tr11();															// poll if something is to do

}

/*
* @brief setToggle will be addressed by config button in mode 2 by a short key press here we can toggle the status of the actor
*/
void CM_DIMMER::set_toggle(void) {
	DBG(DM, F("DM"), lstC.cnl, F(":set_toggle\n") );

	/* check for inhibit flag */
	if (cms.inhibit) return;																// nothing to do while inhibit is set

	if (cms.value)  cms.set_value = 0;														// if its on, we switch off
	else cms.set_value = 200;

	//tr40.cur = (send_stat.modStat) ? DM_JT::ON : DM_JT::OFF;
	cms.msg_type = STA_INFO::SND_ACTUATOR_STATUS;											// send next time a info status message
	cms.msg_delay.set(5);
}

void CM_DIMMER::info_config_change(uint8_t channel) {
	if (lstC.cnl != channel) return;
	DBG(DM, F("DM"), lstC.cnl, F(":CONFIG_CHANGE\n"));

	/* assign list1 variables and initialize the hardware */
	dm_sum_cnl[vrt_grp].logic[vrt_cnl] = l1->LOGIC_COMBINATION;								// store the logic combination of the virtual channel in the struct
	cms.msg_retr = l1->TRANSMIT_TRY_MAX;													// set the max retrie counter flag

	/* set the status message timer, random can be 0 to 7 seconds */
	get_random((uint8_t*)&cms.status_delay, (uint32_t*)dev_ident.HMID);
	cms.status_delay %= l1->STATUSINFO_RANDOM * 1000;
	cms.status_delay += l1->STATUSINFO_MINDELAY * 500;
	//dbg << F("status_delay: ") << cms.status_delay << F(", l1->STATUSINFO_RANDOM: ") << l1->STATUSINFO_RANDOM << F(", l1->STATUSINFO_MINDELAY: ") << l1->STATUSINFO_MINDELAY << '\n';
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
			uint8_t lst[] = { 0x0A,0x01, 0x0B,0x12, 0x0C,0x22, 0x0D,0x23, 0x8A,0x24, 0x8B,0x12, 0x8C,0x22, 0x8D,0x23, };
			lstP.update_list(lst, 16, idx);
			
		} else {											// even (2,4,6..) means ON
			uint8_t lst[] = { 0x0A,0x01, 0x0B,0x44, 0x0C,0x54, 0x0D,0x64, 0x87,0x0A, 0x8A,0xA5, 0x8B,0x44, 0x8C,0x54, 0x8D,0x64, };
			lstP.update_list(lst, 18, idx);
		}

	} else  {												// toggle peer channel
		//lstP.val[10] = lstP.val[40] = 0x14;
		//lstP.val[11] = lstP.val[41] = 0x52;
		//lstP.val[12] = lstP.val[42] = 0x63;

		//lstP.val[36] = 0xff;
		//lstP.val[39] = 0x26;
	}

	DBG(DM, F("DM"), lstC.cnl, F(":request_peer_defaults CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), F(", idx:"), _HEX(idx), '\n' );
}

/*
* @brief Received message handling forwarded by AS::processMessage
*/
void CM_DIMMER::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	cms.msg_type = STA_INFO::SND_ACTUATOR_STATUS_ANSWER;									// send next time a info status message
	cms.msg_delay.set(5);																	// wait a short time to set status

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
	cms.sm_set = cms.sm_stat;																// stay in the current position
	cms.sm_active = 0;																		// stop all activity
	cms.sm_delay.set(0);																	// and timers

	/* fill the trigger 11 struct depending on the message length and set the active flag accordingly 
	* m> 0E 18 A0 11 45 FB FA 33 11 24 02 01 6A 00 A0 (79357) */
	tr11.value = buf->VALUE;																// value to be set
	tr11.ramp_time = (buf->MSG_LEN >= 14) ? buf->RAMP_TIME : 0;								// get the ramp time if message len indicates that it is included
	tr11.dura_time = (buf->MSG_LEN >= 16) ? buf->DURA_TIME : 0;								// get the dura time if message len indicates that it is included
	tr11.active = 1;																		// something needs to be set

	cms.msg_type = STA_INFO::SND_ACK_STATUS_PAIR;											// ACK should be send
	cms.msg_delay.set(5);																	// give some time

	//dbg << "msg: " << _HEX((uint8_t*)buf, buf->MSG_LEN + 1) << '\n';
	DBG(DM, F("\nDM"), lstC.cnl, F(":INSTRUCTION_SET, setValue:"), tr11.value, F(", rampTime:"), intTimeCvt(tr11.ramp_time), F(", duraTime:"), intTimeCvt(tr11.dura_time), ' ', _TIME, '\n');
}
/*
* @brief INSTRUCTION_INHIBIT_OFF avoids any status change via Sensor, Remote or set_toogle
* Answer to a sensor or remote message is an NACK
*/
void CM_DIMMER::INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf) {
	cms.inhibit = 0;
	hm->send_ACK();
}
/*
* @brief INSTRUCTION_INHIBIT_ON, see INSTRUCTION_INHIBIT_OFF
**/
void CM_DIMMER::INSTRUCTION_INHIBIT_ON(s_m1101xx *buf) {
	cms.inhibit = 1;
	hm->send_ACK();
}
/*
* @brief Function is called on messages comming from master, simulating a remote or push button.
* restructure the message and forward it to the local cmSwitch::REMOTE(s_m40xxxx *buf) function...
* -> 0F 09 B0 3E 63 19 64 33 11 22 23 70 D8 40 01 1D 
*/
void CM_DIMMER::SWITCH(s_m3Exxxx *buf) {

	DBG(DM, F("\nDM"), lstC.cnl, F(":SWITCH"), F(", msg_cnt: "), buf->P_CNT, '\n');
	//REMOTE((s_m40xxxx*)(((uint8_t*)buf) + 4));

	l3 = (s_l3*)lstP.val;																	// set struct portion

	/* forward the request to evaluate the action type; based on the true_or_not flag we use the jump table (lstP + 10) or the else jump table (lstP + 27) */
	jt = (s_jt*)((uint8_t*)l3 + 9);
	do_jump_table(buf->P_CNT, buf->FLAG.BIDI);
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
	//dbg << "msg: " << _HEX((uint8_t*)buf, 0x0b) << '\n';
	DBG(DM, F("\nDM"), lstC.cnl, F(":REMOTE- "));

	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)((uint8_t*)lstP.val + 30) : (s_l3*)lstP.val;				// set short or long struct portion

	// check for multi execute flag
	if ((buf->BLL.LONG) && (cms.msg_cnt == buf->COUNTER) && (!l3->MULTIEXECUTE)) {			// trigger was a repeated long, but we have no multi execute, so return
		DBG(DM, F("no multiexecute\n"));
		return;
	}
	cms.msg_cnt = buf->COUNTER;																// remember message counter

	DBG(DM, F("msg_long: "), buf->BLL.LONG, F(", msg_cnt: "), buf->COUNTER, '\n');
	//dbg << F("lst3 val: ") << _HEX(lstP.val, lstP.len) << '\n';
	//dbg << F("lst3 srt: ") << _HEX((uint8_t*)l3, lstP.len/2) << '\n';

	/* forward the request to evaluate the action type; based on the true_or_not flag we use the jump table (lstP + 10) or the else jump table (lstP + 27) */
	jt = (s_jt*)((uint8_t*)l3 + 9);
	do_jump_table(buf->COUNTER, buf->FLAG.BIDI);
}
/**
* @brief Function is called on messages comming from sensors.
*/
void CM_DIMMER::SENSOR_EVENT(s_m41xxxx *buf) {
	DBG(DM, F("\nDM"), lstC.cnl, F(":SENSOR- "), F("msg_long: "), buf->BLL.LONG, F(", msg_cnt: "), buf->COUNTER, F(", msg_val: "), buf->VALUE, F(", "));

	/* depending on the long flag, we cast the value array into a list3 struct.
	* we do this, because the struct is seperated in two sections, values for a short key press and a section for long key press */
	l3 = (buf->BLL.LONG) ? (s_l3*)((uint8_t*)lstP.val + 30) : (s_l3*)lstP.val;				// set short or long struct portion

	/* set condition table in conjunction of the current jump table status */
	uint8_t cond_tbl;
	if      (cms.sm_stat == DM_JT::ONDELAY)  cond_tbl = l3->CT_ONDELAY;						// delay on
	else if (cms.sm_stat == DM_JT::RAMPON)   cond_tbl = l3->CT_RAMPON;						// ramp on
	else if (cms.sm_stat == DM_JT::ON)       cond_tbl = l3->CT_ON;							// on
	else if (cms.sm_stat == DM_JT::OFFDELAY) cond_tbl = l3->CT_OFFDELAY;					// delay off
	else if (cms.sm_stat == DM_JT::RAMPOFF)  cond_tbl = l3->CT_RAMPOFF;						// ramp off
	else if (cms.sm_stat == DM_JT::OFF)      cond_tbl = l3->CT_OFF;							// currently off
	DBG(DM, F("sm_stat: "), cms.sm_stat, F(", cond_tbl: "), cond_tbl, F(", "));


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
	DBG(DM, F(", true_or_else: "), true_or_else, '\n');
	//dbg << F("lst3 val: ") << _HEX(lstP.val, lstP.len) << '\n';
	//dbg << F("lst3 srt: ") << _HEX((uint8_t*)l3, lstP.len/2) << '\n';

	/* forward the request to evaluate the action type; based on the true_or_not flag we use the jump table (lstP + 10) or the else jump table (lstP + 27) */
	jt = (s_jt*)((uint8_t*)l3 + (true_or_else) ? 9 : 26);
	do_jump_table(buf->COUNTER, buf->FLAG.BIDI);
}


/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */

void CM_DIMMER::adjust_status(void) {

	/* check if something is to do */
	if (cms.value == cms.set_value) return;													// nothing to do, return

	/* calculate the next step, based on the set_value */
	if (cms.set_value > 200) cms.set_value = 200;											// value cannot be higher than 200
	cms.value = cms.set_value;																// no need to enter the function again, value and set_value now similar

	/* summerize up in the mixer value by taking care of LOGIC_COMBINATION and call afterwards the external function */
	//<option id = "LOGIC_INACTIVE", "LOGIC_OR" default = "true", "LOGIC_AND", "LOGIC_XOR", "LOGIC_NOR", "LOGIC_NAND", "LOGIC_ORINVERS"
	//             "LOGIC_ANDINVERS", "LOGIC_PLUS", "LOGIC_MINUS", "LOGIC_MUL", "LOGIC_PLUSINVERS", "LOGIC_MINUSINVERS", "LOGIC_MULINVERS"
	//             "LOGIC_INVERSPLUS", "LOGIC_INVERSMINUS", "LOGIC_INVERSMUL"
	// phyLevel = (((0% o Ch1) o Ch2) o Ch3) 
	int32_t calc_value = 0;																	// define a temp variable to calculate the summary channel
	dm_sum_cnl[vrt_grp].value[vrt_cnl] = cms.value;

	for (uint8_t i = 0; i < 3; i++) {

		if        (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 0) {		/* LOGIC_INACTIVE */
			continue;																		// inaktiv: Der Kanal wird bei der Verkn¸pfung ignoriert

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 1) {		/* LOGIC_OR */
			calc_value |= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// OR: Das Verkn¸pfungsergebnis ist der hˆhere von beiden Pegeln.

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 2) {		/* LOGIC_AND */
			calc_value &= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// AND: Das Verkn¸pfungsergebnis ist der niedrigere von beiden Pegeln.

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 3) {		/* LOGIC_XOR */
			calc_value ^= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// XOR: Ist nur einer der Pegel grˆﬂer als 0 %, ist dieser Pegel auch das Verkn¸pfungsergebnis. In den anderen F‰llen ist das Verkn¸pfungsergebnis 0 %.

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 4) {		/* LOGIC_NOR */
			calc_value = ~(calc_value | dm_sum_cnl[vrt_grp].value[vrt_cnl]);				// NOR: Es wird die Verkn¸pfung OR ausgef¸hrt und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 5) {		/* LOGIC_NAND */
			calc_value = ~(calc_value & dm_sum_cnl[vrt_grp].value[vrt_cnl]);				// NAND: Es wird die Verkn¸pfung AND ausgef¸hrt und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 6) {		/* LOGIC_ORINVERS */
			calc_value |= ~(dm_sum_cnl[vrt_grp].value[vrt_cnl]);							// OR_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung OR ausgef¸hrt.
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 7) {		/* LOGIC_ANDINVERS */
			calc_value &= ~(dm_sum_cnl[vrt_grp].value[vrt_cnl]);							// AND_INVERS: Der zu verkn¸pfende Kanal(rechts vom Ñoì) wird zuerst invertiert(100 % -Pegel) und anschlieﬂend die Verkn¸pfung AND ausgef¸hrt.
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 8) {		/* LOGIC_PLUS */
			calc_value += dm_sum_cnl[vrt_grp].value[vrt_cnl];								// PLUS: Die beiden Pegel werden addiert (max. 100 %).

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 9) {		/* LOGIC_MINUS */
			calc_value -= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// MINUS: Die beiden Pegel werden subtrahiert (min. 0 %).
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 10) {		/* LOGIC_MUL */
			calc_value *= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// MULTI: Die beiden Pegel werden multipliziert.

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 11) {		/* LOGIC_PLUSINVERS */
			calc_value += ~(dm_sum_cnl[vrt_grp].value[vrt_cnl]);							// PLUS_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung PLUS ausgef¸hrt.
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 12) {		/* LOGIC_MINUSINVERS */
			calc_value -= ~(dm_sum_cnl[vrt_grp].value[vrt_cnl]);							// MINUS_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung MINUS ausgef¸hrt.
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 13) {		/* LOGIC_MULINVERS */
			calc_value *= ~(dm_sum_cnl[vrt_grp].value[vrt_cnl]);							// MULTI_INVERS: Der zu verkn¸pfende Kanal (rechts vom Ñoì) wird zuerst invertiert (100 % - Pegel) und anschlieﬂend die Verkn¸pfung MULTI ausgef¸hrt.
		
		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 14) {		/* LOGIC_INVERSPLUS */
			calc_value += dm_sum_cnl[vrt_grp].value[vrt_cnl];								// INVERS_PLUS: Die beiden Pegel werden addiert (max. 100 %) und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).
			if (calc_value > 200) calc_value = 200;
			calc_value = (uint8_t)~calc_value;

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 15) {		/* LOGIC_INVERSMINUS */
			calc_value -= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// INVERS_MINUS: Die beiden Pegel werden subtrahiert (min. 0 %) und das Ergebnis anschlieﬂend invertiert (100 % - Pegel).
			if (calc_value < 0) calc_value = 0;
			calc_value = (uint8_t)~calc_value;

		} else if (dm_sum_cnl[vrt_grp].logic[vrt_cnl] == 16) {		/* LOGIC_INVERSMUL */
			calc_value *= dm_sum_cnl[vrt_grp].value[vrt_cnl];								// INVERS_MULTI: Die beiden Pegel werden multipliziert und das Ergebnis anschlieﬂend invertiert(100 % -Pegel).
			if (calc_value > 200) calc_value = 200;
			calc_value = (uint8_t)~calc_value;

		}

		if (calc_value > 200) calc_value = 200;												// after calculation we need to ensure that max not too big
		if (calc_value < 0) calc_value = 0;													// or if below it should be 0
		dm_sum_cnl_value = calc_value;														// make it globally available
		//dbg << "v: " << dm_sum_cnl[vrt_grp].value[vrt_cnl] << ", l: " << dm_sum_cnl[vrt_grp].logic[vrt_cnl] << ", c: " << calc_value << '\n';
	}

	/* check if we have a quadratic approach to follow and call the main sketch to set the value on the HW */
	//uint16_t calc_value = sum_cnl[vrt_grp].value[vrt_cnl];								// copy value in a bigger variable to calculate the quadratic value
	if (l1->CHARACTERISTIC) {																// check if we should use quadratic approach
		calc_value *= calc_value;															// recalculate the value
		calc_value /= 200;
		if ((cms.value) && (!calc_value)) calc_value = 1;									// till 15 it is below 1, set to 1
	} 
	switch_dimmer(vrt_grp, vrt_cnl, lstC.cnl, (uint8_t)calc_value);						// calling the external function to make it happen
	DBG(DM, F("DM"), lstC.cnl, F(":ADJUST-\t\tvalue: "), cms.value, F(", set: "), calc_value, F(", "), (l1->CHARACTERISTIC)?F("sqare"):F("linear"), '\n';)
}

/* trigger 11 poll function, similar to trigger 40 state machine,
* but all required information stored in tr11 struct 
*/
void CM_DIMMER::poll_tr11(void) {
	uint8_t last_value;
	/* poll tr11 gets polled by the main poll function and takes care of the instruction set message 
	* tr11.active has x stati, 1 for setting up action and start on/off delay, 2 for set value and start dura delay, 3 dura is done-leave */

	if (!cms.sm_delay.done()) return;														// timer not done, wait until then
	DBG(DM, F("DM"), lstC.cnl, F(":TR11-\t\t"), 'A', tr11.active, F(", "));


	if (tr11.active == 1) {																	// set next action based on ramp and dura content
		cms.sm_delay.set(intTimeCvt(tr11.ramp_time));										// set the ramp timer accordingly, could be 0 or a time
		tr11.active = 2;
		cms.sm_stat = cms.sm_set = (cms.value < tr11.value) ? DM_JT::ONDELAY : DM_JT::OFFDELAY;
		DBG(DM, F("value: "), cms.value, F(", set_value: "), tr11.value, F(", ramp_time: "), intTimeCvt(tr11.ramp_time));

	} else if (tr11.active == 2) {
		if (tr11.dura_time) {
			last_value = cms.value;															// remember the current value
			cms.sm_delay.set(intTimeCvt(tr11.dura_time));									// set the duration timer accordingly, could be 0 or a time
		}
		cms.set_value = tr11.value;															// set the tr11 value
		cms.sm_stat = cms.sm_set = (tr11.value > 0) ? DM_JT::ON : DM_JT::OFF;				// set the state machine accordingly
		tr11.active = 3;
		DBG(DM, F("value: "), cms.value, F(", set_value: "), tr11.value, F(", dura_time: "), intTimeCvt(tr11.dura_time));

	} else if (tr11.active == 3) {
		if (tr11.dura_time) cms.set_value = last_value;										// restore the initial value while dura time was set and passed
		cms.sm_stat = cms.sm_set = (tr11.value > 0) ? DM_JT::ON : DM_JT::OFF;				// set the state machine accordingly
		tr11.active = 0;
		DBG(DM,F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}

/* trigger 3E, 40, 41 action type related functions
* the trigger is calling the do_jump_table function, which sets the the necassary parameters or calls up/downdim
*/
void CM_DIMMER::do_jump_table(uint8_t counter, uint8_t bidi) {
	uint8_t toogle_cnt = counter & 1;
	DBG(DM, F("DM"), lstC.cnl, F(":DO_JT-\t\t"));

	/* check for inhibit flag */
	if (cms.inhibit) {
		hm->send_NACK();
		DBG(DM, F("inhibit is set, exit\n"));
		return;
	}

	/* stop any action started by trigger 11 */
	if (tr11.active) {
		tr11.active = 0;
		cms.sm_delay.set(0);
		DBG(DM,F("stop tr11, "));
	}

	/* look after the requested action and progress accordingly */
	DBG(DM, F("action_type: "), jt->ACTION_TYPE, F(", "));
	if (jt->ACTION_TYPE == DM_ACTION::INACTIVE) {
		DBG(DM, '\n');
		// nothing to do

	} else if (jt->ACTION_TYPE == DM_ACTION::JUMP_TO_TARGET) {
		// set next status depending on current status
		if      (cms.sm_stat == DM_JT::ONDELAY)  cms.sm_set = jt->JT_ONDELAY;				// delay on
		else if (cms.sm_stat == DM_JT::RAMPON)   cms.sm_set = jt->JT_RAMPON;				// ramp on
		else if (cms.sm_stat == DM_JT::ON)       cms.sm_set = jt->JT_ON;					// on
		else if (cms.sm_stat == DM_JT::OFFDELAY) cms.sm_set = jt->JT_OFFDELAY;				// delay off
		else if (cms.sm_stat == DM_JT::RAMPOFF)  cms.sm_set = jt->JT_RAMPOFF;				// ramp off
		else if (cms.sm_stat == DM_JT::OFF)      cms.sm_set = jt->JT_OFF;					// currently off

		cms.sm_active = 1;																	// make the respective modul active
		DBG(DM, F(", sm_stat: "), cms.sm_stat, F(", sm_set: "), cms.sm_set);

		/* take care of OFF_TIME_MODE and ON_TIME_MODE, absolut = 0, minimal = 1. 
		* check the status of the state machine and overwrite the delay timer if necassary */
		if ((cms.sm_set == DM_JT::OFF) && (cms.sm_stat == DM_JT::OFF)) {					// it is working through the delay in off state
			uint32_t t_time = (l3->OFF_TIME != 255) ? byteTimeCvt(l3->OFF_TIME) : 0;		// write the time in a variable
			if ((!jt->OFF_TIME_MODE) || (cms.sm_delay.remain() < t_time)) {					// if time is absolut or current delay is smaller than delay to set
				cms.sm_delay.set(t_time);													// set the new delay
				cms.sm_active = 2;															// as we are only correcting the time
				DBG(DM, F("off_time_mode: "), t_time, F(", "));
			}

		} else if ((cms.sm_set == DM_JT::ON) && (cms.sm_stat != DM_JT::ON)) {				// it is working through the delay in on state
			uint32_t t_time = (l3->ON_TIME != 255) ? byteTimeCvt(l3->ON_TIME) : 0;			// write the time in a variable
			if ((!jt->ON_TIME_MODE) || (cms.sm_delay.remain() < t_time)) {					// if time is absolut or current delay is smaller than delay to set
				cms.sm_delay.set(t_time);													// set the new delay
				cms.sm_active = 2;															// as we are only correcting the time
				DBG(DM, F("on_time_mode: "), t_time, F(", "));
			}
		} 

		/* if we are in on mode but the ON_LEVEL_PRIO is set to normal, we need to adjust the brightness */
		if ((cms.sm_stat == DM_JT::ON) && (cms.sm_set == DM_JT::ON) && (!l3->ON_LEVEL_PRIO)) {
			cms.set_value = l3->ON_LEVEL;													// set a new brightness value
			DBG(DM, F("on_level_prio: "), l3->ON_LEVEL, F(", "));
		}

		if (cms.sm_stat == DM_JT::ON) last_on_value = cms.value;							// remember the last on value
		DBG(DM, '\n');

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLE_TO_COUNTER) {
		cms.set_value = (toogle_cnt) ? 200 : 0;												// set the dimmer status depending on message counter
		DBG(DM, F("toogle_to_counter: "), toogle_cnt, '\n');

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLE_INV_TO_COUNTER) {
		cms.set_value = (toogle_cnt) ? 0 : 200;												// set the dimmer status depending on message counter
		DBG(DM, F("toogle_inv_to_counter: "), toogle_cnt, '\n');

	} else if (jt->ACTION_TYPE == DM_ACTION::UPDIM) {	
		/* increase brightness by one step */ 
		DBG(DM, F("updim"), '\n');
		do_updim();																			// call updim 

	} else if (jt->ACTION_TYPE == DM_ACTION::DOWNDIM) {									
		/* decrease brightness by one step */
		DBG(DM, F("downdim"), '\n');
		do_downdim();																		// call down dim

	} else if (jt->ACTION_TYPE == DM_ACTION::TOOGLEDIM) {
		/* increase or decrease, direction change after each use. ideal if a dimmer is driven by only one key */
		static uint8_t r_counter;
		static uint8_t direction;
		DBG(DM, F("toogledim, counter: "), counter, F("r_counter: "), r_counter, '\n');
		//dbg << "tgl, cnt: " << r_counter << ", dir: " << direction << '\n';
		if (r_counter != counter) direction ^= 1;
		r_counter = counter;
		(direction) ? do_updim() : do_downdim();

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLEDIM_TO_COUNTER) {
		/* increase or decrease brightness by one step, direction comes from the last bit of the counter */
		DBG(DM, F("toogledim_to_counter: "), counter, F(", "), (toogle_cnt) ? F("updim") : F("downdim"), '\n');
		(toogle_cnt) ? do_updim() : do_downdim();

	} else if (jt->ACTION_TYPE == DM_ACTION::TOGGLEDIM_INVERS_TO_COUNTER) {
		/* same as TOGGLEDIM_TO_COUNTER but invers */
		DBG(DM, F("toogledim_inv_to_counter: "), counter, F(", "), (toogle_cnt) ? F("downdim") : F("updim"), '\n');
		(toogle_cnt) ? do_downdim() : do_updim();

	}

	/* prepare an answer message to be sent */
	if (bidi) {																				// check if an answer is required
		cms.msg_type = STA_INFO::SND_ACK_STATUS_PEER;										// send next time a ack info message
		cms.msg_delay.set(5);																// wait a short time to set status
	}
}

void CM_DIMMER::do_updim(void) {
	/* increase brightness by DIM_STEP but not over DIM_MAX_LEVEL */
	if (cms.value < l3->DIM_MAX_LEVEL - l3->DIM_STEP) cms.set_value = cms.value + l3->DIM_STEP;
	else cms.set_value = l3->DIM_MAX_LEVEL;
	if (cms.set_value < l3->ON_MIN_LEVEL) cms.set_value = l3->ON_MIN_LEVEL;					// take care of the on_min_level flag

	cms.sm_stat = cms.sm_set = DM_JT::ON;
	DBG(DM, F("DM"), lstC.cnl, F(":UPDIM-\t\t"), F("value: "), cms.value, F(", set_value: "), cms.set_value, F(", dim_step: "), l3->DIM_STEP, '\n');
}
void CM_DIMMER::do_downdim(void) {
	/* decrease brightness by DIM_STEP  but not lower than DIM_MIN_LEVEL */
	if (cms.value > l3->DIM_MIN_LEVEL + l3->DIM_STEP) cms.set_value = cms.value - l3->DIM_STEP;
	else cms.set_value = l3->DIM_MIN_LEVEL;

	/* set the state machine accordingly */
	if (cms.set_value) cms.sm_stat = cms.sm_set = DM_JT::ON;								// set the state machine accordingly
	else cms.sm_stat = cms.sm_set = DM_JT::OFF;

	DBG(DM, F("DM"), lstC.cnl, F(":DOWNDIM-\t\t"), F("value: "), cms.value, F(", set_value: "), cms.set_value, F(", dim_step: "), l3->DIM_STEP, '\n');
}

/* trigger 3E, 40, 41 state machine functions 
* mainly all hm devices are designed as a state machine. state is defined within the jump table and this 
* functions are working through the different states based on the list3 register values
*/
void CM_DIMMER::poll_ondelay(void) {
	/* if poll_ondelay is active it gets polled by the main poll function
	* ondelay has 3 stati, 0 off, 1 set mode and timer, 2 done */

	if (!cms.sm_delay.done()) return;														// check if we need to wait
	DBG(DM, F("DM"), lstC.cnl, F(":SM_ONDELAY-\t\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {																// set up mode, first time call
		// <option id = "SET_TO_OFF" default = "true" / > value 0
		// <option id = "NO_CHANGE" / >                   value 1
		if (l3->ONDELAY_MODE == 0) cms.set_value = l3->OFF_LEVEL;							// check if set_to_off is required 
		cms.sm_delay.set(byteTimeCvt(l3->ONDELAY_TIME));									// set the respective delay
		cms.sm_active = 2;																	// next step is cleaning up the function, while delay is passed
		DBG(DM, F("ondelay_mode: "), (l3->ONDELAY_MODE)? F("no_change") : F("set_to_off") , F(", ondelay_time: "), l3->ONDELAY_TIME);

	} else if (cms.sm_active == 2) {
		cms.process_next(jt->JT_ONDELAY);													// set status based on state machine position and a new target if required
		DBG(DM, F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}
void CM_DIMMER::poll_rampon(void) {
	/* if poll_rampon is active it gets polled by the main poll function, we differentiate between first poll, while set the ramp start step
	* and normal ramp on poll to increase brightness over the given time by l3->RAMPON_TIME 
	* do_rampon has 5 stati, 0 for off, 1 for set the start step, 2 for set the value, 3 for regular and for done */
	//static uint8_t target_value;

	if (!cms.aj_delay.done()) return;														// not ready with setting the value

	/* mode 3 means we decrease brightness over time by checking the remaining time and setting the adj_timer direct */
	if (cms.sm_active == 3) {
		uint8_t delta = (l3->ON_LEVEL - cms.value) / l3->DIM_STEP;							// calculate the neccassary steps
		if (!delta) delta = 1;																// delta needs to be at least one because we use it as divisor
		cms.set_value = (delta == 1) ? l3->ON_LEVEL : cms.value + l3->DIM_STEP;				// set the next value to be set
		cms.aj_delay.set(cms.sm_delay.remain() / delta);									// calculate the slots needed to decrease set_value within the remaining rampon time into 
		DBG(DM, F("\t\t\tA3, value: "), cms.value, F(", set_value: "), cms.set_value, ' ', _TIME, '\n');
		//dbg << F("\t\t\tA3, value: ") << cms.value << F(", set: ") << cms.set_value << F(", sm_delay: ") << cms.sm_delay.remain() << F(", aj_delay: ") << cms.aj_delay.remain() << F(", delta: ") << delta, ' ' << _TIME << '\n';
	}

	if (!cms.sm_delay.done()) return;														// leave while wait timer is active
	DBG(DM, F("DM"), lstC.cnl, F(":SM_RAMPON-\t\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {
		if (cms.value + l3->RAMP_START_STEP < l3->ON_LEVEL) cms.set_value = cms.value + l3->RAMP_START_STEP;
		else cms.set_value = l3->ON_LEVEL;
		cms.sm_active = 2;																	// we will follow up in mode 2
		DBG(DM, F("value: "), cms.value, F(", set: "), cms.set_value, F(", ramp_start_step: "), l3->RAMP_START_STEP);

	} else if (cms.sm_active == 2) {
		if (l3->ON_LEVEL == 201) {															// which value to be set, last on value or register?
			l3->ON_LEVEL = last_on_value;													// write the last on value in the register
			DBG(DM, F(", set last_on_value: "), last_on_value);
		} else 
			DBG(DM, F(", set on_level: "), l3->ON_LEVEL);

		if (!l3->DIM_STEP) l3->DIM_STEP = 1;												// dim step needs to be at least 1 while used as dividor
		cms.sm_delay.set(byteTimeCvt(l3->RAMPON_TIME));										// set the state machine timer
		cms.sm_active = 3;																	// we will follow up in mode 3
		DBG(DM, F(", rampon_time: "), l3->RAMPON_TIME);

	} else if (cms.sm_active == 3) {
		/* mode 3 was processed above, we are here while the rampon time was done */
		cms.process_next(jt->JT_RAMPON);													// set status based on state machine position and a new target if required
		DBG(DM, F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}
void CM_DIMMER::poll_on(void) {
	/* if poll_on is active it gets polled by the main poll function
	* poll_on has 3 stati, 0 off, 1 set timer, 2 done */

	if (!cms.sm_delay.done()) return;
	DBG(DM, F("DM"), lstC.cnl, F(":SM_ON-\t\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {
		if (l3->ON_TIME != NOT_USED) cms.sm_delay.set(byteTimeCvt(l3->ON_TIME));			// set the respective delay, but only if it is not 255
		cms.sm_active = 2;																	// next time will be entered after the delay
		DBG(DM, F("on_time: "), l3->ON_TIME);

	} else if (cms.sm_active == 2) {
		cms.process_next(jt->JT_ON, l3->ON_TIME);											// set status based on state machine position and a new target if required
		DBG(DM, F("done "));

	}
	DBG(DM, ' ', _TIME, '\n');
}
void CM_DIMMER::poll_offdelay(void) {
	/* if poll_offdelay is active it gets polled by the main poll function
	* poll_offdelay has 4 stati, 0 off, 1 set timer, 2 process wait, 3 done */
	static uint8_t old_new_flag;															// flag to remember the initial value while blink on low level

	if (!cms.aj_delay.done()) return;														// blink timer not done yet

	/* make the off delay blinking */
	if (cms.sm_active == 2) {																// 2 is a special mode, let the dimmer signalize with blinking the led that it is in offdelay mode

		/* check if light is on and set the next blink delay accordingly */
		if (old_new_flag) {		// high value
			// <conversion type="float_integer_scale" factor="10" offset="-0.1"/>
			cms.aj_delay.set((l3->OFFDELAY_OLDTIME + 1) * 50);								// set the time based on the conversation type
			cms.set_value = old_new_flag;													// restore the initial brightness
			old_new_flag = 0;																// on next poll we enter the low brightness state

		} else {				// low value
			cms.aj_delay.set((l3->OFFDELAY_NEWTIME + 1) * 50);								// set the time based on the conversation type
			old_new_flag = cms.value;														// remember the initial brightness
			if ((cms.value - l3->OFFDELAY_STEP) > 0) cms.set_value = cms.value - l3->OFFDELAY_STEP;// check if we are above the minimum level
			else cms.set_value = 0;															// and set the brightness level

		}
		DBG(DM, F("\t\t\tA2, value: "), cms.value, F(", set_value: "), cms.set_value, ' ', _TIME, '\n');
		//dbg << F("\t\t\tA2, value: ") << cms.value << F(", set_value: ") << cms.set_value << F(", old_new: ") << old_new_flag << F(", offdelay_step: ") << l3->OFFDELAY_STEP << F(", newtime: ") << l3->OFFDELAY_NEWTIME << F(", oldtime: ") << l3->OFFDELAY_OLDTIME << ' ' << _TIME << '\n';
	}

	if (!cms.sm_delay.done()) return;														// leave while wait timer is active
	DBG(DM, F("DM"), lstC.cnl, F(":SM_OFFDELAY-\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {
		/* there is a special option which let the led blink while in offdelay state, so check the flag and set further procedure accordingly */
		cms.sm_delay.set(byteTimeCvt(l3->OFFDELAY_TIME));									// set the respective delay
		if ((l3->OFFDELAY_BLINK) && (l3->OFFDELAY_TIME)) cms.sm_active = 2;					// 2 is a special mode, let the led blink while in offdelay
		else cms.sm_active = 3;																// to be entered after the delay is done
		old_new_flag = 0;																	// start offdelay blink from start
		DBG(DM, F("offdelay_time: "), l3->OFFDELAY_TIME, F(", off_delay_blink: "), l3->OFFDELAY_BLINK);

	} else if (cms.sm_active == 2) {
		if (old_new_flag) cms.set_value = old_new_flag;										// restore the initial brightness, if blink was ended with a low phase
		cms.sm_active = 3;																	// on next poll goto close the action
		DBG(DM, F("value: "), cms.value, F(", set_value: "), cms.set_value);

	} else if (cms.sm_active == 3) {
		cms.process_next(jt->JT_OFFDELAY);													// set status based on state machine position and a new target if required
		DBG(DM, F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}
void CM_DIMMER::poll_rampoff(void) {
	/* if do_rampoff is active it gets polled by the main poll function
	*  we differentiate between first poll, while set the ramp stop step
	* and normal ramp off poll to decrease brightness over the given time by l3->RAMPOFF_TIME
	* do_rampoff has 4 stati, 0 for off, 1 for initial step down, 2 for set polling, 3 for done */

	if (!cms.aj_delay.done()) return;														// not ready with setting the value

	/* mode 3 means we decrease brightness over time by checking the remaining time and setting the set_delay direct */
	if (cms.sm_active == 3) {
		uint8_t delta = (cms.value - l3->OFF_LEVEL) / l3->DIM_STEP;							// evaluate the delta between current and target value
		if (!delta) delta = 1;																// if delta is empty, we need at least one step
		cms.set_value = (delta == 1)? l3->OFF_LEVEL : cms.value - l3->DIM_STEP;				// set the next value to be set
		cms.aj_delay.set(cms.sm_delay.remain() / delta);									// calculate the slots needed to decrease set_value within the remaining rampon time into 
		DBG(DM, F("\t\t\tA3, value: "), cms.value, F(", set: "), cms.set_value, ' ', _TIME, '\n');
		//dbg << F("\t\t\tA3, value: ") << cms.value << F(", set: ") << cms.set_value << F(", sm_delay: ") << cms.sm_delay.remain() << F(", aj_delay: ") << cms.aj_delay.remain() << F(", delta: ") << delta << ' ' << _TIME << '\n';
	}

	if (!cms.sm_delay.done()) return;														// leave while wait timer is active
	DBG(DM, F("DM"), lstC.cnl, F(":SM_RAMPOFF-\t\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {
		/* first we check what we can set as value for adjust_status */
		if (cms.value > (l3->RAMP_START_STEP + l3->OFF_LEVEL)) cms.set_value = cms.value - l3->RAMP_START_STEP;
		else cms.set_value = l3->OFF_LEVEL;
		cms.sm_active = 2;																	// we will follow up in mode 2
		DBG(DM, F("value: "), cms.value, F(", set_value: "), cms.set_value, F(", ramp_start_step: "), l3->RAMP_START_STEP);

	} else if (cms.sm_active == 2) {
		if (!l3->DIM_STEP) l3->DIM_STEP = 1;												// dim step must be at least one
		cms.sm_delay.set(byteTimeCvt(l3->RAMPOFF_TIME));									// set the state machine timer
		cms.sm_active = 3;																	// we will follow up in mode 3
		DBG(DM, F("ramp_off_time: "), l3->RAMPOFF_TIME);

	} else if (cms.sm_active == 3) {
		/* mode 3 was processed above, we are here while the rampoff time was done */
		cms.process_next(jt->JT_RAMPOFF);													// set status based on state machine position and a new target if required
		DBG(DM, F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}
void CM_DIMMER::poll_off(void) {
	/* if poll_on is active it gets polled by the main poll function
	* poll_on has 3 stati, 0 off, 1 set timer, 2 done */

	if (!cms.sm_delay.done()) return;
	DBG(DM, F("DM"), lstC.cnl, F(":SM_OFF-\t\t"), 'A', cms.sm_active, F(", "));

	if (cms.sm_active == 1) {
		if (l3->OFF_TIME != NOT_USED) cms.sm_delay.set(byteTimeCvt(l3->OFF_TIME));			// set the respective delay
		cms.sm_active = 2;																	// next time will be entered after the delay
		DBG(DM, F("off_time: "), l3->OFF_TIME);

	} else if (cms.sm_active == 2) {
		cms.process_next(jt->JT_OFF, l3->OFF_TIME);											// set status based on state machine position and a new target if required
		DBG(DM, F("done"));

	}
	DBG(DM, ' ', _TIME, '\n');
}

