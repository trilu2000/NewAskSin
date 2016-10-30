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

	DBG_START(CM, F("CM["), lstC.cnl, F("].\n"));
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
	hm.check_send_ACK_NACK(ret_byte);
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
	hm.check_send_ACK_NACK(ret_byte);
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
	hm.send_ACK();
	hm.send_INFO_PEER_LIST(buf->MSG_CNL);
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
	hm.send_ACK();
	hm.send_INFO_PARAM_RESPONSE_PAIRS(buf->MSG_CNL, buf->PARAM_LIST, buf->PEER_ID);
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
		hm.send_ACK();																		// send back that everything is ok
		// TODO: set message id flag to config in send module
	} else {
		hm.send_NACK();																		// something wrong
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
	hm.send_ACK();																			// send back that everything is ok

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
		hm.send_ACK();																		// we are fine
		DBG(CM, F("CM:CONFIG_WRITE_INDEX2, cnl:"), buf->MSG_CNL, F(", lst:"), cm->lst, F(", idx:"), cm->idx_peer, '\n');
	} else hm.send_NACK();
}
/*
* @brief Process message CONFIG_SERIAL_REQ.
*
* Message description:
*             Sender__ Receiver
* 0B 77 A0 01 63 19 63 01 02 04 00 09
*/
void cmMaster::CONFIG_SERIAL_REQ(s_m01xx09 *buf) {
	hm.send_INFO_SERIAL();
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
		hm.send_DEVICE_INFO(MSG_REASON::ANSWER);
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
	hm.send_ACK();																			// prepare an ACK message
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

	if (!cm->message_type) return;															// nothing to do
	if (!cm->message_delay.done()) return;													// not the right time

	/* prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80 */
	if      (cm->value == cm->set_value) cm->flag = 0;
	else if (cm->value <  cm->set_value) cm->f.UP = 1;
	else if (cm->value >  cm->set_value) cm->f.DOWN = 1;

	if (!cm->delay.done())               cm->f.DELAY = 1;
	if (bat.getStatus())                 cm->f.LOWBAT = 1;;

	/* check which type has to be send - if it is an ACK and modDUL != 0, then set timer for sending a actuator status */
	if (cm->message_type == INFO::SND_ACK_STATUS)
		phm->send_ACK_STATUS(cnl, cm->value, cm->flag);
	else if (cm->message_type == INFO::SND_ACTUATOR_STATUS)
		phm->send_INFO_ACTUATOR_STATUS(cnl, cm->value, cm->flag);
	else if (cm->message_type == INFO::SND_ACTUATOR_STATUS_AGAIN)
		phm->send_INFO_ACTUATOR_STATUS(cnl, cm->value, cm->flag);

	/* check if it is a stable status, otherwise schedule next check */
	if (cm->f.DELAY) {																		// status is currently changing
		cm->message_type = INFO::SND_ACTUATOR_STATUS_AGAIN;									// send next time a info status message
		cm->message_delay.set(cm->delay.remain() + 100);									// check again in 10 seconds

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