/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"





//public://------------------------------------------------------------------------------------------------------------------
CM_MASTER::CM_MASTER(const uint8_t peer_max) {
	peerDB.max = peer_max;

	lstC.cnl = cnl_max;																		// set the channel to the lists
	lstP.cnl = cnl_max++;
}



/**
* @brief Virtual function is called while we received a new list0 or list1
* Herewith we can adapt changes given by the config change. Needs to be overwritten
* by the respective channel module
*/
void CM_MASTER::info_config_change(uint8_t channel) {
	DBG(CM, F("CM:config_change "), channel, '\n' );
}
/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st 3 bytes shows the peer address, 4th and 5th byte gives the peer channel
* no need for sending an answer here, for information only
*/
void CM_MASTER::info_peer_add(s_m01xx01 *buf) {
	DBG(CM, F("CM:info_peer_add, peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), '\n');
}

void CM_MASTER::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {
	lstP.load_default();																	// copy the defaults from progmem into the peer list, index doesn't matter
	lstP.save_list(idx);																	// and save the list, index is important while more choices in the peer table
	DBG(CM, F("CM:request_peer_defaults, idx:"), _HEX(idx), F(", CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), '\n' );
}


void CM_MASTER::init(void) {
	cm_init();																				// init the virtual poll function
}

void CM_MASTER::cm_init(void) {
}

void CM_MASTER::poll(void) {
	cm_poll();																				// poll the virtual poll function 
}
void CM_MASTER::cm_poll(void) {
}

/*
* @brief This function is called at the moment by the config button class, it is to toogle the output
* of an alligned channel. Needs to be overwritten by any actor class 
*/
void CM_MASTER::set_toggle(void) {
	DBG(CM, F("CM:toggle\n") );
}


/*
* @brief Received message handling forwarded by AS::processMessage
*/
/*
* @brief Adds one or two peers to a channel
* CONFIG_PEER_ADD message is send by the HM master to combine two client devices
* request is forwarded by the AS:processMessage function
*/
void CM_MASTER::CONFIG_PEER_ADD(s_m01xx01 *buf) {
	uint8_t temp_peer[4];																	// temp byte array to load peer addresses
	uint8_t ret_byte = 0;																	// prepare a placeholder for success reporting

	for (uint8_t i = 0; i < 2; i++) {														// standard gives 2 peer channels
		if (!buf->PEER_CNL[i]) continue;													// if the current peer channel is empty, go to the next entry

		memcpy(temp_peer, buf->PEER_ID, 3);													// copy the peer address into the temp array
		temp_peer[3] = buf->PEER_CNL[i];													// write the peer channel byte into the array

		uint8_t idx = peerDB.get_idx(temp_peer);											// search if we have already the peer in the database
		if (idx == 0xff) idx = peerDB.get_free_slot();										// not in the in the database, search a free slot

		if (idx != 0xff) {																	// free slot available
			peerDB.set_peer(idx, temp_peer);												// write the peer into the database
			request_peer_defaults(idx, buf);												// ask the channel module to load the defaults
			ret_byte++;																		// increase success
		}
	}
	info_peer_add(buf);																		// inform the user module of the added peer

	DBG(CM, F("CM:CONFIG_PEER_ADD, cnl:"), buf->MSG_CNL, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), F(", RET:"), ret_byte, '\n');
	hm->check_send_ACK_NACK(ret_byte);
}
/*
* @brief Removes one or two peers from a channel
* CONFIG_PEER_REMOVE message is send by the HM master to remove the binding of two client devices
* request is forwarded by the AS:processMessage function
*/
void CM_MASTER::CONFIG_PEER_REMOVE(s_m01xx02 *buf) {
	uint8_t temp_peer[4];																	// temp byte array to load peer addresses
	uint8_t ret_byte = 0;																	// prepare a placeholder for success reporting

	for (uint8_t i = 0; i < 2; i++) {														// standard gives 2 peer channels
		if (!buf->PEER_CNL[i]) continue;													// if the current peer channel is empty, go to the next entry

		memcpy(temp_peer, buf->PEER_ID, 3);													// copy the peer address into the temp array
		temp_peer[3] = buf->PEER_CNL[i];													// write the peer channel byte into the array
		uint8_t idx = peerDB.get_idx(temp_peer);											// find the peer in the database

		if (idx != 0xff) {																	// found it
			peerDB.clear_peer(idx);															// delete the peer in the database
			ret_byte++;																		// increase success
		}
	}
	DBG(CM, F("CM:CONFIG_PEER_REMOVE, cnl:"), buf->MSG_CNL, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEX(buf->PEER_CNL[0]), F(", CNL_B:"), _HEX(buf->PEER_CNL[1]), '\n');
	hm->check_send_ACK_NACK(ret_byte);
}
/*
* @brief Requests a listing of all registered peers from a specific channel
* CONFIG_PEER_LIST_REQ message is send by the HM master to get information which peers are known at the client device.
* This type of message can't be answered within one string, therefor we send an ACK on the initial list request
* and prepare a struct with the required information to process it further in a seperate function call
* request is forwarded by the AS:processMessage function
*/
void CM_MASTER::CONFIG_PEER_LIST_REQ(s_m01xx03 *buf) {
	DBG(CM, F("CM:CONFIG_PEER_LIST_REQ\n"));
	s_list_msg   *lm = &list_msg;															// short hand to the struct with all information for slice wise send

	lm->active = LIST_ANSWER::PEER_LIST;													// we want to get the peer list
	lm->peer = &peerDB;																		// pointer to the respective peerDB struct
	lm->max_slc = peerDB.get_nr_slices();													// get an idea of the total needed slices
	lm->timer.set(15);																		// some time between last message
	DBG(CM, F("DM"), lstC.cnl, F(":CONFIG_PEER_LIST_REQ-\t\tnr slices: "), lm->max_slc, '\n');
}
/*
* @brief Requests a listing of all values in a list from a specific channel
* CONFIG_PARAM_REQ message is send by the HM master to get information which list values are stored at the client device.
* This type of message can't be answered within one string, therefor we send an ACK on the initial list request
* and prepare a struct with the required information to process it further in a seperate function call
* request is forwarded by the AS:processMessage function
*/
void CM_MASTER::CONFIG_PARAM_REQ(s_m01xx04 *buf) {
	//m> 10 1E A0 01 45 FB FA 33 11 24 01 04 33 11 24 01 03 (103556)
	DBG(CM, F("DM"), lstC.cnl, F(":CONFIG_PARAM_REQ-\t\taddr: "), _HEX(buf->PEER_ID,4), F(", list: "), buf->PARAM_LIST);
	s_list_msg   *lm = &list_msg;															// short hand to the struct with all information for slice wise send
	s_list_table *ls = list[buf->PARAM_LIST];												// short hand to the respective list table

	if (!ls) {																				// specific list is not available, return without anything
		DBG(CM, F(", list unavailable\n"));
		return;	
	}

	uint8_t idx = peerDB.get_idx(buf->PEER_ID);												// get the requested peer index, PEER_ID is only 3 byte, but the following byte gives the channel
	if (idx == 0xff) {																		// nothing to do while index not found
		DBG(CM, F(", address unavailable\n"));
		hm->send_NACK_TARGET_INVALID();
		return;
	}

	lm->active = LIST_ANSWER::PARAM_RESPONSE_PAIRS;											// we want to get the param list as pairs
	lm->peer_idx = idx;																		// remember on the peer index
	lm->list = ls;																			// pointer to the respective list struct
	lm->max_slc = ls->get_nr_slices_pairs();												// get an idea of the total needed slices, plus one for closing 00 00 message
	lm->timer.set(15);																		// some time between last message
	DBG(CM, F(", slice send started\n"));
}
/*
* @brief HM protocol indicates changes to any list by a config start request
* Within this message we find the channel, the respective list and the peer address to evaluate the index
* if we have to write a list3 or 4. Within the write index we will find only the channel where to write,
* all other information we need to save somewhere to remember on.
* Message description:
*             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
* 10 04 A0 01 63 19 63 01 02 04 01 05      00 00 00 00          00
*/
void CM_MASTER::CONFIG_START(s_m01xx05 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct
	cm->list = list[buf->PARAM_LIST];														// short hand to the list table
	cm->idx_peer = peerDB.get_idx(buf->PEER_ID);											// try to get the peer index

	if ((cm->list) && (cm->idx_peer != 0xff)) {												// list and peer index found
		cm->timer.set(5000);																// set timeout time, otherwise the channel will be open for write forever
		cm->active = 1;																		// set active
		hm->send_ACK();																		// send back that everything is ok
		// TODO: set message id flag to config in send module
	} else {
		hm->send_NACK();																	// something wrong
	}
	DBG(CM, F("CM:CONFIG_START, cnl:"), buf->MSG_CNL, '/', cm->list->cnl, F(", lst:"), buf->PARAM_LIST, '/', cm->list->lst, F(", peer:"), _HEX(buf->PEER_ID, 4), F(", idx:"), cm->idx_peer, '\n');
}
/*
* @brief Config end indicates that changes are written to the respective list and no access is needed any more
* Within this function we call the respective channel module and inform of a change in any list
* Message description:
*             Sender__ Receiver    Channel
* 10 04 A0 01 63 19 63 01 02 04 01 06
*/
void CM_MASTER::CONFIG_END(s_m01xx06 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct
	cm->timer.set(0);																		// clear the timer
	cm->active = 0;																			// clear the flag

	hm->send_ACK();																			// send back that everything is ok

	if (cm->list->lst < 2) {
		lstC.load_list(cm->idx_peer);														// reload list0 or 1
		inform_config_change(lstC.cnl);														// inform the channel module on a change of list0 or 1
	}
	// TODO: remove message id flag to config in send module
	DBG(CM, F("CM:CONFIG_END, cnl:"), buf->MSG_CNL, '\n');
}
/* 
* todo: implement
*/
void CM_MASTER::CONFIG_WRITE_INDEX1(s_m01xx07 *buf) {
	DBG(CM, F("CM:CONFIG_WRITE_INDEX1\n"));
}
/*
* @brief config write index writes new content to specific list values
* Format is 02 00 03 ff where 02 and 03 is the register address and 00 and ff is the content
* list has to be enabled by a config start message and closed with a config end message
* Message description:
*             Sender__ Receiver        Channel ConfigData: Register:BytePairs
* 13 02 A0 01 63 19 63 01 02 04 00  08 02      01 0A 63 0B 19 0C 63
*/
void CM_MASTER::CONFIG_WRITE_INDEX2(s_m01xx08 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct

	if (cm->active)  {																		// check if config is active, channel fit is checked in AS
		cm->list->write_array(buf->DATA, buf->MSG_LEN - 11, cm->idx_peer);					// write the array into the list
		hm->send_ACK();																		// we are fine
		DBG(CM, F("CM:CONFIG_WRITE_INDEX2, cnl:"), buf->MSG_CNL, F(", lst:"), cm->list->lst, F(", idx:"), cm->idx_peer, '\n');
	} else hm->send_NACK();
}
/*
* @brief Process message CONFIG_SERIAL_REQ.
*
* Message description:
*             Sender__ Receiver
* 0B 77 A0 01 63 19 63 01 02 04 00 09
*/
void CM_MASTER::CONFIG_SERIAL_REQ(s_m01xx09 *buf) {
	hm->send_INFO_SERIAL();
	DBG(CM, F("CM:CONFIG_SERIAL_REQ\n"));
}
/*
* @brief Process message CONFIG_PAIR_SERIAL
*
* Message description:
*             Sender__ Receiver       SerialNumber
* 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
*/
void CM_MASTER::CONFIG_PAIR_SERIAL(s_m01xx0a *buf) {
	DBG(CM, F("CM:CONFIG_PAIR_SERIAL, cnl:"), _HEX(buf->SERIALNO,10), '\n');

	if (isEqual(buf->SERIALNO, dev_ident.SERIAL_NR, 10)) 									// compare serial and send device info
		hm->send_DEVICE_INFO(MSG_REASON::ANSWER);
}
/*
* @brief Process message CONFIG_STATUS_REQUEST
* Virtual function to be overwritten by the respective channel module
*
* Message description:
*                Sender__  Receiver  CNL   BY11
* l> 0B 40 A0 01 63 19 64  23 70 D8  01    0E     
*/
void CM_MASTER::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	DBG(CM, F("CM:CONFIG_STATUS_REQUEST\n"));
}


void CM_MASTER::ACK(s_m0200xx *buf) {
	DBG(CM, F("CM:ACK\n"));
}
void CM_MASTER::ACK_STATUS(s_m0201xx *buf) {
	DBG(CM, F("CM:ACK_STATUS\n"));
}
void CM_MASTER::ACK2(s_m0202xx *buf) {
	DBG(CM, F("CM:ACK2\n"));
}
void CM_MASTER::NACK(s_m0280xx *buf) {
	DBG(CM, F("CM:NACK\n"));
}
void CM_MASTER::NACK_TARGET_INVALID(s_m0284xx *buf) {
	DBG(CM, F("CM:NACK_TARGET_INVALID\n"));
}
void CM_MASTER::ACK_NACK_UNKNOWN(s_m02xxxx *buf) {
	DBG(CM, F("CM:ACK_NACK_UNKNOWN\n"));
}


void CM_MASTER::INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_INHIBIT_OFF\n"));
}
void CM_MASTER::INSTRUCTION_INHIBIT_ON(s_m1101xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_INHIBIT_ON\n"));
}
void CM_MASTER::INSTRUCTION_SET(s_m1102xx *buf) {
	dbg << "CM set\n";
	DBG(CM, F("CM:INSTRUCTION_SET\n"));
}
void CM_MASTER::INSTRUCTION_STOP_CHANGE(s_m1103xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_STOP_CHANGE\n"));
}

//void CM_MASTER::INSTRUCTION_RESET(s_m1104xx *buf) {
//}
void CM_MASTER::INSTRUCTION_LED(s_m1180xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LED\n"));
}
void CM_MASTER::INSTRUCTION_LED_ALL(s_m1181xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LED_ALL\n"));
}
void CM_MASTER::INSTRUCTION_LEVEL(s_m1181xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LEVEL\n"));
}
void CM_MASTER::INSTRUCTION_SLEEPMODE(s_m1182xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_SLEEPMODE\n"));
}
//void CM_MASTER::INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf) {
//}
void CM_MASTER::INSTRUCTION_SET_TEMP(s_m1186xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_SET_TEMP\n"));
}
//void CM_MASTER::INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf) {
//}
//void CM_MASTER::INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf) {
//}


void CM_MASTER::HAVE_DATA(s_m12xxxx *buf) {
	DBG(CM, F("CM:HAVE_DATA\n"));
}


void CM_MASTER::SWITCH(s_m3Exxxx *buf) {
	DBG(CM, F("CM:SWITCH\n"));
}
void CM_MASTER::TIMESTAMP(s_m3fxxxx *buf) {
	DBG(CM, F("CM:TIMESTAMP\n"));
}
void CM_MASTER::REMOTE(s_m40xxxx *buf) {
	DBG(CM, F("CM:REMOTE\n"));
}
void CM_MASTER::SENSOR_EVENT(s_m41xxxx *buf) {
	DBG(CM, F("CM:SENSOR_EVENT\n"));
}
void CM_MASTER::SWITCH_LEVEL(s_m42xxxx *buf) {
	DBG(CM, F("CM:SWITCH_LEVEL\n"));
}
void CM_MASTER::SENSOR_DATA(s_m53xxxx *buf) {
	DBG(CM, F("CM:SENSOR_DATA\n"));
}
void CM_MASTER::GAS_EVENT(s_m54xxxx *buf) {
	DBG(CM, F("CM:GAS_EVENT\n"));
}
void CM_MASTER::CLIMATE_EVENT(s_m58xxxx *buf) {
	DBG(CM, F("CM:CLIMATE_EVENT\n"));
}
void CM_MASTER::SET_TEAM_TEMP(s_m59xxxx *buf) {
	DBG(CM, F("CM:SET_TEAM_TEMP\n"));
}
void CM_MASTER::THERMAL_CONTROL(s_m5axxxx *buf) {
	DBG(CM, F("CM:THERMAL_CONTROL\n"));
}
void CM_MASTER::POWER_EVENT_CYCLE(s_m5exxxx *buf) {
	DBG(CM, F("CM:POWER_EVENT_CYCLE\n"));
}
void CM_MASTER::POWER_EVENT(s_m5fxxxx *buf) {
	DBG(CM, F("CM:POWER_EVENT\n"));
}
void CM_MASTER::WEATHER_EVENT(s_m70xxxx *buf) {
	DBG(CM, F("CM:WEATHER_EVENT\n"));
}


/*
* @brief Sends the ACK_STATUS and INFO_ACTUATOR_STATUS based on content of send_stat struct
* polled by cmMaster poll function.
*/
void process_send_status_poll(s_cm_status *cm, uint8_t cnl) {

	if (snd_msg.active) return;																// send has already something to do
	if (!cm->msg_type) return;																// nothing to do
	if (!cm->msg_delay.done()) return;														// not the right time

	/* prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80 
	* enum E : uint8_t { NO_JUMP_IGNORE_COMMAND = 0x00, ONDELAY = 0x01, RAMPON = 0x02, ON = 0x03, OFFDELAY = 0x04, RAMPOFF = 0x05, OFF = 0x06 };*/
	cm->sf.UP = (cm->sm_set == 0x02) ? 1 : 0;												// RAMPON
	cm->sf.DOWN = (cm->sm_set == 0x05) ? 1 : 0;												// RAMPOFF
	cm->sf.DELAY = ((cm->sm_set == 0x01) || (cm->sm_set == 0x04)) ? 1 : 0;					// ON or OFFDELAY
	//dbg << "up/down/delay: " << cm->sf.UP << '/' << cm->sf.DOWN << '/' << cm->sf.DELAY  << '\n';

	/* compose the message, beside flags and type the ack_status and aktuator_status are identical */
	snd_msg.buf[11] = cnl;																	// add the channel
	snd_msg.buf[12] = cm->value;															// and the status/value
	snd_msg.buf[13] = cm->flag;																// flags are prepared in the status poll function
	if (bat->get_status()) snd_msg.buf[13] |= 0x80;											// highest bit is battery flag
	else snd_msg.buf[13] &= 0x7F;
	snd_msg.buf[14] = com->rssi;															// add rssi information
	snd_msg.buf[15] = *cm->sum_value;														// we can add it to the buffer in any case, while length byte is set below
	snd_msg.temp_max_retr = cm->msg_retr;													// how often to resend a message

	/* check which type has to be send, set message type in a first step and then more in detail the receiver */
	if ((cm->msg_type == STA_INFO::SND_ACK_STATUS_PAIR) || (cm->msg_type == STA_INFO::SND_ACK_STATUS_PEER)) {
		if (cm->sum_value) snd_msg.type = MSG_TYPE::ACK_STATUS_SUM;							// length and flags are set within the snd_msg struct
		else snd_msg.type = MSG_TYPE::ACK_STATUS;
		if (!rcv_msg.mBody.FLAG.BIDI) snd_msg.active = MSG_ACTIVE::NONE;

	} else if ((cm->msg_type == STA_INFO::SND_ACTUATOR_STATUS_ANSWER) || (cm->msg_type == STA_INFO::SND_ACTUATOR_STATUS)) {
		if (cm->sum_value) snd_msg.type = MSG_TYPE::INFO_ACTUATOR_STATUS_SUM;				// length and flags are set within the snd_msg struct
		else snd_msg.type = MSG_TYPE::INFO_ACTUATOR_STATUS;

	}

	if (cm->msg_type == STA_INFO::SND_ACK_STATUS_PAIR) {
		snd_msg.active = MSG_ACTIVE::ANSWER;												// ack is always an answer, set address, counter and to make it active

	} else if (cm->msg_type == STA_INFO::SND_ACK_STATUS_PEER) {
		snd_msg.active = MSG_ACTIVE::ANSWER;												// ack is always an answer, set address, counter and to make it active

		cm->msg_type = STA_INFO::SND_ACTUATOR_STATUS;										// message to peer was initiated, we need to send the status also to the master
		cm->msg_delay.set(cm->status_delay);												// with some delay

	} else if (cm->msg_type == STA_INFO::SND_ACTUATOR_STATUS_ANSWER) {
		snd_msg.active = MSG_ACTIVE::ANSWER_BIDI;											// answer to a status request

	} else if (cm->msg_type == STA_INFO::SND_ACTUATOR_STATUS) {
		snd_msg.active = MSG_ACTIVE::PAIR;													// initial status send to pair

	}

	/* check if it is a stable status, otherwise schedule next check */
	cm->msg_type = STA_INFO::SND_ACTUATOR_STATUS;											// set and actuator status as default, while not activated yet
	if ((cm->sf.UP) || (cm->sf.DOWN) || (cm->sf.DELAY)) {									// check if we have an status change active

		if (cm->sm_delay.remain() < cm->status_delay) cm->msg_delay.set(cm->status_delay);	// and choose the next lookup time accordingly
		else cm->msg_delay.set(cm->sm_delay.remain() + 100);

	} else 
		cm->msg_type = STA_INFO::NOTHING;													// no need for next time
}


//- channel master related helpers ----------------------------------------------------------------------------------------
/*
* @brief Prepare defaults and read the defaults from the eeprom in the channel module space.
*        We have to read only list0 or list 1 content, while list 3 or list 4 is read while received a peer message.
*        After loading the defaults we inform the modules of this by calling the info_config_change function.
*/
uint16_t cm_prep_default(uint16_t ee_start_addr) {

	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels

		cmm[i]->list[cmm[i]->lstC.lst] = &cmm[i]->lstC;										// allign lstC to the list array
		if (cmm[i]->lstP.lst != 255) cmm[i]->list[cmm[i]->lstP.lst] = &cmm[i]->lstP;		// because of the absence of lstP in channel0

		cmm[i]->lstC.ee_addr = ee_start_addr;												// write the eeprom address in the channel list
		ee_start_addr += cmm[i]->lstC.len;													// create new address by adding the length of the list before
		cmm[i]->lstP.ee_addr = ee_start_addr;												// write the eeprom address in the peer list
		ee_start_addr += (cmm[i]->lstP.len * cmm[i]->peerDB.max);							// create new address by adding the length of the list before but while peer list, multiplied by the amount of possible peers

		// defaults loaded in the AS module init, on every time start
		DBG(CM, F("CM:prep_defaults, cnl:"), cmm[i]->lstC.cnl, F(", lst:"), cmm[i]->lstC.lst, F(", len:"), cmm[i]->lstC.len, F(", data:"), _HEX(cmm[i]->lstC.val, cmm[i]->lstC.len), '\n');
		//DBG(CM, F("CM:prep_defaults, list_ptr: "), (uint16_t)cmm[i]->list[0], F(", "), (uint16_t)cmm[i]->list[1], F(", "), (uint16_t)cmm[i]->list[2], F(", "), (uint16_t)cmm[i]->list[3], F(", "), (uint16_t)cmm[i]->list[4], F(", "), (uint16_t)cmm[i]->list[5], F(", "), '\n');
	}

	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		cmm[i]->peerDB.ee_addr = ee_start_addr;												// write eeprom address into the peer table
		ee_start_addr += cmm[i]->peerDB.max * 4;											// create nwe eeprom start address depending on the space for max peers are used
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
	uint16_t flashCRC = 0;																	// define and set the return value

	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		s_peer_table *pPeer = &cmm[i]->peerDB;												// short hand to the peer database
		for (uint8_t j = 0; j < list_max; j++) {											// step through all lists
			s_list_table *pList = cmm[i]->list[j];											// short hand to list module
			if (!pList) continue;															// skip on empty pointer
			flashCRC = crc16_P(flashCRC, pList->len, pList->reg);							// and calculate the crc - arrays are in PROGMEM
			flashCRC = crc16_P(flashCRC, pList->len, pList->def);
			flashCRC = crc16(flashCRC, pPeer->max);
			DBG(CM, F("CM:calc_crc cnl:"), i, F(", crc:"), flashCRC, '\n');					// some debug
		}
	}
	return flashCRC;
}
uint16_t crc16_P(uint16_t crc, uint8_t len, const uint8_t *buf) {
	for (uint8_t i = 0; i < len; i++) {														// step through all channels
		uint8_t t = _PGM_BYTE(buf[i]);
		crc = crc16(crc, t);
	}
	return crc;
}
uint16_t crc16(uint16_t crc, uint8_t a) {
	crc ^= a;
	for (uint8_t i = 0; i < 8; ++i) {
		if (crc & 1) crc = (crc >> 1) ^ 0xA001;
		else crc = (crc >> 1);
	}
	return crc;
}

void inform_config_change(uint8_t channel) {
	for (uint8_t i = 0; i < cnl_max; i++) {
		cmm[i]->info_config_change(channel);												// inform the channel module on a change of list0 or 1
	}
}
//-------------------------------------------------------------------------------------------------------------------------














//- helpers ---------------------------------------------------------------------------------------------------------------














