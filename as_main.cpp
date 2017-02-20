/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin framework main class -------------------------------------------------------------------------------------------
* - with a lot of support from many people at FHEM forum
*   thanks a lot to martin876, dirk, pa-pa, martin, Dietmar63 and all i have not personal named here 
*   special thank you to https://git.zerfleddert.de/hmcfgusb/AES/ for bidcos(R) AES explanation
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"
#include "as_analyze.h"


// public:		//---------------------------------------------------------------------------------------------------------
/**
 * @brief Initialize the AskSin Module
 */
void AS::init(void) {

	/* - init eeprom function if a i2c eeprom is used and 
	* prepare the defaults incl eeprom address map for the channel modules */
	init_eeprom();		
	uint16_t pAddr = cm_prep_default(sizeof(dev_ident));

	/* - First time start check is done via comparing a magic number at the start of the eeprom
	* with the CRC of the different lists in the channel modules. Every time there was a
	* change in the configuration some addresses are changed and we have to rewrite the eeprom content.	*/
	uint16_t flashCRC = cm_calc_crc();														// calculate the crc of all channel module list0/1, list3/4
	get_eeprom(0, sizeof(dev_ident), &dev_ident);											// get magic byte and all other information from eeprom
	DBG(AS, F("AS:init crc- flash:"), flashCRC, F(", eeprom: "), dev_ident.MAGIC, '\n');	// some debug

	if (flashCRC != dev_ident.MAGIC) {	

		/* - Write the defaults into the respective lists in eeprom and clear the peer database.
		* defaults are read from channel modules PROGMEM section, copied into the value byte array
		* and written to the eeprom.  */
		for (uint8_t i = 0; i < cnl_max; i++) {												// write the defaults in respective list0/1
			cmm[i]->lstC.load_default();													// copy from progmem into array
			cmm[i]->lstC.save_list();														// write it into the eeprom
			cmm[i]->peerDB.clear_all();
			DBG(AS, F("AS:write_defaults, cnl:"), cmm[i]->lstC.cnl, F(", lst:"), cmm[i]->lstC.lst, F(", len:"), cmm[i]->lstC.len, '\n');
		}

		/* - First time detected
		* get the homematic id, serial number, aes key index and homematic aes key from flash and copy it into the eeprom
		* order in HMSerialData[]                 * HMID *, * Serial number *, * Default-Key *, * Key-Index *
		* order in dev_ident struct   *	MAGIC *, * HMID[3] *, * SERIAL_NR[10] *, * HMKEY[16] *, * HMKEY_INDEX *
		* we can copy the complete struct with a 2 byte offset in regards to the magic byte */
		dev_ident.MAGIC = flashCRC;															// set new magic number
		memcpy_P(((uint8_t*)&dev_ident) + 2, HMSerialData, sizeof(dev_ident) - 2);			// copy from PROGMEM
		set_eeprom(0, sizeof(dev_ident), ((uint8_t*)&dev_ident));
		while (!eeprom_is_ready());
		DBG(AS, F("AS:writing new magic byte\n"));											// some debug

		/* - function to be placed in register.h, to setup default values on first time start */
		firstTimeStart();				
	}


	/* load list 0 and 1 defaults and inform the channel modules */
	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		cmm[i]->lstC.load_list();															// read the defaults in respective list0/1
		inform_config_change(i);															// inform the channel modules
		cmm[i]->init();																		// initialize the channel modules
	}

	/* - add this function in register.h to setup default values every start */
	everyTimeStart();

	/* - Initialize the hardware. All this functions are defined in HAL.h and HAL_extern.h 	*/
	com->init();																			// init the rf module
	cbn->init();																			// init the config button
	led->init();																			// initialize the leds
	led->set(LED_STAT::WELCOME);															// show something as status

}

/**
 * @brief Cyclic poll all related functions
 */
void AS::poll(void) {

	/* copy the decoded data into the receiver module if something was received
	*  and poll the received buffer, it checks if something is in the queue  */
	if (com->has_data()) {																	// check if something is in the cc1101 receive buffer
		com->rcv_data(rcv_msg.buf);															// if yes, get it into our receive processing struct
		rcv_poll();																			// and poll the receive function to get intent and some basics
	}
	if (rcv_msg.buf[0]) process_message();													// check if we have to handle the receive buffer

	/* handle the send module */
	snd_poll();																				// check if there is something to send

	/* peer and register list messages are processed here */
	process_list_message_poll();															// check if something has to be send slice wise

	/* peer message poll, typically needed from channels with a list4 */
	process_peer_message_poll();															// if we want to send a peer message we have to poll the peer send processing



	/* time out the config flag */
	if (config_mode.active) {																// check only if we are still in config mode
		if (config_mode.timer.done()) config_mode.active = 0;								// when timer is done, set config flag to inactive
	}

	/* regular polls for the channel modules */
	for (uint8_t i = 0; i < cnl_max; i++) {													// poll all the registered channel modules
		cmm[i]->poll();
	}

	/* check if the device needs a reset */
	if (dev_operate.reset) {
		if ((dev_operate.reset == 2) && (!snd_msg.active)) {								// check reset status, but wait till send is done
			clear_eeprom(0, 2);																// clear the magic byte in eeprom
			dev_operate.reset = 0;															// clear the reset flag
			init();																			// call init function to set the defaults
		}
	}


	// time out the pairing timer
	if (pair_mode.active) { 
		if (pair_mode.timer.done()) {
			pair_mode.active = 0;
			isEmpty(dev_operate.MAID, 3)? led->set(LED_STAT::PAIR_ERROR) : led->set(LED_STAT::PAIR_SUCCESS);
		}
	}


	cbn->poll();																			// poll the config button
	led->poll();																			// poll the led's
	bat->poll();																			// poll the battery check
	pom->poll();																			// poll the power management
}


