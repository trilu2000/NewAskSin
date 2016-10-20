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


void cmMaster::message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime) {
	DBG(CM, F("CM:trigger11, setValue:"), value, F(", rampTime:"), intTimeCvt(*(uint16_t*)rampTime), F(", duraTime:"), intTimeCvt(*(uint16_t*)duraTime), '\n' );
}

void cmMaster::message_trigger3E(uint8_t msgLng, uint8_t msgCnt) {
	DBG(CM, F("CM:trigger3E, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger40(uint8_t msgLng, uint8_t msgCnt) {
	DBG(CM, F("CM:trigger40, msgLng:"), msgLng, F(", msgCnt:"), msgCnt, '\n' );
}

void cmMaster::message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal) {
	DBG(CM, F("CM:trigger41, val:"), msgLng, '\n' );
}


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
	//if (rcv_msg.hasdata) processMessage();													// check if we have to handle the receive buffer
	cm_poll();
}



void cmMaster::set_toggle(void) {
	DBG(CM, F("CM:toggle\n") );
}



/*
* @brief Received message handling forwarded by AS::processMessage
*/
void cmMaster::processMessage() {
	DBG(CM, F("CM:processMessage cnl:"), lstC.cnl, '\n');

	if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::DEVICE_INFO)) {
		/* not sure what to do with while received, probably nothing */

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) {
		/* config request messages are used to configure a devive by writing registers and peers -
		*  check the channel and forward for processing to the respective function */
		if (rcv_msg.mBody.BY10 != lstC.cnl) return;											// check if channel is this
		
		uint8_t by11 = rcv_msg.mBody.BY11;													// short hand to byte 11 in the received string
		if      (by11 == BY11(MSG_TYPE::CONFIG_PEER_ADD))       CONFIG_PEER_ADD(&rcv_msg.m01xx01);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_REMOVE))    CONFIG_PEER_REMOVE(&rcv_msg.m01xx02);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_LIST_REQ))  CONFIG_PEER_LIST_REQ(&rcv_msg.m01xx03);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PARAM_REQ))      CONFIG_PARAM_REQ(&rcv_msg.m01xx04);
		else if (by11 == BY11(MSG_TYPE::CONFIG_START))          CONFIG_START(&rcv_msg.m01xx05);
		else if (by11 == BY11(MSG_TYPE::CONFIG_END))            CONFIG_END(&rcv_msg.m01xx06);
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX1))   CONFIG_WRITE_INDEX1(&rcv_msg.m01xx07);
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX2))   CONFIG_WRITE_INDEX2(&rcv_msg.m01xx08);
		else if (by11 == BY11(MSG_TYPE::CONFIG_SERIAL_REQ))     CONFIG_SERIAL_REQ(&rcv_msg.m01xx09);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL))    CONFIG_PAIR_SERIAL(&rcv_msg.m01xx0a);
		else if (by11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST)) CONFIG_STATUS_REQUEST(&rcv_msg.m01xx0e);
		rcv_msg.clear();																	// progressed, so nothing to do any more

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::ACK_MSG)) {
		/* at the moment we need the ACK message only for avoiding resends, so let the send_msg struct know about
		*  a received ACK/NACK whatever - probably we have to change this function in the future */
		if (rcv_msg.mBody.MSG_CNT == snd_msg.mBody.MSG_CNT) snd_msg.retr_cnt = 0xff;		// check if the message counter is similar and let the send function know
		rcv_msg.clear();																	// progressed, so nothing to do any more

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::AES_REPLY)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SEND_AES)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::REPLY_MSG)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::INSTRUCTION_MSG)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::HAVE_DATA)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SWITCH)) {

	} else if (rcv_msg.intent == MSG_INTENT::PEER) {
		if (rcv_msg.cnl != lstC.cnl) return;

		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::TIMESTAMP))         TIMESTAMP(&rcv_msg.m3fxxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::REMOTE))            REMOTE(&rcv_msg.m40xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SENSOR_EVENT))      SENSOR_EVENT(&rcv_msg.m41xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SWITCH_LEVEL))      SWITCH_LEVEL(&rcv_msg.m42xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SENSOR_DATA))       SENSOR_DATA(&rcv_msg.m53xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::GAS_EVENT))         GAS_EVENT(&rcv_msg.m54xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CLIMATE_EVENT))     CLIMATE_EVENT(&rcv_msg.m58xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SET_TEAM_TEMP))     SET_TEAM_TEMP(&rcv_msg.m59xxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::THERMAL_CONTROL))   THERMAL_CONTROL(&rcv_msg.m5axxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::POWER_EVENT_CYCLE)) POWER_EVENT_CYCLE(&rcv_msg.m5exxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::POWER_EVENT))       POWER_EVENT(&rcv_msg.m5fxxxx);
		if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::WEATHER_EVENT))     WEATHER_EVENT(&rcv_msg.m70xxxx);
		rcv_msg.clear();																	// progressed, so nothing to do any more

	} else {
		dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';
		DBG(AS, F("AS:message not known - please report: "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
	}
}





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

	if (cm->cnl < 2) {
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

	if ((cm->active) && (cm->cnl == buf->MSG_CNL)) {										// check if we are in config mode and if the channel fit
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
	DBG(CM, F("CM:INSTRUCTION_SET\n"));
}
void cmMaster::INSTRUCTION_STOP_CHANGE(s_m1103xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_STOP_CHANGE\n"));
}
void cmMaster::INSTRUCTION_RESET(s_m1104xx *buf) {
	DBG(CM, F("CM:INSTRUCTION_RESET\n"));
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