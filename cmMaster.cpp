/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "cmMaster.h"


uint8_t cnl_max = 0;																		// defined in cmMaster.h, increased by every instance which is initialized


//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const uint8_t peer_max) {
	peerDB.max = peer_max;

	lstC.cnl = cnl_max;																		// set the channel to the lists
	lstP.cnl = cnl_max++;

	DBG(CM, F("CM["), lstC.cnl, F("].\n"));
}



/**
* @brief Virtual function is called while we received a new list0 or list1
* Herewith we can adapt changes given by the config change. Needs to be overwritten
* by the respective channel module
*/
void cmMaster::info_config_change(void) {
	DBG(CM, F("CM:config_change\n") );
}
/**
* we received an peer add event, which means, there was a peer added in this respective channel
* 1st 3 bytes shows the peer address, 4th and 5th byte gives the peer channel
* no need for sending an answer here, for information only
*/
void cmMaster::info_peer_add(s_m01xx01 *buf) {
	DBG(CM, F("CM:info_peer_add, peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n');
}

void cmMaster::request_peer_defaults(uint8_t idx, s_m01xx01 *buf) {
	DBG(CM, F("CM:request_peer_defaults, idx:"), _HEXB(idx), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n' );
}


void cmMaster::init(void) {
	cm_init();																				// init the virtual poll function
}

void cmMaster::poll(void) {
	cm_poll();																				// poll the virtual poll function 
}

/*
* @brief This function is called at the moment by the config button class, it is to toogle the output
* of an alligned channel. Needs to be overwritten by any actor class 
*/
void cmMaster::set_toggle(void) {
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
void cmMaster::CONFIG_PEER_ADD(s_m01xx01 *buf) {
	uint8_t *temp_peer = new uint8_t[4];													// temp byte array to load peer addresses
	uint8_t ret_byte = 0;																	// prepare a placeholder for success reporting

	for (uint8_t i = 0; i < 2; i++) {														// standard gives 2 peer channels
		if (!buf->PEER_CNL[i]) continue;													// if the current peer channel is empty, go to the next entry

		memcpy(temp_peer, buf->PEER_ID, 3);													// copy the peer address into the temp array
		temp_peer[3] = buf->PEER_CNL[i];													// write the peer channel byte into the array

		uint8_t idx = peerDB.get_idx(temp_peer);											// search if we have already the peer in the database
		if (idx == 0xff) idx = peerDB.get_free_slot();										// not in the in the database, search a free slot

		if (idx != 0xff) {																	// free slot available
			peerDB.set_peer(idx, temp_peer);												// write the peer into the database

			lstP.load_default();															// copy the defaults from progmem into the peer list, index doesn't matter
			request_peer_defaults(idx, buf);												// ask the channel module to load the defaults
			lstP.save_list(idx);															// and save the list, index is important while more choices in the peer table
			ret_byte++;																		// increase success
		}
	}
	info_peer_add(buf);																		// inform the user module of the added peer

	DBG(CM, F("CM:CONFIG_PEER_ADD, cnl:"), buf->MSG_CNL, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", RET:"), ret_byte, '\n');
	check_send_ACK_NACK(ret_byte);
}
/*
* @brief Removes one or two peers from a channel
* CONFIG_PEER_REMOVE message is send by the HM master to remove the binding of two client devices
* request is forwarded by the AS:processMessage function
*/
void cmMaster::CONFIG_PEER_REMOVE(s_m01xx02 *buf) {
	uint8_t *temp_peer = new uint8_t[4];													// temp byte array to load peer addresses
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
	DBG(CM, F("CM:CONFIG_PEER_REMOVE, cnl:"), buf->MSG_CNL, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n');
	check_send_ACK_NACK(ret_byte);
}
/*
* @brief Requests a listing of all registered peers from a specific channel
* CONFIG_PEER_LIST_REQ message is send by the HM master to get information which peers are known at the client device.
* This type of message can't be answered within one string, therefor we send an ACK on the initial list request
* and prepare a struct with the required information to process it further in a seperate function call
* request is forwarded by the AS:processMessage function
*/
void cmMaster::CONFIG_PEER_LIST_REQ(s_m01xx03 *buf) {
	DBG(CM, F("CM:CONFIG_PEER_LIST_REQ\n"));
	send_ACK();
	send_INFO_PEER_LIST(buf->MSG_CNL);
}
/*
* @brief Requests a listing of all values in a list from a specific channel
* CONFIG_PARAM_REQ message is send by the HM master to get information which list values are stored at the client device.
* This type of message can't be answered within one string, therefor we send an ACK on the initial list request
* and prepare a struct with the required information to process it further in a seperate function call
* request is forwarded by the AS:processMessage function
*/
void cmMaster::CONFIG_PARAM_REQ(s_m01xx04 *buf) {
	DBG(CM, F("CM:CONFIG_PARAM_REQ\n"));
	send_ACK();
	send_INFO_PARAM_RESPONSE_PAIRS(buf->MSG_CNL, buf->PARAM_LIST, buf->PEER_ID);
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
void cmMaster::CONFIG_START(s_m01xx05 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct
	cm->list = list[buf->PARAM_LIST];														// short hand to the list table
	cm->idx_peer = peerDB.get_idx(buf->PEER_ID);											// try to get the peer index

	if ((cm->list) && (cm->idx_peer != 0xff)) {												// list and peer index found
		cm->timer.set(2000);																// set timeout time, otherwise the channel will be open for write forever
		cm->active = 1;																		// set active
		send_ACK();																			// send back that everything is ok
		// TODO: set message id flag to config in send module
	} else {
		send_NACK();																		// something wrong
	}
	DBG(CM, F("CM:CONFIG_START, cnl:"), buf->MSG_CNL, '/', cm->list->cnl, F(", lst:"), buf->PARAM_LIST, '/', cm->list->cnl, F(", peer:"), _HEX(buf->PEER_ID, 4), F(", idx:"), cm->idx_peer, '\n');
}
/*
* @brief Config end indicates that changes are written to the respective list and no access is needed any more
* Within this function we call the respective channel module and inform of a change in any list
* Message description:
*             Sender__ Receiver    Channel
* 10 04 A0 01 63 19 63 01 02 04 01 06
*/
void cmMaster::CONFIG_END(s_m01xx06 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct
	cm->timer.set(0);																		// clear the timer
	cm->active = 0;																			// clear the flag
	send_ACK();																				// send back that everything is ok

	if (cm->list->lst < 2) {
		lstC.load_list(cm->idx_peer);														// reload list0 or 1
		info_config_change();																// inform the channel module on a change of list0 or 1
	}
	// TODO: remove message id flag to config in send module
	DBG(CM, F("CM:CONFIG_END, cnl:"), buf->MSG_CNL, '\n');
}
/* 
* todo: implement
*/
void cmMaster::CONFIG_WRITE_INDEX1(s_m01xx07 *buf) {
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
void cmMaster::CONFIG_WRITE_INDEX2(s_m01xx08 *buf) {
	s_config_mode *cm = &config_mode;														// short hand to config mode struct

	if (cm->active)  {																		// check if config is active, channel fit is checked in AS
		cm->list->write_array(buf->DATA, buf->MSG_LEN - 11, cm->idx_peer);					// write the array into the list
		send_ACK();																			// we are fine
		DBG(CM, F("CM:CONFIG_WRITE_INDEX2, cnl:"), buf->MSG_CNL, F(", lst:"), cm->lst, F(", idx:"), cm->idx_peer, '\n');
	} else send_NACK();
}
/*
* @brief Process message CONFIG_SERIAL_REQ.
*
* Message description:
*             Sender__ Receiver
* 0B 77 A0 01 63 19 63 01 02 04 00 09
*/
void cmMaster::CONFIG_SERIAL_REQ(s_m01xx09 *buf) {
	send_INFO_SERIAL();
	DBG(CM, F("CM:CONFIG_SERIAL_REQ\n"));
}
/*
* @brief Process message CONFIG_PAIR_SERIAL
*
* Message description:
*             Sender__ Receiver       SerialNumber
* 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
*/
void cmMaster::CONFIG_PAIR_SERIAL(s_m01xx0a *buf) {
	DBG(CM, F("CM:CONFIG_PAIR_SERIAL, cnl:"), _HEX(buf->SERIALNO,10), '\n');

	if (isEqual(buf->SERIALNO, dev_ident.SERIAL_NR, 10)) 									// compare serial and send device info
		send_DEVICE_INFO(MSG_REASON::ANSWER);
}
/*
* @brief Process message CONFIG_PAIR_SERIAL
* Virtual function to be overwritten by the respective channel module
*
* Message description:
*                Sender__  Receiver  CNL   BY11
* l> 0B 40 A0 01 63 19 64  23 70 D8  01    0E     
*/
void cmMaster::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {
	DBG(CM, F("CM:CONFIG_STATUS_REQUEST\n"));
}


void cmMaster::ACK(s_m0200xx *buf) {
	DBG(CM, F("CM:ACK\n"));
}
void cmMaster::ACK_STATUS(s_m0201xx *buf) {
	DBG(CM, F("CM:ACK_STATUS\n"));
}
void cmMaster::ACK2(s_m0202xx *buf) {
	DBG(CM, F("CM:ACK2\n"));
}
void cmMaster::AES_REQ(s_m0204xx *buf) {
	DBG(CM, F("CM:AES_REQ\n"));
}
void cmMaster::NACK(s_m0280xx *buf) {
	DBG(CM, F("CM:NACK\n"));
}
void cmMaster::NACK_TARGET_INVALID(s_m0284xx *buf) {
	DBG(CM, F("CM:NACK_TARGET_INVALID\n"));
}
void cmMaster::ACK_NACK_UNKNOWN(s_m02xxxx *buf) {
	DBG(CM, F("CM:ACK_NACK_UNKNOWN\n"));
}


void cmMaster::AES_REPLY(s_m03xxxx *buf) {
	DBG(CM, F("CM:AES_REPLY\n"));
}


void cmMaster::SEND_AES_TO_HMLAN(s_m0401xx *buf) {
	DBG(CM, F("CM:SEND_AES_TO_HMLAN\n"));
}
void cmMaster::SEND_AES_TO_ACTOR(s_m04xxxx *buf) {
	DBG(CM, F("CM:SEND_AES_TO_ACTOR\n"));
}

void cmMaster::INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_INHIBIT_OFF\n"));
}
void cmMaster::INSTRUCTION_INHIBIT_ON(s_m1101xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_INHIBIT_ON\n"));
}
void cmMaster::INSTRUCTION_SET(s_m1102xx *buf) {
	dbg << "CM set\n";
	DBG(CM, F("CM:INSTRUCTION_SET\n"));
}
void cmMaster::INSTRUCTION_STOP_CHANGE(s_m1103xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_STOP_CHANGE\n"));
}
/*
* @brief Reset to factory defaults
*/
void cmMaster::INSTRUCTION_RESET(s_m1104xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_RESET\n"));
	send_ACK();																				// prepare an ACK message
	while (snd_msg.active) snd.poll();														// poll to get the ACK message send
	clearEEPromBlock(0, 2);																	// delete the magic byte in eeprom 
	hm.init();																				// call the init function to get the device in factory status
}
void cmMaster::INSTRUCTION_LED(s_m1180xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LED\n"));
}
void cmMaster::INSTRUCTION_LED_ALL(s_m1181xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LED_ALL\n"));
}
void cmMaster::INSTRUCTION_LEVEL(s_m1181xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_LEVEL\n"));
}
void cmMaster::INSTRUCTION_SLEEPMODE(s_m1182xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_SLEEPMODE\n"));
}
void cmMaster::INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_ENTER_BOOTLOADER\n"));
}
void cmMaster::INSTRUCTION_SET_TEMP(s_m1186xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_SET_TEMP\n"));
}
void cmMaster::INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_ADAPTION_DRIVE_SET\n"));
}
void cmMaster::INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf) {
	DBG(CM, F("CM:INSTRUCTION_ENTER_BOOTLOADER2\n"));
}


void cmMaster::HAVE_DATA(s_m12xxxx *buf) {
	DBG(CM, F("CM:HAVE_DATA\n"));
}


void cmMaster::SWITCH(s_m3Exxxx *buf) {
	DBG(CM, F("CM:SWITCH\n"));
}
void cmMaster::TIMESTAMP(s_m3fxxxx *buf) {
	DBG(CM, F("CM:TIMESTAMP\n"));
}
void cmMaster::REMOTE(s_m40xxxx *buf) {
	DBG(CM, F("CM:REMOTE\n"));
}
void cmMaster::SENSOR_EVENT(s_m41xxxx *buf) {
	DBG(CM, F("CM:SENSOR_EVENT\n"));
}
void cmMaster::SWITCH_LEVEL(s_m42xxxx *buf) {
	DBG(CM, F("CM:SWITCH_LEVEL\n"));
}
void cmMaster::SENSOR_DATA(s_m53xxxx *buf) {
	DBG(CM, F("CM:SENSOR_DATA\n"));
}
void cmMaster::GAS_EVENT(s_m54xxxx *buf) {
	DBG(CM, F("CM:GAS_EVENT\n"));
}
void cmMaster::CLIMATE_EVENT(s_m58xxxx *buf) {
	DBG(CM, F("CM:CLIMATE_EVENT\n"));
}
void cmMaster::SET_TEAM_TEMP(s_m59xxxx *buf) {
	DBG(CM, F("CM:SET_TEAM_TEMP\n"));
}
void cmMaster::THERMAL_CONTROL(s_m5axxxx *buf) {
	DBG(CM, F("CM:THERMAL_CONTROL\n"));
}
void cmMaster::POWER_EVENT_CYCLE(s_m5exxxx *buf) {
	DBG(CM, F("CM:POWER_EVENT_CYCLE\n"));
}
void cmMaster::POWER_EVENT(s_m5fxxxx *buf) {
	DBG(CM, F("CM:POWER_EVENT\n"));
}
void cmMaster::WEATHER_EVENT(s_m70xxxx *buf) {
	DBG(CM, F("CM:WEATHER_EVENT\n"));
}













//- helpers ---------------------------------------------------------------------------------------------------------------
/*
* @brief Sends the ACK_STATUS and INFO_ACTUATOR_STATUS based on content of send_stat struct
* polled by cmMaster poll function.
*/
void send_status(s_cm_status *cm, uint8_t cnl) {
	AS *phm = &hm;																			// short hand to main class

	if (snd_msg.active) return;																// send has already something to do
	if (!cm->message_type) return;															// nothing to do
	if (!cm->message_delay.done()) return;													// not the right time

	/* prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80 */
	cm->flag = 0;
	if      (cm->value <  cm->set_value) cm->f.UP = 1;
	else if (cm->value >  cm->set_value) cm->f.DOWN = 1;

	if (!cm->delay.done())               cm->f.DELAY = 1;
	if (bat.getStatus())                 cm->f.LOWBAT = 1;;

	/* check which type has to be send - if it is an ACK and modDUL != 0, then set timer for sending a actuator status */
	if (cm->message_type == INFO::SND_ACK_STATUS)
		send_ACK_STATUS(cnl, cm->value, cm->flag);
	else if (cm->message_type == INFO::SND_ACTUATOR_STATUS)
		send_INFO_ACTUATOR_STATUS(cnl, cm->value, cm->flag);
	else if (cm->message_type == INFO::SND_ACTUATOR_STATUS_AGAIN)
		send_INFO_ACTUATOR_STATUS(cnl, cm->value, cm->flag);

	/* check if it is a stable status, otherwise schedule next check */
	if (cm->f.DELAY) {																		// status is currently changing
		cm->message_type = INFO::SND_ACTUATOR_STATUS_AGAIN;									// send next time a info status message
		cm->message_delay.set(cm->delay.remain() + 100);									// check again when timer is finish

	} else cm->message_type = INFO::NOTHING;												// no need for next time

}

/*
* @brief Prepare defaults and read the defaults from the eeprom in the channel module space.
*        We have to read only list0 or list 1 content, while list 3 or list 4 is read while received a peer message.
*        After loading the defaults we inform the modules of this by calling the info_config_change function.
*/
uint16_t cm_prep_default(uint16_t ee_start_addr) {

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		
		pCM->list[pCM->lstC.lst] = &pCM->lstC;											// allign lstC to the list array
		if (pCM->lstP.lst < 5) pCM->list[pCM->lstP.lst] = &pCM->lstP;					// because of the absence of lstP in channel0

		pCM->lstC.ee_addr = ee_start_addr;												// write the eeprom address in the channel list
		ee_start_addr += pCM->lstC.len;													// create new address by adding the length of the list before
		pCM->lstP.ee_addr = ee_start_addr;												// write the eeprom address in the peer list
		ee_start_addr += (pCM->lstP.len * pCM->peerDB.max);								// create new address by adding the length of the list before but while peer list, multiplied by the amount of possible peers

		// defaults loaded in the AS module init, on every time start
		DBG(CM, F("CM:prep_defaults, cnl:"), pCM->lstC.cnl, F(", lst:"), pCM->lstC.lst, F(", len:"), pCM->lstC.len, F(", data:"), _HEX(pCM->lstC.val, pCM->lstC.len), '\n');
	}

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		pCM->peerDB.ee_addr = ee_start_addr;											// write eeprom address into the peer table
		ee_start_addr += pCM->peerDB.max * 4;											// create nwe eeprom start address depending on the space for max peers are used
	}

	return ee_start_addr;
}