/* ------------------------------------------------------------------------------------------------------------------------
* - receive functions -----------------------------------------------------------------------------------------------------
* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
* The intent is to overload them there by the respective user channel module and work with the information accordingly.
*/
void AS::rcv_poll(void) {
	/* checks a received string for validity and intent */
	if (rcv_msg.mBody.MSG_LEN < 9) goto clear_rcv_poll;										// check if the string has all mandatory bytes, if not

	/* check for a repeated string which was already processed, after this the repeated flag doesn't matter anymore */
	if ((rcv_msg.mBody.FLAG.RPTED) && (rcv_msg.prev_MSG_CNT == rcv_msg.mBody.MSG_CNT)) {
		DBG(RV, F("  repeated...\n"));
		goto clear_rcv_poll;
	}
	rcv_msg.prev_MSG_CNT = rcv_msg.mBody.MSG_CNT;											// remember for next time
	rcv_msg.mBody.FLAG.RPTED = 0;															// clear the repeated flag

	/* check the addresses in the message */
	get_intend();

	explain_msg();
	//DBG(RV, (char)rcv_msg.intend, F("> "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), ' ', _TIME, '\n');

	/* only process messages for us from master or peer */
	if ((rcv_msg.intend == MSG_INTENT::MASTER) || (rcv_msg.intend == MSG_INTENT::PEER) || (rcv_msg.intend == MSG_INTENT::NOT_PAIRED)) return;

clear_rcv_poll:
	rcv_msg.clear();																		// nothing to do any more
}

/*
* @brief get the intend of the message
* This function is part of the rcv_poll function and searches based on the sender and receiver address for the intend of the 
* given message. It is important to know, because we work only on messages which are addressed to us and sent by a pair or peer.
*/
void AS::get_intend() {
	uint8_t *rcv_by03 = &rcv_msg.mBody.MSG_TYP;												// some shorthands
	uint8_t *rcv_by10 = &rcv_msg.mBody.BY10;
	uint8_t *rcv_by11 = &rcv_msg.mBody.BY11;

	/* prepare the peer address for the search */
	memcpy(rcv_msg.peer, rcv_msg.mBody.SND_ID, 3);											// peer has 4 byte and the last byte indicates the channel but also lowbat and long message, therefore we copy it together in a seperate byte array
	rcv_msg.peer[3] = *rcv_by10;															// get the peer channel from the received string
	rcv_msg.peer[3] &= 0x3f;																// mask out long and battery low

	if (isEmpty(rcv_msg.mBody.RCV_ID, 3)) {
		/* it could come as a broadcast message - report it only while loging is enabled */
		rcv_msg.intend = MSG_INTENT::BROADCAST;
		/* broadcast messages not used, with one exception - serial pair request */
		if ((*rcv_by03 == BY03(MSG_TYPE::CONFIG_REQ)) && (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL))) {
			rcv_msg.intend = MSG_INTENT::MASTER;
		}

	} else if (!isEqual(rcv_msg.mBody.RCV_ID, dev_ident.HMID, 3)) {
		/* it could be addressed to a different device - report it only while loging is enabled
		*  memcmp gives 0 if string matches, any other value while no match */
		rcv_msg.intend = MSG_INTENT::LOGGING;

	} else if (isEqual(rcv_msg.mBody.SND_ID, dev_operate.MAID, 3)) {
		/* because of the previous check, message is for us, check against master */
		rcv_msg.intend = MSG_INTENT::MASTER;

	} else if (rcv_msg.cnl = is_peer_valid(rcv_msg.peer)) {
		/* message is for us, but not from master, maybe it is a peer message */
		rcv_msg.intend = MSG_INTENT::PEER;

	} else if (isEqual(rcv_msg.mBody.SND_ID, dev_ident.HMID, 3)) {
		/* message is for us, but not from pair or peer, check if we were the sender and flag it as internal */
		rcv_msg.intend = MSG_INTENT::INTERN;

	} else if (isEmpty(dev_operate.MAID, 3)) {
		/* message is for us, but not from pair or peer or internal - check if we miss the master id because we are not paired */
		rcv_msg.intend = MSG_INTENT::NOT_PAIRED;

	} else {
		rcv_msg.intend = MSG_INTENT::ERROR;												// should never happens
		/* an AES_REQ could come from a pair or peer, the peer address does not include the channel information -
		*  therefor we check the send string and if the address is equal, we process as peer message */
		if ((*rcv_by03 == BY03(MSG_TYPE::AES_REQ)) && (*rcv_by10 == BY10(MSG_TYPE::AES_REQ)) && (isEqual(rcv_msg.mBody.SND_ID, snd_msg.mBody.RCV_ID, 3))) {
			rcv_msg.intend = MSG_INTENT::PEER;
		} 
	}
}

