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
s_config_list_answer_slice config_list_answer_slice;										// defined in cmMaster.h, holds information to answer config list requests for peer or param lists


//public://------------------------------------------------------------------------------------------------------------------
cmMaster::cmMaster(const uint8_t peer_max) {
	peerDB.max = peer_max;

	lstC.cnl = cnl_max;																			// set the channel to the lists
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
void cmMaster::request_pair_status(void) {
	DBG(CM, F("CM:pair_status\n") );
}


void cmMaster::poll(void) {
	cm_poll();
	process_config_list_answer_slice();
}

void cmMaster::process_config_list_answer_slice(void) {
	if (!config_list_answer_slice.active) return;											// nothing to send, return
	if (snd_msg.active) return;																// we have something to do, but send_msg is busy

	s_config_list_answer_slice *cl = &config_list_answer_slice;								// short hand
	uint8_t payload_len;

	if (cl->type == LIST_ANSWER::PEER_LIST) {						// INFO_PEER_LIST
		payload_len = cl->peer->get_slice(cl->cur_slc, snd_msg.buf + 11);							// get the slice and the amount of bytes
		send_INFO_PEER_LIST(cl->RCV_ID, payload_len);															// create the body
		cl->cur_slc++;																			// increase slice counter
		//dbg << "peer slc: " << _HEX(snd_msg.buf, payload_len+11) << '\n';

	}
	else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_PAIRS) {		// INFO_PARAM_RESPONSE_PAIRS
		if (cl->cur_slc <= cl->max_slc) {															// within message processing, get the content													
			payload_len = cl->list->get_slice_pairs(cl->peer_idx, cl->cur_slc, snd_msg.buf + 11);		// get the slice and the amount of bytes
		}
		else {																			// last slice, terminating message
			payload_len = 2;																// reflect it in the payload_len
			memset(snd_msg.buf + 11, 0, payload_len);										// write terminating zeros
		}
		//sendINFO_PARAM_RESPONSE_PAIRS(payload_len);											// create the message
		cl->cur_slc++;																	// increase slice counter
																							//dbg << "reg2 slc: " << _HEX(snd_msg.buf,snd_msg.buf[0]+1) << '\n';				

	}
	else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_SEQ) {		// INFO_PARAM_RESPONSE_SEQ
															// not implemented yet
	}

	if (cl->cur_slc >= cl->max_slc) {																// if everything is send, we could stop the struct
		cl->active = 0;
		cl->cur_slc = 0;
	}
}

void cmMaster::set_toggle(void) {
	DBG(CM, F("CM:toggle\n") );
}



/*
* @brief Received message handling forwarded by AS::processMessage
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

	DBG(CM, F("CM:CONFIG_PEER_ADD, cnl:"), lstP.cnl, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), F(", RET:"), ret_byte, '\n');
	check_send_ACK_NACK(buf->SND_ID, ret_byte);
}

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

	DBG(CM, F("CM:CONFIG_PEER_REMOVE, cnl:"), lstC.cnl, F(", peer:"), _HEX(buf->PEER_ID, 3), F(", CNL_A:"), _HEXB(buf->PEER_CNL[0]), F(", CNL_B:"), _HEXB(buf->PEER_CNL[1]), '\n');
	check_send_ACK_NACK(buf->SND_ID, ret_byte);
}

void cmMaster::CONFIG_PEER_LIST_REQ(s_m01xx03 *buf) {
	send_ACK(buf->SND_ID);
	memcpy(config_list_answer_slice.RCV_ID, buf->SND_ID, 3);
	config_list_answer_slice.type = LIST_ANSWER::PEER_LIST;
	config_list_answer_slice.peer = &peerDB;
	config_list_answer_slice.max_slc = peerDB.get_nr_slices();
	config_list_answer_slice.active = 1;
	//dbg << "peerlist req\n";
}

void cmMaster::CONFIG_PARAM_REQ(s_m01xx04 *buf) {

}

void cmMaster::CONFIG_START(s_m01xx05 *buf) {

}

void cmMaster::CONFIG_END(s_m01xx06 *buf) {

}

void cmMaster::CONFIG_WRITE_INDEX1(s_m01xx07 *buf) {

}

void cmMaster::CONFIG_WRITE_INDEX2(s_m01xx08 *buf) {

}

void cmMaster::CONFIG_SERIAL_REQ(s_m01xx09 *buf) {

}

void cmMaster::CONFIG_PAIR_SERIAL(s_m01xx0a *buf) {

}

void cmMaster::CONFIG_STATUS_REQUEST(s_m01xx0e *buf) {

}



//- send functions --------------------------------------------------------------------------------------------------------
/**
* @brief Send device info
*
* Message description:
*             Sender__ Receiver fwVer type   Serial number                     class  pCnlA  pCnlB  unknown
* 1A 94 84 00 1F B7 4A 01 02 04 15    00 6C  4B 45 51 30 32 33 37 33 39 36  10 41     01     00
*/
void send_DEVICE_INFO(uint8_t *RCV_ID) {
	s_m00xxxx *msg = (s_m00xxxx*)snd_msg.buf;												// define a device info struct while easier to fill
	uint8_t *rcv_id;

	/* copy the payload from different sources */
	memcpy_P(&msg->FIRMWARE, dev_static, 3);													// copy firmware and modelID
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);											// copy the serial number
	memcpy_P(&msg->CLASS, dev_static + 3, 4);												// copy subtype and device info
																							
	/* is it an answer to a CONFIG_PAIR_SERIAL request, or while we initiate the pairing process */
	if ( (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CONFIG_PAIR_SERIAL) ) && (rcv_msg.mBody.BY11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL) ) ) {
		msg->MSG_CNT = rcv_msg.mBody.MSG_CNT;
		rcv_id = rcv_msg.mBody.SND_ID;
	} else {
		msg->MSG_CNT = snd_msg.MSG_CNT++;
		rcv_id = dev_ident.HMID;
	}

	snd_msg.set_msg(MSG_TYPE::DEVICE_INFO, rcv_id, 1);
	//prepareToSend(msgCount, AS_MESSAGE_DEVINFO, MAID);

	//pair_mode.active = 1;																		// set pairing flag
	//pair_mode.timer.set(20000);															// set pairing time
	led.set(pairing);																			// and visualize the status
}