/*
* @brief Search for a 4 byte peer address in all channel instances and returns
*        the channel number where the peer was found. Returns 0 if nothing was found.
*/
uint8_t  is_peer_valid(uint8_t *peer) {
	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		cmMaster *pCM = ptr_CM[i];														// short hand to respective channel master	
		if (pCM->peerDB.get_idx(peer) != 0xff) return i;								// ask the peer table to find the peer, if found, return the cnl
	}
	return 0;																			// nothing was found, return 0
}

/*
* @brief Calculates and returns the crc number for all channel module lists.
* This information is needed for the first time check. Within this check, we compare a stored
* magic number with the calculated one and if they differ, we have a good indication that something
* was changed in the configuration.
*/
uint16_t cm_calc_crc(void) {
	uint16_t flashCRC = 0;																// define and set the return value

	for (uint8_t i = 0; i < cnl_max; i++) {												// step through all channels
		s_peer_table *pPeer = &ptr_CM[i]->peerDB;										// short hand to the peer database
		for (uint8_t j = 0; j < list_max; j++) {										// step through all lists
			s_list_table *pList = ptr_CM[i]->list[j];									// short hand to list module
			if (!pList) continue;														// skip on empty pointer
			flashCRC = crc16_P(flashCRC, pList->len, pList->reg);						// and calculate the crc - arrays are in PROGMEM
			flashCRC = crc16_P(flashCRC, pList->len, pList->def);
			flashCRC = crc16(flashCRC, pPeer->max);
			DBG(CM, F("CM:calc_crc cnl:"), i, F(", crc:"), flashCRC, '\n');				// some debug
		}
	}
	return flashCRC;
}
inline uint16_t crc16_P(uint16_t crc, uint8_t len, const uint8_t *buf) {
	for (uint8_t i = 0; i < len; i++) {													// step through all channels
		uint8_t t = _PGM_BYTE(buf[i]);
		crc = crc16(crc, t);
	}
	return crc;
}
inline uint16_t crc16(uint16_t crc, uint8_t a) {
	crc ^= a;
	for (uint8_t i = 0; i < 8; ++i) {
		if (crc & 1) crc = (crc >> 1) ^ 0xA001;
		else crc = (crc >> 1);
	}
	return crc;
}