/**
* @brief Receive handler: Process received messages
* Process message is called while rcv_msg.hasdata is set to TRUE. It takes all information from the rcv_msg struct.
* Sort out by several if's which message we received and delegates the reuqest for answer to the according class/function.
* 
*/
void AS::process_message(void) {
	uint8_t *rcv_by03 = &rcv_msg.mBody.MSG_TYP;
	uint8_t *rcv_by10 = &rcv_msg.mBody.BY10;
	uint8_t *rcv_by11 = &rcv_msg.mBody.BY11;
	CM_MASTER *pCM;
	//dbg << "by03:" << *rcv_by03 << ", by10:" << *rcv_by10 << ", by11:" << *rcv_by11 << '\n';

	if (*rcv_by03 == BY03(MSG_TYPE::DEVICE_INFO)) {
		/* not sure what to do with while received, probably nothing */

	} else if (*rcv_by03 == BY03(MSG_TYPE::CONFIG_REQ)) {
		/* config request messages are used to configure a devive by writing registers and peers -
		*  check the channel and forward for processing to the respective function */
		if (rcv_msg.mBody.BY10 >= cnl_max) return;											// channel is out of range, return
		//uint8_t by11 = rcv_msg.mBody.BY11;												// short hand to byte 11 in the received string
		pCM = cmm[*rcv_by10];																// short hand to the respective channel module instance

		/* check if we need to challange the request */
		switch (*rcv_by11) {
			case BY11(MSG_TYPE::CONFIG_PEER_ADD):
			case BY11(MSG_TYPE::CONFIG_PEER_REMOVE):
			case BY11(MSG_TYPE::CONFIG_START):
			case BY11(MSG_TYPE::CONFIG_END):
			case BY11(MSG_TYPE::CONFIG_WRITE_INDEX1):
			case BY11(MSG_TYPE::CONFIG_WRITE_INDEX2):
				uint8_t *AES = pCM->lstC.ptr_to_val(0x08);
				if (!AES) AES = new uint8_t[1];
				if ((*AES) && (aes->active != MSG_AES::AES_REPLY_OK)) {						// check if we need AES confirmation
					send_AES_REQ();															// send a request
					return;																	// nothing to do any more, wait and see
				}
		}

		/* challange done, now we can process the initial request */
		if      (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PEER_ADD))       pCM->CONFIG_PEER_ADD(&rcv_msg.m01xx01);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PEER_REMOVE))    pCM->CONFIG_PEER_REMOVE(&rcv_msg.m01xx02);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PEER_LIST_REQ))  pCM->CONFIG_PEER_LIST_REQ(&rcv_msg.m01xx03);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PARAM_REQ))      pCM->CONFIG_PARAM_REQ(&rcv_msg.m01xx04);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_START))          pCM->CONFIG_START(&rcv_msg.m01xx05);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_END))            pCM->CONFIG_END(&rcv_msg.m01xx06);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX1))   pCM->CONFIG_WRITE_INDEX1(&rcv_msg.m01xx07);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX2))   pCM->CONFIG_WRITE_INDEX2(&rcv_msg.m01xx08);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_SERIAL_REQ))     pCM->CONFIG_SERIAL_REQ(&rcv_msg.m01xx09);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL))    pCM->CONFIG_PAIR_SERIAL(&rcv_msg.m01xx0a);
		else if (*rcv_by11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST)) pCM->CONFIG_STATUS_REQUEST(&rcv_msg.m01xx0e);


	} else if (*rcv_by03 == BY03(MSG_TYPE::ACK_MSG)) {
		if (*rcv_by10 == BY10(MSG_TYPE::AES_REQ)) {
		/* AES request is a speciality in this section, pair or peer is sending this request to challenge the last command we had send,
		*  we have to use the 6 byte payload and generate a SEND_AES type message (* 0x02 04 ff 11 * - AES_REQ) */
			//dbg << "AES_REQ, ind: " << _HEX(rcv_msg.buf[17]) << ", data: " << _HEX(rcv_msg.buf+11, 6) << '\n';
			aes->prep_AES_REPLY(dev_ident.HMKEY, dev_ident.HMKEY_INDEX, rcv_msg.buf + 11, snd_msg.buf);// prepare the reply
			snd_msg.clear();																// clear send message
			send_AES_REPLY(aes->prev_buf);													// and send it

		} else if (rcv_msg.mBody.MSG_CNT == snd_msg.mBody.MSG_CNT) {
		/* at the moment we need the ACK message only for avoiding resends, so let the send_msg struct know about
		*  a received ACK/NACK whatever - probably we have to change this function in the future */
			snd_msg.retr_cnt = 0xff;														// check if the message counter is similar and let the send function know
		}

	} else if (*rcv_by03 == BY03(MSG_TYPE::AES_REPLY)) {
		/* we received an AES_REPLY, first we tell the send function that we received an answer. as the receive flag is not cleared, we will come back again */
		if (snd_msg.active) {
			snd_msg.retr_cnt = 0xff;														// we received an answer to our request, no need to resend
			return;
		}
		aes->check_AES_REPLY(dev_ident.HMKEY, rcv_msg.buf);									// check the data, if ok, the last message will be restored, otherwise the hasdata flag will be 0
		return;																				// next round to work on the restored message


	} else if (*rcv_by03 == BY03(MSG_TYPE::SEND_AES)) {
		/* pair starts an key exchange, first message shows our key starting with byte 12 and the keyindex in byte 11 (-2) 
		*  second message holds the new key starting with byte 12 and the new keyindex in byte 11 (again -2) */

		/* challange the message */
		if ((*pCM->lstC.ptr_to_val(0x08)) && (aes->active != MSG_AES::AES_REPLY_OK)) {		// check if we need AES confirmation
			send_AES_REQ();																	// send a request
			return;																			// nothing to do any more, wait and see
		}

		/* check the message in the aes_key struct, returns are 0 for doesnt fit, 1 key exchange started, 2 new key received */
		uint8_t new_key = aes->check_SEND_AES_TO_ACTOR(dev_ident.HMKEY, dev_ident.HMKEY_INDEX, rcv_msg.buf);
		if (new_key) {
			dbg << "new idx " << aes->new_hmkey_index[0] << ", new key " << _HEX(aes->new_hmkey, 16) << '\n';
			memcpy(dev_ident.HMKEY, aes->new_hmkey, 16);									// store the new key
			dev_ident.HMKEY_INDEX[0] = aes->new_hmkey_index[0];
			set_eeprom(0, sizeof(dev_ident), ((uint8_t*)&dev_ident));						// write it to the eeprom
		}
		send_ACK();																			// send ACK


	} else if (*rcv_by03 == BY03(MSG_TYPE::REPLY_MSG)) {


	} else if (*rcv_by03 == BY03(MSG_TYPE::INSTRUCTION_MSG)) {
		//uint8_t by10 = rcv_msg.mBody.BY10;												// short hand to byte 10 in the received string
		uint8_t mlen = rcv_msg.mBody.MSG_LEN;												// short hand to the message length

		if (mlen == 0x0a) pCM = cmm[0];													// some messages are channel independent, identification by length
		else pCM = cmm[*rcv_by11];															// short hand to respective channel module instance

		/* check if we need to challange the request */
		if ((*pCM->lstC.ptr_to_val(0x08)) && (aes->active != MSG_AES::AES_REPLY_OK)) {		// check if we need AES confirmation
			send_AES_REQ();																	// send a request
			return;																			// nothing to do any more, wait and see
		}

		/* check if channel related, there are four messages in this category without a channel value, this will be handled here 
		*  INSTRUCTION_RESET, INSTRUCTION_ENTER_BOOTLOADER, INSTRUCTION_ENTER_BOOTLOADER2, INSTRUCTION_ADAPTION_DRIVE_SET */
		if      (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_RESET))              INSTRUCTION_RESET(&rcv_msg.m1104xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER))   INSTRUCTION_ENTER_BOOTLOADER(&rcv_msg.m1183xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_ADAPTION_DRIVE_SET)) INSTRUCTION_ADAPTION_DRIVE_SET(&rcv_msg.m1187xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER2))  INSTRUCTION_ENTER_BOOTLOADER2(&rcv_msg.m11caxx);

		/* everything below is channel related */
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_OFF))        pCM->INSTRUCTION_INHIBIT_OFF(&rcv_msg.m1100xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_ON))         pCM->INSTRUCTION_INHIBIT_ON(&rcv_msg.m1101xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_SET))                pCM->INSTRUCTION_SET(&rcv_msg.m1102xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_STOP_CHANGE))        pCM->INSTRUCTION_STOP_CHANGE(&rcv_msg.m1103xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_LED))                pCM->INSTRUCTION_LED(&rcv_msg.m1180xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_LED_ALL))            pCM->INSTRUCTION_LED_ALL(&rcv_msg.m1181xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_LEVEL))              pCM->INSTRUCTION_LEVEL(&rcv_msg.m1181xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_SLEEPMODE))          pCM->INSTRUCTION_SLEEPMODE(&rcv_msg.m1182xx);
		else if (*rcv_by10 == BY10(MSG_TYPE::INSTRUCTION_SET_TEMP))           pCM->INSTRUCTION_SET_TEMP(&rcv_msg.m1186xx);


	} else if (*rcv_by03 == BY03(MSG_TYPE::HAVE_DATA)) {


	} else if (*rcv_by03 == BY03(MSG_TYPE::SWITCH)) {
	 /* to process this message we need to load the right list table for the respective peer index into memory
	 * need also to find the right channel to issue the message, depending on the peer address */
		memcpy(rcv_msg.peer, rcv_msg.m3Exxxx.PEER, 3);										// prepare a peer string
		rcv_msg.peer[3] = rcv_msg.m3Exxxx.P_CNL;
		rcv_msg.cnl = is_peer_valid(rcv_msg.peer);											// search for the peer channel
		if (!rcv_msg.cnl) return;															// peer not found in any channel, return

		//pCM = &cmm[rcv_msg.cnl];															// short hand to the respective channel module
		/* check if we need to challange the request */
		if ((*cmm[rcv_msg.cnl]->lstC.ptr_to_val(0x08)) && (aes->active != MSG_AES::AES_REPLY_OK)) { // check if we need AES confirmation
			send_AES_REQ();																	// send a request
			return;																			// nothing to do any more, wait and see
		}
		cmm[rcv_msg.cnl]->lstP.load_list(pCM->peerDB.get_idx(rcv_msg.peer));				// load the respective list 3 with the respective index 
		cmm[rcv_msg.cnl]->SWITCH(&rcv_msg.m3Exxxx);


	} else if (rcv_msg.intend == MSG_INTENT::PEER) {
	 /* it is a peer message, which was checked in the receive class, so reload the respective list 3/4 */
		pCM = cmm[rcv_msg.cnl];																// we remembered on the channel by checking validity of peer
		/* check if we need to challange the request */
		if ((*cmm[rcv_msg.cnl]->lstC.ptr_to_val(0x08)) && (aes->active != MSG_AES::AES_REPLY_OK)) {		// check if we need AES confirmation
			send_AES_REQ();																	// send a request
			return;																			// nothing to do any more, wait and see
		}
		/* forward to the respective channel function */
		pCM->lstP.load_list(cmm[rcv_msg.cnl]->peerDB.get_idx(rcv_msg.peer));				// load the respective list 3
		if      (*rcv_by03 == BY03(MSG_TYPE::TIMESTAMP))         pCM->TIMESTAMP(&rcv_msg.m3fxxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::REMOTE))            pCM->REMOTE(&rcv_msg.m40xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::SENSOR_EVENT))      pCM->SENSOR_EVENT(&rcv_msg.m41xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::SWITCH_LEVEL))      pCM->SWITCH_LEVEL(&rcv_msg.m42xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::SENSOR_DATA))       pCM->SENSOR_DATA(&rcv_msg.m53xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::GAS_EVENT))         pCM->GAS_EVENT(&rcv_msg.m54xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::CLIMATE_EVENT))     pCM->CLIMATE_EVENT(&rcv_msg.m58xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::SET_TEAM_TEMP))     pCM->SET_TEAM_TEMP(&rcv_msg.m59xxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::THERMAL_CONTROL))   pCM->THERMAL_CONTROL(&rcv_msg.m5axxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::POWER_EVENT_CYCLE)) pCM->POWER_EVENT_CYCLE(&rcv_msg.m5exxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::POWER_EVENT))       pCM->POWER_EVENT(&rcv_msg.m5fxxxx);
		else if (*rcv_by03 == BY03(MSG_TYPE::WEATHER_EVENT))     pCM->WEATHER_EVENT(&rcv_msg.m70xxxx);


	} else {
		dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';
		DBG(AS, F("AS:message not known - please report: "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
	}

	rcv_msg.clear();
	return;

}






/* ------------------------------------------------------------------------------------------------------------------------
* - send functions --------------------------------------------------------------------------------------------------------
* @brief Here are the send functions for the device, this library is focused on client communication,
* so you will find send functions for client communication only. The send functions are sorted by
* message type and sub type. Only for the peer related messages there is the need to define the receiver.
* Configuration and status answers send only to HMID, ACK and subtypes are always the response to a received string
*/
void AS::snd_poll(void) {
	s_snd_msg *sm = &snd_msg;																// short hand to snd_msg struct

	if (sm->active == MSG_ACTIVE::NONE) return;												// nothing to do

	/* can only happen while an ack was received and AS:processMessage had send the retr_cnt to 0xff */
	if (sm->retr_cnt == 0xff) {
		sm->clear();																		// nothing to do any more
		led->set(LED_STAT::GOT_ACK);														// fire the status led
		pom->stayAwake(100);																// and stay awake for a short while
		return;
	}

	/*  return while no ACK received and timer is running */
	if (!sm->timer.done()) return;


	/* check for first time and prepare the send */
	if ((!sm->retr_cnt) && (sm->active != MSG_ACTIVE::DEBUG)) {

		/* copy snd_id and message flag */
		memcpy(sm->mBody.SND_ID, dev_ident.HMID, 3);										// we always send the message in our name
		sm->mBody.MSG_TYP = BY03(sm->type);													// msg type
		if (BY10(sm->type) != 0xff) sm->mBody.BY10 = BY10(sm->type);						// byte 10
		if (BY11(sm->type) != 0xff) sm->mBody.BY11 = BY11(sm->type);						// byte 11
		if (MLEN(sm->type) != 0xff) sm->mBody.MSG_LEN = MLEN(sm->type);						// msg len


		/* work on the message flag, config, bidi, wakemeup */
		sm->mBody.FLAG.RPTEN = 1;															// every message need this flag

		if (sm->active & 0x01) sm->mBody.FLAG.BIDI = 1;										// check if bidi flag is set
		else sm->mBody.FLAG.BIDI = 0;

		if (snd_msg.type == MSG_TYPE::DEVICE_INFO) sm->mBody.FLAG.CFG = 1;					// DEVICE_INFO will be send, so we signalize that we are in config mode
		else sm->mBody.FLAG.CFG = 0;														// default is not in config mode


		/* based on the given flag we select the address we want the message to be sent to */
		if        (sm->active         == MSG_ACTIVE::INTERN) {
		/* not sure if this case will happen */

		} else if ((sm->active & 0xFE) == MSG_ACTIVE::ANSWER) {								// ANSWER_BIDI
		/* it is an answer to the received message */
			memcpy(sm->mBody.RCV_ID, rcv_msg.mBody.SND_ID, 3);								// use the address from the received message
			sm->mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;										// use the message counter from the received message

		} else if (sm->active         == MSG_ACTIVE::PAIR) {
		/* pair means - msg_cnt from snd_msg struct, rcv_id is master_id, bidi not needed */
			memcpy(sm->mBody.RCV_ID, dev_operate.MAID, 3);									// use the address from the received message
			sm->mBody.MSG_CNT = sm->MSG_CNT;												// use own message counter
			sm->MSG_CNT++;																	// increase counter for next try

		} else if (sm->active == MSG_ACTIVE::BROADCAST) {
			/* broadcast means - msg_cnt from snd_msg struct, rcv_id is 00 00 00, bidi not needed */
			memset(sm->mBody.RCV_ID, 0, 3);													// clear the receive address
			sm->mBody.MSG_CNT = sm->MSG_CNT;												// use own message counter
			sm->MSG_CNT++;																	// increase counter for next try

		} else if ((sm->active & 0xFE) == MSG_ACTIVE::PEER) {								// PEER_BIDI
		/* peer messages are handled in a seperate loop */
		}

		/* an internal message which is only to forward while already prepared,
		* other options are internal but need to be prepared, external message are differs to whom they have to be send and if they
		* are initial send or as an answer to a received message. all necassary information are in the send struct */
		if (isEqual(sm->mBody.RCV_ID, dev_ident.HMID, 3)) {
			memcpy(rcv_msg.buf, sm->buf, sm->buf[0] + 1);									// copy send buffer to received buffer
			DBG(SN, F("<i ...\n"));															// some debug, message is shown in the received string
			rcv_poll();																		// get intent and so on...
			sm->clear();																	// nothing to do any more for send, msg will processed in the receive loop
			return;																			// and return...
		}

		/* internal messages doesn't matter anymore*/
		sm->temp_MSG_CNT = sm->mBody.MSG_CNT;												// copy the message count to identify the ACK
		if (isEmpty(sm->mBody.RCV_ID, 3)) sm->mBody.FLAG.BIDI = 0;							// broadcast, no ack required

		if (!sm->temp_max_retr)
			sm->temp_max_retr = (sm->mBody.FLAG.BIDI) ? sm->max_retr : 1;					// send once while not requesting an ACK

		sm->mBody.FLAG.CFG = 1;
	}


	/* check the retr count if there is something to send, while message timer was checked earlier */
	if (sm->retr_cnt < sm->temp_max_retr) {													// not all sends done and timing is OK
		uint8_t tBurst = sm->mBody.FLAG.BURST;												// get burst flag, while string will get encoded
		com->snd_data(sm->buf, tBurst);														// send to communication module
		sm->retr_cnt++;																		// remember that we had send the message

		if (sm->mBody.FLAG.BIDI) sm->timer.set(sm->max_time);								// timeout is only needed while an ACK is requested
		led->set(LED_STAT::SEND_MSG);														// fire the status led
		pom->stayAwake(100);																// and stay awake for a short while

		DBG(SN, F("<- "), _HEX(sm->buf, sm->buf[0] + 1), ' ', _TIME, '\n');					// some debug

	} else {
	/* if we are here, message was send one or multiple times and the timeout was raised if an ack where required */
	/* seems, nobody had got our message, other wise we had received an ACK */
		sm->clear();																		// clear the struct, while nothing to do any more

		if (!sm->mBody.FLAG.BIDI) return;													// everything fine, ACK was not required

		sm->timeout = 1;																	// set the time out only while an ACK or answer was requested
		led->set(LED_STAT::GOT_NACK);														// fire the status led
		pom->stayAwake(100);																// and stay awake for a short while

		DBG(SN, F("  timed out "), _TIME, '\n');											// some debug
	}
}

void AS::process_list_message_poll(void) {
	s_snd_msg  *sm = &snd_msg;																// short hand to snd_msg struct
	s_list_msg *lm = &list_msg;																// short hand

	if (!lm->active) return;																// nothing to send, return
	if (!lm->timer.done()) return;															// something to send but we have to wait
	if (sm->active) return;																	// send is busy, wait....

	uint8_t payload_len;

	if (lm->active == LIST_ANSWER::PEER_LIST) {
		/* process the INFO_PEER_LIST */
		payload_len = lm->peer->get_slice(lm->cur_slc, sm->buf + 11);						// get the slice and the amount of bytes
		sm->type = MSG_TYPE::INFO_PEER_LIST;												// flags are set within the snd_msg struct
		//DBG(SN, F("SN:LIST_ANSWER::PEER_LIST cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		lm->cur_slc++;																		// increase slice counter

	} else if (lm->active == LIST_ANSWER::PARAM_RESPONSE_PAIRS) {
		/* process the INFO_PARAM_RESPONSE_PAIRS */
		payload_len = lm->list->get_slice_pairs(lm->peer_idx, lm->cur_slc, sm->buf + 11);	// get the slice and the amount of bytes
		if (payload_len == 2) sm->type = MSG_TYPE::INFO_PARAM_RESPONSE_SEQ;					// if it is a message with only terminating 00 00 then it is a INFO_PARAM_RESPONSE_SEQ
		else sm->type = MSG_TYPE::INFO_PARAM_RESPONSE_PAIRS;								// otherwise we send a INFO_PARAM_RESPONSE_PAIRS
		//DBG(SN, F("SN:LIST_ANSWER::PARAM_RESPONSE_PAIRS cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		lm->cur_slc++;																		// increase slice counter

	} else if (lm->active == LIST_ANSWER::PARAM_RESPONSE_SEQ) {
		/* process the INFO_PARAM_RESPONSE_SEQ
		* not implemented yet */
	}

	sm->mBody.MSG_LEN = payload_len + 10;													// set the message len accordingly
	sm->active = MSG_ACTIVE::ANSWER_BIDI;													// for address, counter and to make it active

	if (lm->cur_slc >= lm->max_slc) {														// if everything is send, we could stop the struct
		//DBG(SN, F("SN:LIST_ANSWER::DONE cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		lm->active = LIST_ANSWER::NONE;
		lm->cur_slc = 0;
	}
}

/*
* @brief poll function to process peer messages.
* Respective information needs to be in peer_msg struct, while herein the string preparation for the
* send function is done. This function needs to be polled by at least on channel module which uses
* one peer send function from the list above.
*/
void AS::process_peer_message_poll(void) {
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
	sm->active = pm->active;																// set it active

	/* take care of the payload - peer message means in any case that the payload starts at the same position in the string and
	*  while it could have a different length, we calculate the length of the string by a hand over value */
	sm->mBody.MSG_LEN = pm->payload_len + 9;
	memcpy(&sm->buf[10], pm->payload_ptr, pm->payload_len);

	/* send it as pair message if we have no peer registered */
	if (!pm->peerDB->used_slots()) {														// if no peer is registered, we send the message to the pair
		memcpy(sm->mBody.RCV_ID, dev_operate.MAID, 3);										// copy in the pair address
		sm->mBody.MSG_CNT = sm->MSG_CNT;													// set the message counter
		sm->MSG_CNT++;																		// increase the counter for next time use
		pm->clear();																		// nothing to do here any more, while handled as pair message
		return;																				// and return, otherwise some infos are overwritten
	}

	sm->mBody.MSG_CNT = sm->MSG_CNT;														// set the message counter

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

	//hm.snd_poll();																		// call send poll function direct, otherwise someone could change the snd_msg content
}






/*
* @brief Send device info
* Could be an answer on a pair serial request, or initial send by the device
* Only difference is the message counter - on own initialized send, we use the message
* counter from the send class, on a pair serial request we take the counter value from the received string
*
* Message description:
*             Sender__ Receiver fwVer type   Serial number                     class  pCnlA  pCnlB  unknown
* 1A 94 84 00 1F B7 4A 01 02 04 15    00 6C  4B 45 51 30 32 33 37 33 39 36  10 41     01     00
*/
void AS::send_DEVICE_INFO(MSG_REASON::E reason) {
	s_m00xxxx *msg = &snd_msg.m00xxxx;														// short hand to send buffer

																							/* copy the payload from different sources */
	memcpy_P(&msg->FIRMWARE, dev_static, 3);												// copy firmware and modelID
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);										// copy the serial number
	memcpy_P(&msg->CLASS, dev_static + 3, 4);												// copy subtype and device info

																							/* is it an answer to a CONFIG_PAIR_SERIAL request, or while we initiate the pairing process */
	if (reason == MSG_REASON::ANSWER) {
		snd_msg.active = MSG_ACTIVE::ANSWER;												// for address, counter and to make it active
	}
	else {
		snd_msg.active = MSG_ACTIVE::PAIR;													// for address, counter and to make it active
	}

	/* BIDI is asked all time, will removed automatically if MAID is empty */
	snd_msg.type = MSG_TYPE::DEVICE_INFO;													// length and flags are set within the snd_msg struct

																							//pair_mode.active = 1;																	// set pairing flag
																							//pair_mode.timer.set(20000);															// set pairing time
																							//led->set(LED_STAT::PAIR_WAIT);															// and visualize the status
}
/**
* @brief Check if ACK required and send ACK or NACK
*/
void AS::check_send_ACK_NACK(uint8_t ackOk) {
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
void AS::send_ACK(void) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ack only if required

	if (aes->active == MSG_AES::AES_REPLY_OK) {											// if last message was a valid aes reply we have to answer with an ack_auth
		snd_msg.type = MSG_TYPE::ACK_AUTH;													// length and flags are set within the snd_msg struct
		memcpy(snd_msg.buf + 11, aes->ACK_payload, 4);									// 4 byte auth payload
	}
	else {
		snd_msg.type = MSG_TYPE::ACK;														// length and flags are set within the snd_msg struct
	}
	aes->active = MSG_AES::NONE;														// no need to remember on the last message
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
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
void AS::send_ACK_STATUS(uint8_t *chnl, uint8_t *stat, uint8_t *flag, uint8_t *sum) {
	//<frame id = "ACK_STATUS" direction = "from_device" event = "true" type = "0x02" subtype = "1" subtype_index = "9" channel_field = "10">
	//	<parameter type = "integer" index = "11.0" size = "1.0" param = "LEVEL" / >
	//	<parameter type = "integer" index = "12.1" size = "0.3" param = "ERROR" / >
	//	<parameter type = "integer" index = "12.1" size = "0.1" param = "ERROR_OVERLOAD" / >
	//	<parameter type = "integer" index = "12.2" size = "0.1" param = "ERROR_OVERHEAT" / >
	//	<parameter type = "integer" index = "12.3" size = "0.1" param = "ERROR_REDUCED" / >
	//	<parameter type = "integer" index = "12.4" size = "0.3" param = "STATE_FLAGS" / >     UP, DOWN, DELAY (could be battery)
	//	<parameter type = "integer" index = "12.4" size = "0.2" param = "DIRECTION_FLAGS" / > UP, DOWN
	//                                       12.7                                             Battery
	//	<parameter type = "integer" index = "14.0" size = "1.0" param = "LEVEL_REAL" / >
	//< / frame>
	// byte 13 is RSSI
	// 0F 8D 80 02   1E 7A AD    63 19 64   01 01 14 00 3A 14

	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	snd_msg.buf[11] = *chnl;																// add the channel
	snd_msg.buf[12] = *stat;																// and the status/value
	snd_msg.buf[13] = *flag;																// flags are prepared in the status poll function
	if (bat->get_status()) snd_msg.buf[13] |= 0x80;											// highest bit is battery flag
	else snd_msg.buf[13] &= 0x7F;
	snd_msg.buf[14] = com->rssi;															// add rssi information
	snd_msg.buf[15] = *sum;																	// we can add it to the buffer in any case, while length byte is set below

	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	if (sum) snd_msg.type = MSG_TYPE::ACK_STATUS_SUM;										// length and flags are set within the snd_msg struct
	else snd_msg.type = MSG_TYPE::ACK_STATUS;
	// todo: AES ACK
}
void AS::send_ACK2(void) {
}
void AS::send_AES_REQ() {
	/* save the initial message for later use and prepare the temp key */
	aes->prep_AES_REQ(dev_ident.HMKEY, rcv_msg.buf, snd_msg.buf);							// prepare the message, store received string and so on
	rcv_msg.buf[0] = 0;																		// and terminate the further processing

																							/* create the message */
	snd_msg.active = MSG_ACTIVE::ANSWER_BIDI;												// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::AES_REQ;														// length and flags are set within the snd_msg struct
	snd_msg.buf[17] = dev_ident.HMKEY_INDEX[0];												// the 7th byte is the key index
}
/**
* @brief Send a NACK (not ACK)
*
* Message description:
*             Sender__ Receiver NACK
* 0A 24 80 02 1F B7 4A 63 19 63 80
*/
void AS::send_NACK(void) {
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
void AS::send_NACK_TARGET_INVALID(void) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	snd_msg.active = MSG_ACTIVE::ANSWER;													// for address, counter and to make it active
	snd_msg.type = MSG_TYPE::NACK_TARGET_INVALID;											// length and flags are set within the snd_msg struct

}
void AS::send_ACK_NACK_UNKNOWN() {
}

void AS::send_AES_REPLY(uint8_t *payload) {
	memcpy(snd_msg.buf + 10, payload, 16);													// payload starts at byte 10 and has a length of 16 byte
	snd_msg.type = MSG_TYPE::AES_REPLY;														// prepare the send message, payload was filled already
	snd_msg.active = MSG_ACTIVE::ANSWER_BIDI;												// answer the request but we need an ACK
}

void AS::send_SEND_AES_TO_HMLAN() {
}
void AS::send_SEND_AES_TO_ACTOR() {
}

/*
* @brief Transmit the device serial number
*
*    LEN CNT FLAG BY03 SND       RCV       By10  SERIAL
* m> 14  16  A0   10   63 19 64  33 11 22  00    01 02 03 04 05 06 07 08 09 10 * /
*/
void AS::send_INFO_SERIAL() {
	s_m1000xx *msg = &snd_msg.m1000xx;														// short hand to info serial struct
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);										// copy the serial number

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
void AS::send_INFO_PEER_LIST(uint8_t cnl) {
	s_list_msg   *lm = &list_msg;														// short hand to the struct with all information for slice wise send
	s_peer_table *peerDB = &cmm[cnl]->peerDB;												// short hand to the respective peer table of the channel

	lm->active = LIST_ANSWER::PEER_LIST;													// we want to get the peer list
	lm->peer = peerDB;																		// pointer to the respective peerDB struct
	lm->max_slc = peerDB->get_nr_slices();													// get an idea of the total needed slices
	lm->timer.set(15);																		// some time between last message
	DBG(AS, F("AS:send_INFO_PEER_LIST, cnl:"), cnl, F(", slices:"), lm->max_slc, '\n');
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
void AS::send_INFO_PARAM_RESPONSE_PAIRS(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
	s_list_msg   *lm = &list_msg;														// short hand to the struct with all information for slice wise send
	s_peer_table *peerDB = &cmm[cnl]->peerDB;												// short hand to the respective peer table of the channel
	s_list_table *list = cmm[cnl]->list[lst];												// short hand to the respective list table

	if (!list) return;																		// specific list is not available
	uint8_t idx = peerDB->get_idx(peer_id);													// get the requested peer index
	if (idx == 0xff) return;																// nothing to do while index not found

	lm->active = LIST_ANSWER::PARAM_RESPONSE_PAIRS;											// we want to get the param list as pairs
	lm->peer_idx = peerDB->get_idx(peer_id);												// remember on the peer index
	lm->list = list;																		// pointer to the respective list struct
	lm->max_slc = list->get_nr_slices_pairs();												// get an idea of the total needed slices, plus one for closing 00 00 message
	lm->timer.set(15);																		// some time between last message
	DBG(AS, F("AS:send_INFO_PARAM_RESPONSE_PAIRS, cnl:"), cnl, F(", lst:"), lst, F(", peer:"), _HEX(peer_id, 4), F(", idx:"), lm->peer_idx, F(", slices:"), lm->max_slc, '\n');
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
void AS::send_INFO_PARAM_RESPONSE_SEQ(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
	//config_list_answer_slice.type = LIST_ANSWER::PARAM_RESPONSE_PAIRS;;					// we want to get the param list sequential
	//config_list_answer_slice.list = ptr_CM[cnl]->list[lst];								// pointer to the respective list struct
	//config_list_answer_slice.max_slc = ptr_CM[cnl]->list[lst]->get_nr_slices();			// get an idea of the total needed slices
	//config_list_answer_slice.active = 1;													// and set it active
}

void AS::send_INFO_PARAMETER_CHANGE() {
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
void AS::send_INFO_ACTUATOR_STATUS(uint8_t *chnl, uint8_t *stat, uint8_t *flag, uint8_t *sum) {
	//<frame id = "INFO_LEVEL" direction = "from_device" allowed_receivers = "BROADCAST,CENTRAL,OTHER" event = "true" type = "0x10" subtype = "6" subtype_index = "9" channel_field = "10">
	//	<parameter type = "integer" index = "11.0" size = "1.0" param = "LEVEL" / >
	//	<parameter type = "integer" index = "12.1" size = "0.3" param = "ERROR" / >
	//	<parameter type = "integer" index = "12.1" size = "0.1" param = "ERROR_OVERLOAD" / >
	//	<parameter type = "integer" index = "12.2" size = "0.1" param = "ERROR_OVERHEAT" / >
	//	<parameter type = "integer" index = "12.3" size = "0.1" param = "ERROR_REDUCED" / >
	//	<parameter type = "integer" index = "12.4" size = "0.3" param = "STATE_FLAGS" / >
	//	<parameter type = "integer" index = "12.4" size = "0.2" param = "DIRECTION_FLAGS" / >
	//	<parameter type = "integer" index = "14.0" size = "1.0" param = "LEVEL_REAL" / >
	//< / frame>
		// byte 13 is RSSI                          CNL STAT FLAG RSSI SUM
	// 0F  00 A0 10   33 11 24   63 19 64  06   01  00   00   64   00

	snd_msg.buf[11] = *chnl;																// add the channel
	snd_msg.buf[12] = *stat;																// and the status/value
	snd_msg.buf[13] = *flag;																// flags are prepared in the status poll function
	if (bat->get_status()) snd_msg.buf[13] |= 0x80;											// highest bit is battery flag
	else snd_msg.buf[13] &= 0x7F;
	snd_msg.buf[14] = com->rssi;															// add rssi information
	snd_msg.buf[15] = *sum;																	// we can add it to the buffer in any case, while length byte is set below

	snd_msg.active = MSG_ACTIVE::PAIR;														// for address, counter and to make it active
	if (sum) snd_msg.type = MSG_TYPE::INFO_ACTUATOR_STATUS_SUM;								// length and flags are set within the snd_msg struct
	else snd_msg.type = MSG_TYPE::INFO_ACTUATOR_STATUS;
	// todo: check AES
}

void AS::send_INFO_TEMP() {
}

void AS::send_HAVE_DATA() {
}

//void send_SWITCH(s_peer_table *peerDB) {
//}
void AS::send_TIMESTAMP(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}

/*
* @brief prepares a REMOTE message to all registered peers
* This message is used by push buttone devices like the HM_PB_6_WM55
* @parameter bidi, 1 if we need an ACK, 0 if it is a repeated long
*            *channel_module, ptr to the respective channel module, use "this"
*            *ptr_payload, pointer to the payload, in this case it is a fixed 2 byte array
*/
void AS::send_REMOTE(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
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
void AS::send_SENSOR_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_SWITCH_LEVEL(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_SENSOR_DATA(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_GAS_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_CLIMATE_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_SET_TEAM_TEMP(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_THERMAL_CONTROL(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_POWER_EVENT_CYCLE(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_POWER_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}
void AS::send_WEATHER_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload) {
}







/* - device related functions without any relation to a specific channel */
void AS::INSTRUCTION_RESET(s_m1104xx *buf) {
	DBG(AS, F("CM:INSTRUCTION_RESET\n"));
	send_ACK();																				// prepare an ACK message
	dev_operate.reset = 2;																	// set the reset flag and wait...
	//if (snd_msg.active) snd_poll();														// poll to get the ACK message send
	//clearEEPromBlock(0, 2);																// delete the magic byte in eeprom 
	//init();																				// call the init function to get the device in factory status
}
void AS::INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf) {
	DBG(AS, F("CM:INSTRUCTION_ENTER_BOOTLOADER\n"));
}
void AS::INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf) {
	DBG(AS, F("CM:INSTRUCTION_ADAPTION_DRIVE_SET\n"));
}
void AS::INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf) {
	DBG(AS, F("CM:INSTRUCTION_ENTER_BOOTLOADER2\n"));
}



/*
* @brief Search for a 4 byte peer address in all channel instances and returns
*        the channel number where the peer was found. Returns 0 if nothing was found.
*/
uint8_t AS::is_peer_valid(uint8_t *peer) {
	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		if (cmm[i]->peerDB.get_idx(peer) != 0xff) return i;									// ask the peer table to find the peer, if found, return the cnl
	}
	return 0;																				// nothing was found, return 0
}




/*
 * @brief Process message MESSAGE_KEY_EXCHANGE.
 *
 * Message description:
 *             Sender__ Receiver Decrypted Payload with one key part
 * 0E 08 80 02 1F B7 4A 23 70 D8 81 78 5C 37 30 65 61 93 1A 63 CF 90 44 31 60 4D
*/
/*inline void AS::processMessageKeyExchange(void) {
	memcpy(rcv_msg.prev_buf, rcv_msg.buf, rcv_msg.buf[0]+1);												// remember this message
//	rcv.prevBufUsed = 1;																		// ToDo: check if we need this here

	aes128_init(dev_ident.HMKEY, &ctx);																// load HMKEY
	aes128_dec(rcv_msg.buf+10, &ctx);															// decrypt payload width HMKEY first time

	#ifdef AES_DBG
	dbg << F("decrypted buf: ") << _HEX(rcv_msg.buf+10, 16) << '\n';
	#endif

	if (rcv_msg.buf[10] == 0x01) {																// the decrypted data must start with 0x01
		keyPartIndex = (rcv_msg.buf[11] & 1) ? AS_STATUS_KEYCHANGE_ACTIVE2 : AS_STATUS_KEYCHANGE_ACTIVE1;
	
		if (keyPartIndex == AS_STATUS_KEYCHANGE_ACTIVE1) {
			newHmKeyIndex[0] = rcv_msg.buf[11];
		}

		memcpy(newHmKey + keyPartIndex, rcv_msg.buf+12, 8);

		#ifdef AES_DBG
		dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << ", keyPartIndex: " << _HEX(keyPartIndex) << '\n';
		#endif

		sendSignRequest(0);
	
	} else {
		keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;
	}
}

/*
 * @brief Process message RESPONSE_AES_CHALLANGE.
 *
 * Message description:
 *             Sender__ Receiver By10 By11  Challenge_____ KeyIndex
 * 11 24 80 02 1F B7 4A 63 19 63 02   04 01 02 03 04 05 06 02`
 *
 * The Encryption:
 * 1. The temporarily key was built by XORing the key with the challenge
 * 2. Prepare the payload:
 *    6 Random-Bytes___ The bytes 1-11 of the message to sign
 *    xx xx xx xx xx xx 0A A4 01 23 70 EC 1E 7A AD 02
 * 3. Encrypt the payload width the generated temporarily key first time -> ePL (encrypted Payload)
 * 4. IV (initial vector) was build from bytes 11 - n of the message to sign padded with 0x00
 * 5. The encrypted payload (ePL) was XORed with the IV -> ePl^IV
 * 6. Encrypt the ePl^IV width the generated temporarily again
 */
/*	inline void AS::processMessageResponseAES_Challenge(void) {
		uint8_t i;

		snd_msg.clear();																			// cleanup send module data;
		initPseudoRandomNumberGenerator();

		uint8_t challenge[6];
		memcpy(challenge, rcv_msg.buf+11, 6);														// get challenge

		makeTmpKey(challenge);																	// Build the temporarily key from challenge

		// Prepare the payload for encryption.
		uint8_t msgLen = snd_msg.prev_buf[5];														// the message length stored at byte 5
		for (i = 0; i < 32; i++) {
			if (i < 6) {
				snd_msg.prev_buf[i] = (uint8_t)rand();												// fill the first 6 bytes with random data
			} else if (i > msgLen + 5 ) {
				snd_msg.prev_buf[i] = 0x00;															// the unused message bytes padded with 0x00
			}
		}

		aes128_enc(snd_msg.prev_buf, &ctx);															// encrypt the message first time
		for (i = 0; i < 16; i++) {
			snd_msg.prev_buf[i] ^= snd_msg.prev_buf[i+16];												// xor encrypted payload with IV (the bytes 11-27)
		}

		aes128_enc(snd_msg.prev_buf, &ctx);															// encrypt payload again
		snd_msg.mBody.MSG_LEN = 0x19;
	}
#endif


/**
 * @brief Reset the Device
 *        Set all register to default 0x00, reset HMKEY, reset device via watchdog,
 *        and so on.
 */
/*void AS::deviceReset(uint8_t clearEeprom) {
	if (clearEeprom == AS_RESET_CLEAR_EEPROM) {
		clearEEPromBlock(0, 2);
	}

	#ifdef WDT_RESET_ON_RESET
		wdt_enable(WDTO_15MS);																	// configure the watchdog so the reset sould trigger in 15ms
	#else
		init();
		ld.set(welcome);
	#endif
}
*/

	
	
	