/**
* @brief Check if ACK required and send ACK or NACK
*/
void check_send_ACK_NACK(uint8_t *RCV_ID, uint8_t ackOk) {
	if (ackOk) send_ACK(RCV_ID);																	// we are here to proof the result
	else send_NACK(RCV_ID);																		// and send an ACK or NACK
}

/**
* @brief Send ACK message
*
* Message description:
*             Sender__ Receiver ACK
* 0A 24 80 02 1F B7 4A 63 19 63 00
*/
void send_ACK(uint8_t *RCV_ID) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ack only if required
	snd_msg.set_msg(MSG_TYPE::ACK, rcv_msg.mBody.SND_ID);									// length and flags are set within the snd_msg struct
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
void send_ACK_STATUS(uint8_t *RCV_ID, uint8_t chnl, uint8_t stat, uint8_t actn) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	s_m0201xx *msg = (s_m0201xx*)snd_msg.buf;												// define an ACK Status struct while easier to fill
	msg->MSG_CNL = chnl;
	msg->MSG_STATUS = stat;
	*(uint8_t*)&msg->MSG_FLAG = actn;
	msg->MSG_FLAG.LOWBAT = bat.getStatus();
	msg->MSG_RSSI = cc.rssi;
	snd_msg.set_msg(MSG_TYPE::ACK_STATUS, rcv_msg.mBody.SND_ID);
}

void send_AES_REQ(uint8_t *RCV_ID) {
}

/**
* @brief Send a NACK (not ACK)
*
* Message description:
*             Sender__ Receiver NACK
* 0A 24 80 02 1F B7 4A 63 19 63 80
*/
void send_NACK(uint8_t *RCV_ID) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ack only if required
	snd_msg.set_msg(MSG_TYPE::NACK, rcv_msg.mBody.SND_ID);									// length and flags are set within the snd_msg struct
}

/**
* @brief Send a NACK (not ACK and target invalid)
*
* Message description:
*             Sender__ Receiver NACK_TAGRET_INVALID
* 0A 24 80 02 1F B7 4A 63 19 63 84
*/
void send_NACK_TARGET_INVALID(uint8_t *RCV_ID) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	snd_msg.set_msg(MSG_TYPE::NACK_TARGET_INVALID, rcv_msg.mBody.SND_ID);
}

void send_ACK_AES(uint8_t *RCV_ID) {
}
void send_ACK_NACK_UNKNOWN(uint8_t *RCV_ID) {
}
void send_AES_REPLY(uint8_t *RCV_ID) {
}

void send_SEND_AES_TO_HMLAN(uint8_t *RCV_ID) {
}
void send_SEND_AES_TO_ACTOR(uint8_t *RCV_ID) {
}

void send_INFO_SERIAL(uint8_t *RCV_ID) {
}
void send_INFO_PEER_LIST(uint8_t *RCV_ID, uint8_t payload_len) {
	snd_msg.mBody.MSG_CNT = snd_msg.MSG_CNT++;
	snd_msg.set_msg(MSG_TYPE::INFO_PEER_LIST, RCV_ID, 1, payload_len+11);
}
void send_INFO_PARAM_RESPONSE_PAIRS(void) {
}
void send_INFO_PARAM_RESPONSE_SEQ(void) {
}
void send_INFO_PARAMETER_CHANGE(void) {
}
void send_INFO_ACTUATOR_STATUS(void) {
}
void send_INFO_TEMP(void) {
}
void send_HAVE_DATA(void) {
}
void send_SWITCH(void) {
}
void send_TIMESTAMP(void) {
}
void send_REMOTE(void) {
}
void send_SENSOR_EVENT(void) {
}
void send_SWITCH_LEVEL(void) {
}
void send_SENSOR_DATA(void) {
}
void send_GAS_EVENT(void) {
}
void send_CLIMATE_EVENT(void) {
}
void send_SET_TEAM_TEMP(void) {
}
void send_THERMAL_CONTROL(void) {
}
void send_POWER_EVENT_CYCLE(void) {
}
void send_POWER_EVENT(void) {
}
void send_WEATHER_EVENT(void) {
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