/**
* @brief Send device info
* Could be an answer on a pair serial request, or initial send by the device
* Only difference is the message counter - on own initialized send, we use the message
* counter from the send class, on a pair serial request we take the counter value from the received string
*
* Message description:
*             Sender__ Receiver fwVer type   Serial number                     class  pCnlA  pCnlB  unknown
* 1A 94 84 00 1F B7 4A 01 02 04 15    00 6C  4B 45 51 30 32 33 37 33 39 36  10 41     01     00
*/
void send_DEVICE_INFO(MSG_REASON::E reason) {
	s_m00xxxx *msg = &snd_msg.m00xxxx;														// short hand to send buffer

	/* copy the payload from different sources */
	memcpy_P(&msg->FIRMWARE, dev_static, 3);												// copy firmware and modelID
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);										// copy the serial number
	memcpy_P(&msg->CLASS, dev_static + 3, 4);												// copy subtype and device info

	/* is it an answer to a CONFIG_PAIR_SERIAL request, or while we initiate the pairing process */
	if (reason == MSG_REASON::ANSWER) {
		snd_msg.active = MSG_ACTIVE::ANSWER;												// for address, counter and to make it active
	} else {
		snd_msg.active = MSG_ACTIVE::PAIR;													// for address, counter and to make it active
	}

	/* BIDI is asked all time, will removed automatically if MAID is empty */
	snd_msg.type = MSG_TYPE::DEVICE_INFO;													// length and flags are set within the snd_msg struct

	//pair_mode.active = 1;																	// set pairing flag
	//pair_mode.timer.set(20000);															// set pairing time
	led.set(pairing);																		// and visualize the status
}
/**
* @brief Check if ACK required and send ACK or NACK
*/
void check_send_ACK_NACK(uint8_t ackOk) {
	if (ackOk) send_ACK();																	// we are here to proof the result
	else send_NACK();																		// and send an ACK or NACK
}
/**
* @brief Send ACK message
*
* Message description:
*             Sender__ Receiver ACK
* 0A 24 80 02 1F B7 4A 63 19 63 00
*/
void send_ACK(void) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ack only if required
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::ACK;															// length and flags are set within the snd_msg struct
}
/**
* @brief Send an ACK with status data
*
* Message description:
*             Sender__ Receiver ACK Cnl Stat Action RSSI
* 0F 12 80 02 1E 7A AD 23 70 EC 01  01  BE   20     27    CC - dimmer
* 0E 5C 80 02 1F B7 4A 63 19 63 01  01  C8   00     42       - pcb relay
*
* Action: Down=0x20, UP=0x10, LowBat=&0x80
*
* @param chnl
* @param stat
* @param actn
*/
void send_ACK_STATUS(uint8_t chnl, uint8_t stat, uint8_t actn) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	s_m0201xx *msg = &snd_msg.m0201xx;														// short hand to ACK Status struct while easier to fill
	msg->MSG_CNL = chnl;
	msg->MSG_STATUS = stat;
	*(uint8_t*)&msg->MSG_FLAG = actn;
	msg->MSG_FLAG.LOWBAT = bat.getStatus();
	msg->MSG_RSSI = cc.rssi;

	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::ACK_STATUS;													// length and flags are set within the snd_msg struct
}
void send_ACK2(void) {
}
void send_AES_REQ(s_m0204xx *buf) {
}
/**
* @brief Send a NACK (not ACK)
*
* Message description:
*             Sender__ Receiver NACK
* 0A 24 80 02 1F B7 4A 63 19 63 80
*/
void send_NACK(void) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ack only if required
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::NACK;															// length and flags are set within the snd_msg struct

}
/**
* @brief Send a NACK (not ACK and target invalid)
*
* Message description:
*             Sender__ Receiver NACK_TAGRET_INVALID
* 0A 24 80 02 1F B7 4A 63 19 63 84
*/
void send_NACK_TARGET_INVALID(void) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::NACK_TARGET_INVALID;											// length and flags are set within the snd_msg struct

}
void send_ACK_NACK_UNKNOWN() {
}

void send_AES_REPLY() {
}

void send_SEND_AES_TO_HMLAN() {
}
void send_SEND_AES_TO_ACTOR() {
}

/*
* @brief Transmit the device serial number
*
*    LEN CNT FLAG BY03 SND       RCV       By10  SERIAL
* m> 14  16  A0   10   63 19 64  33 11 22  00    01 02 03 04 05 06 07 08 09 10 * /
*/
void send_INFO_SERIAL() {
	s_m1000xx *msg = &snd_msg.m1000xx;														// short hand to info serial struct
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);										// copy the serial number

	//snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;										// as it is an answer, we reflect the counter in the answer
	//snd_msg.set_msg(MSG_TYPE::INFO_SERIAL, rcv_msg.mBody.SND_ID, rcv_msg.mBody.MSG_CNT);
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::INFO_SERIAL;													// length and flags are set within the snd_msg struct
}

/**
* @brief Send info peer list
* Typical requested by a CONFIG_PEER_LIST_REQ. As this message could require more bytes then
* it can send within one message, we prepare a slice wise sender struct. Progressed in the send class.
*
* Message description:
*    LEN CNT FLAG BY03 SND       RCV       By10  PEER 1 to 4, empty peer terminates message
* l> 0E  35  A0   10   23 70 D8  63 19 64  01    00 00 00 00
*/
void send_INFO_PEER_LIST(uint8_t cnl) {
	s_config_list_answer_slice *cl = &config_list_answer_slice;								// short hand to the struct with all information for slice wise send
	s_peer_table *peerDB = &ptr_CM[cnl]->peerDB;											// short hand to the respective peer table of the channel

	cl->type = LIST_ANSWER::PEER_LIST;														// we want to get the peer list
	cl->peer = peerDB;																		// pointer to the respective peerDB struct
	cl->max_slc = peerDB->get_nr_slices();													// get an idea of the total needed slices
	cl->timer.set(50);																		// some time between last message
	cl->active = 1;																			// and set it active
	DBG(AS, F("AS:send_INFO_PEER_LIST, cnl:"), cnl, F(", slices:"), cl->max_slc, '\n');
}

/**
* @brief Send info param response pairs of values
* Typical requested by a CONFIG_PARAM_REQ. As this message could require more bytes then
* it can send within one message, we prepare a slice wise sender struct. Progressed in the send class.
*
* Message description:
*    LEN CNT FLAG BY03 SND       RCV       By10  PEER 1 to 4, empty peer terminates message
* l> 0E  35  A0   10   23 70 D8  63 19 64  01    00 00 00 00
*/
void send_INFO_PARAM_RESPONSE_PAIRS(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
	s_config_list_answer_slice *cl = &config_list_answer_slice;								// short hand to the struct with all information for slice wise send
	s_peer_table *peerDB = &ptr_CM[cnl]->peerDB;											// short hand to the respective peer table of the channel
	s_list_table *list = ptr_CM[cnl]->list[lst];											// short hand to the respective list table

	if (!list) return;																		// specific list is not available
	uint8_t idx = peerDB->get_idx(peer_id);													// get the requested peer index
	if (idx == 0xff) return;																// nothing to do while index not found

	cl->type = LIST_ANSWER::PARAM_RESPONSE_PAIRS;											// we want to get the param list as pairs
	cl->peer_idx = peerDB->get_idx(peer_id);												// remember on the peer index
	cl->list = list;																		// pointer to the respective list struct
	cl->max_slc = list->get_nr_slices_pairs() + 1;											// get an idea of the total needed slices, plus one for closing 00 00 message
	cl->timer.set(50);																		// some time between last message
	cl->active = 1;																			// and set it active, while peer index is available
	DBG(AS, F("AS:send_INFO_PARAM_RESPONSE_PAIRS, cnl:"), cnl, F(", lst:"), lst, F(", peer:"), _HEX(peer_id, 4), F(", idx:"), cl->peer_idx, F(", slices:"), cl->max_slc, '\n');
}

/**
* @brief Send info param response sequential - todo: implement
* Typical requested by a CONFIG_PARAM_REQ. As this message could require more bytes then
* it can send within one message, we prepare a slice wise sender struct. Progressed in the send class.
*
* Message description:
*    LEN CNT FLAG BY03 SND       RCV       By10  PEER 1 to 4, empty peer terminates message
* l> 0E  35  A0   10   23 70 D8  63 19 64  01    00 00 00 00
*/
void send_INFO_PARAM_RESPONSE_SEQ(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
	//config_list_answer_slice.type = LIST_ANSWER::PARAM_RESPONSE_PAIRS;;					// we want to get the param list sequential
	//config_list_answer_slice.list = ptr_CM[cnl]->list[lst];								// pointer to the respective list struct
	//config_list_answer_slice.max_slc = ptr_CM[cnl]->list[lst]->get_nr_slices();			// get an idea of the total needed slices
	//config_list_answer_slice.active = 1;													// and set it active
}

void send_INFO_PARAMETER_CHANGE() {
}

/**
* @brief Send info about an actor status
*
* Message description:
*             Sender__  Receiver       Cnl  Stat flag  RSSI
* 0E 40 A4 10 1F B7 4A  63 19 63  06   01   00   00    48 (148679)
*
* @param channel
* @param state
* @param flag: TODO: to be specified
*/
void send_INFO_ACTUATOR_STATUS(uint8_t cnl, uint8_t stat, uint8_t flag) {
	s_m1006xx *msg = &snd_msg.m1006xx;														// struct is easier to fill
	s_mBody *rcvBody = &rcv_msg.mBody;														// short hand to received string
	uint8_t bidi = 0;																		// per default we don't need an ACK

	msg->MSG_CNL = cnl;																		// copy in the channel
	msg->MSG_STAT = stat;																	// the status of the channel
	msg->UNKNOWN = flag;																	// needs investigation
	msg->MSG_RSSI = cc.rssi;																// received rssi value

	snd_msg.active = MSG_ACTIVE::PAIR;														// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::INFO_ACTUATOR_STATUS;											// length and flags are set within the snd_msg struct
}

void send_INFO_TEMP() {
}

void send_HAVE_DATA() {
}

void send_SWITCH(s_peer_table *peerDB) {
}
void send_TIMESTAMP(s_peer_table *peerDB) {
}
void send_REMOTE(uint8_t bidi, cmMaster *channel_module, uint8_t *ptr_payload) {
	if (peer_msg.active) return;
	peer_msg.active = (bidi) ? MSG_ACTIVE::PEER_BIDI : MSG_ACTIVE::PEER;
	peer_msg.type = MSG_TYPE::REMOTE;
	peer_msg.peerDB = &channel_module->peerDB;
	peer_msg.lstP = &channel_module->lstP;
	peer_msg.lstC = &channel_module->lstC;
	peer_msg.payload_ptr = ptr_payload;
	peer_msg.payload_len = 2;
	peer_msg.max_retr = 3;
	DBG(CM, F("CM:send_REMOTE peers:"), channel_module->peerDB.used_slots(), F(", payload:"), _HEX(ptr_payload, 2), ", bidi:", bidi, '\n');
}
void send_SENSOR_EVENT(s_peer_table *peerDB) {
}
void send_SWITCH_LEVEL(s_peer_table *peerDB) {
}
void send_SENSOR_DATA(s_peer_table *peerDB) {
}
void send_GAS_EVENT(s_peer_table *peerDB) {
}
void send_CLIMATE_EVENT(s_peer_table *peerDB) {
}
void send_SET_TEAM_TEMP(s_peer_table *peerDB) {
}
void send_THERMAL_CONTROL(s_peer_table *peerDB) {
}
void send_POWER_EVENT_CYCLE(s_peer_table *peerDB) {
}
void send_POWER_EVENT(s_peer_table *peerDB) {
}
void send_WEATHER_EVENT(s_peer_table *peerDB) {
}

/*
* @brief poll function to process peer messages. 
* Respective information needs to be in peer_msg struct, while herein the string preparation for the 
* send function is done. This function needs to be polled by at least on channel module which uses
* one peer send function from the list above.
*/
void process_peer_message(void) {
	s_peer_msg *pm = &peer_msg;																// short hand to peer message struct
	s_snd_msg  *sm = &snd_msg;

	/* checks if a peer message needs to be processed and if send is busy */
	if (!pm->active) return;																// is there a peer message to send?
	if (sm->active) return;																	// has send function something else to do first?

	/* if we are here, it has one of the following reasons, first time call to send a peer message, prepare the pre-requisites,
	*  or we have sent a message earlier which is processed now, check if it was the last message to send, or process the next slot */
	/* first time message, check if peers are registered and prepare the peer slot table. if it is not the first time, then check 
	*  if the last message was not a timeout, cleanup the flag in the slot table and process the next message */
	if ((!pm->slot_cnt) && (!pm->retr_cnt)) {
		pm->prep_slot();
		pm->retr_cnt++;
		if (pm->active == MSG_ACTIVE::PEER) pm->max_retr = 1;								// todo: read it from list1 of the channel
		//dbg << "prepare the slot table - msg:" << ((pm->active == MSG_ACTIVE::PEER_BIDI) ? "PEER_BIDI" : "") << ((pm->active == MSG_ACTIVE::PEER) ? "PEER" : "") << ", slot_cnt:" << pm->slot_cnt << ", max_retr:" << pm->max_retr << ", retr_cnt:" << pm->retr_cnt << '\n';

	} else {
	/* it is not the first time call, check if the last round was not timeout, clean the flag in the slot counter (flag was not necassarily set) */
		if (!sm->timeout) pm->clear_slot(pm->slot_cnt);
		pm->slot_cnt++;

		/* start the next round while peer slot counter is above the limit; which indicates that all peers are processed */
		if (pm->slot_cnt >= pm->peerDB->max) {
			pm->retr_cnt++;
			//dbg << "slot_cnt:" << pm->slot_cnt << ", max_retr:" << pm->max_retr << ", retr_cnt:" << pm->retr_cnt << '\n';

			if (pm->retr_cnt > pm->max_retr) {												// check if we are done with all retries
			/* clean up the peer message processing while all retries are done */
				pm->clear();																// cleanup the struct
				sm->MSG_CNT++;																// and increase the message counter in the general send function for next time
				//dbg << "all peers done\n";
			} else {
			/* if we are not done, we start from begin of the slot table */	
				pm->slot_cnt = 0;	
				//dbg << "start next try\n";
			}
		}

		/* goto next slot while the current slot is empty */
		if (!pm->get_slot(pm->slot_cnt)) {
			//dbg << "p_cnt: " << pm->slot_cnt << " nothing to do, next...\n";
			return;
		}
	}

	/* build the message, set type, len, bidi and content */
	sm->type = pm->type;																	// copy the type into the send message struct

	/* take care of the payload - peer message means in any case that the payload starts at the same position in the string and
	*  while it could have a different length, we calculate the length of the string by a hand over value */
	sm->mBody.MSG_LEN = pm->payload_len + 9;
	memcpy(&sm->buf[10], pm->payload_ptr, pm->payload_len);

	/* send it as pair message if we have no peer registered */
	if (!pm->peerDB->used_slots()) {														// if no peer is registered, we send the message to the pair
		sm->active = MSG_ACTIVE::PAIR;														// should be handled from now on as a pair message
		pm->clear();																		// nothing to do here any more, while handled as pair message
		return;																				// and return, otherwise some infos are overwritten
	}

	/* set the peer address */
	memcpy(sm->mBody.RCV_ID, pm->peerDB->get_peer(pm->slot_cnt), 3);
	sm->temp_max_retr = 1;

	/* we have at least one peer to process, load the respective list4 to check if a burst is needed */
	pm->lstP->load_list(pm->slot_cnt);														// check if we need a burst, load the respective peer list
	struct s_0x01 {
		uint8_t PEER_NEEDS_BURST : 1;  // 0x01.0, s:1   d: false  
		uint8_t                  : 6;  // 0x01.1, s:6   d:   
		uint8_t EXPECT_AES       : 1;  // 0x01.7, s:1   d: false  
	};
	s_0x01 *flag = (s_0x01*)pm->lstP->ptr_to_val(0x01);										// set a pointer to the burst value
	sm->mBody.FLAG.BURST = flag->PEER_NEEDS_BURST;											// set the burst flag
	//dbg << "burst: " << flag->PEER_NEEDS_BURST << '\n';

	sm->active = pm->active;																// set it active
	snd.poll();																				// call send poll function direct, otherwise someone could change the snd_msg content
}






