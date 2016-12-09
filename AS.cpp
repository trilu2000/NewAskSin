/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin framework main class -------------------------------------------------------------------------------------------
* - with a lot of support from many people at FHEM forum
*   thanks a lot to martin876, dirk, pa-pa, martin, Dietmar63 and all i have not personal named here 
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"


/*
 * Comment out to disable AES support
 */
#define SUPPORT_AES

 /*
 * On device reset the watchdog hart reset the entire device.
 * Comment out to disable this.
 */
#define WDT_RESET_ON_RESET



#include "AS.h"
#include <avr/wdt.h>





s_pair_mode   pair_mode;																	// helper structure for keeping track of active pairing mode
s_config_mode config_mode;																	// helper structure for keeping track of active config mode

s_aes_key     aes_key;																		// struct for handling AES relevant variables
s_dev_ident   dev_ident;																	// struct to hold the device identification related information									
s_dev_operate dev_operate;																	// struct to hold all operational variables or pointers

s_rcv_msg     rcv_msg;																		// struct to process received strings
s_snd_msg     snd_msg;																		// same for send strings


// public:		//---------------------------------------------------------------------------------------------------------
/**
 * @brief Initialize the AskSin Module
 */
void AS::init(void) {
	keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;

	/* - init eeprom function if a i2c eeprom is used and 
	* prepare the defaults incl eeprom address map for the channel modules */
	initEEProm();		
	uint16_t pAddr = cm_prep_default(sizeof(dev_ident));

	/* - First time start check is done via comparing a magic number at the start of the eeprom
	* with the CRC of the different lists in the channel modules. Every time there was a
	* change in the configuration some addresses are changed and we have to rewrite the eeprom content.	*/
	uint16_t flashCRC = cm_calc_crc();														// calculate the crc of all channel module list0/1, list3/4
	getEEPromBlock(0, sizeof(dev_ident), &dev_ident);										// get magic byte and all other information from eeprom
	dbg << F("AS:init crc- flash:") << flashCRC << F(", eeprom: ") << dev_ident.MAGIC << '\n';	// some debug
	DBG(AS, F("AS:init crc- flash:"), flashCRC, F(", eeprom: "), dev_ident.MAGIC, '\n');	// some debug

	if (flashCRC != dev_ident.MAGIC) {	

		/* - Write the defaults into the respective lists in eeprom and clear the peer database.
		* defaults are read from channel modules PROGMEM section, copied into the value byte array
		* and written to the eeprom.  */
		for (uint8_t i = 0; i < cnl_max; i++) {												// write the defaults in respective list0/1
			s_list_table *pList = &ptr_CM[i]->lstC;											// short hand to list 
			s_peer_table *pPeer = &ptr_CM[i]->peerDB;										// short hand to peer db
			pList->load_default();															// copy from progmem into array
			pList->save_list();																// write it into the eeprom
			pPeer->clear_all();
			DBG(AS, F("AS:write_defaults, cnl:"), pCM->lstC.cnl, F(", lst:"), pCM->lstC.lst, F(", len:"), pCM->lstC.len, '\n');
		}

		/* - First time detected
		* get the homematic id, serial number, aes key index and homematic aes key from flash and copy it into the eeprom
		* order in HMSerialData[]                 * HMID *, * Serial number *, * Default-Key *, * Key-Index *
		* order in dev_ident struct   *	MAGIC *, * HMID[3] *, * SERIAL_NR[10] *, * HMKEY[16] *, * HMKEY_INDEX *
		* we can copy the complete struct with a 2 byte offset in regards to the magic byte */
		dev_ident.MAGIC = flashCRC;															// set new magic number
		memcpy_P(((uint8_t*)&dev_ident) + 2, HMSerialData, sizeof(dev_ident) - 2);						// copy from PROGMEM
		setEEPromBlock(0, sizeof(dev_ident), ((uint8_t*)&dev_ident));
		while (!eeprom_is_ready());
		DBG(AS, F("AS:writing new magic byte\n"));											// some debug

		/* - function to be placed in register.h, to setup default values on first time start */
		firstTimeStart();				
	}

	/* - Initialize the hardware. All this functions are defined in HAL.h and HAL_extern.h 	*/
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts
	initMillis();																			// start the millis counter
	
	cc.init();																				// init the rf module
	init_random();																			// generate the random seed

	/* load list 0 and 1 defaults and inform the channel modules */
	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		cmMaster *pCM = ptr_CM[i];															// short hand to respective channel master	
		pCM->lstC.load_list();																// read the defaults in respective list0/1
		pCM->info_config_change();															// inform the channel modules
		pCM->init();																		// initialize the channel modules
	}

	/* - add this function in register.h to setup default values every start */
	everyTimeStart();
}

/**
 * @brief Cyclic poll all related functions
 */
void AS::poll(void) {

	/* copy the decoded data into the receiver module if something was received
	*  and poll the received buffer, it checks if something is in the queue  */
	if (ccGetGDO0()) {																			// check if something is in the cc1101 receive buffer
		cc.rcvData(rcv_msg.buf);																// if yes, get it into our receive processing struct
		rcv_poll();																				// and poll the receive function to get intent and some basics
	}
	if (rcv_msg.buf[0]) process_message();														// check if we have to handle the receive buffer

	/* handle the send module */
	snd_poll();																					// check if there is something to send

	/* time out the config flag */
	if (config_mode.active) {																	// check only if we are still in config mode
		if (config_mode.timer.done()) config_mode.active = 0;									// when timer is done, set config flag to inactive
	}

	/* regular polls for the channel modules */
	for (uint8_t i = 0; i < cnl_max; i++) {														// poll all the registered channel modules
		ptr_CM[i]->poll();
	}



	if (resetStatus == AS_RESET || resetStatus == AS_RESET_CLEAR_EEPROM) {
		//deviceReset(resetStatus);
	}

	// time out the pairing timer
	if (pair_mode.active) { 
		if (pair_mode.timer.done()) {
			pair_mode.active = 0;
			isEmpty(dev_operate.MAID, 3)? led.set(pair_err) : led.set(pair_suc);
		}
	}


	btn.poll();																			// poll the config button
	led.poll();																					// poll the led's
	bat.poll();																					// poll the battery check
		
	// check if we could go to standby
	pom.poll();																					// poll the power management
}


/* ------------------------------------------------------------------------------------------------------------------------
* - receive functions -----------------------------------------------------------------------------------------------------
* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
* The intent is to overload them there by the respective user channel module and work with the information accordingly.
*/
void AS::rcv_poll(void) {

	/* checks a received string for validity and intent */
	if (rcv_msg.mBody.MSG_LEN < 9) {														// check if the string has all mandatory bytes, if not
		DBG(RV, F("  too short...\n"));
		rcv_msg.clear();																	// clear receive buffer
		return;
	}

	/* check for a repeated string which was already processed */
	if ((rcv_msg.mBody.FLAG.RPTED) && (rcv_msg.prev_MSG_CNT == rcv_msg.mBody.MSG_CNT)) {	// check if message was already received
		DBG(RV, F("  repeated ...\n"));
		rcv_msg.clear();																	// clear receive buffer
		return;
	}
	rcv_msg.prev_MSG_CNT = rcv_msg.mBody.MSG_CNT;											// remember for next time

	/* get the intend of the message */
	get_intend();																			// no params neccassary while everything is in the recv struct

	/* filter out the footprint of MAX! devices
	*  b> 0F 04 86 10 38 EB 06 00 00 00 0A 24 B8 0C 00 40  (1963077) */
	if ((rcv_msg.mBody.MSG_LEN == 0x0f) && (*(uint8_t*)&rcv_msg.mBody.FLAG == 0x86) && (rcv_msg.intend == MSG_INTENT::BROADCAST)) {
		rcv_msg.clear();
		return;
	}

	DBG(RV, (char)rcv_msg.intend, F("> "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), ' ', _TIME, '\n');

	/* sort out messages not needed to further processing */
	if ((rcv_msg.intend == MSG_INTENT::LOGGING) || (rcv_msg.intend == MSG_INTENT::ERROR)) {
		rcv_msg.clear();																	// nothing to do any more
		return;
	}

	/* broadcast messages not used, with one exception - serial pair request */
	if ((rcv_msg.intend == MSG_INTENT::BROADCAST) && (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) && (rcv_msg.mBody.BY11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL)))
		rcv_msg.intend = MSG_INTENT::MASTER;

	/* logging and error is already eliminated from further processing, now we can take out broadcasts */
	if (rcv_msg.intend == MSG_INTENT::BROADCAST) {
		rcv_msg.clear();																	// nothing to do any more
		return;
	}
	//rcv_msg.hasdata = 1;																	// signalize that something is to do
}

/*
* @brief get the intend of the message
* This function is part of the rcv_poll function and searches based on the sender and receiver address for the intend of the 
* given message. It is important to know, because we work only on messages which are addressed to us and sent by a pair or peer.
*/
void AS::get_intend() {
	/* prepare the peer search */
	memcpy(rcv_msg.peer, rcv_msg.mBody.SND_ID, 3);											// peer has 4 byte and the last byte indicates the channel but also lowbat and long message, therefore we copy it together in a seperate byte array
	uint8_t buf10 = rcv_msg.buf[10];														// get the channel byte seperate
	if (aes_key.active) buf10 = aes_key.prev_rcv_buf[10];									// if AES is active, we must get buf[10] from prevBuf[10]
	rcv_msg.peer[3] = buf10 & 0x3f;															// mask out long and battery low

	/* it could come as a broadcast message - report it only while loging is enabled */
	if (isEmpty(rcv_msg.mBody.RCV_ID, 3))													// broadcast message
		rcv_msg.intend = MSG_INTENT::BROADCAST;

	/* it could be addressed to a different device - report it only while loging is enabled
	*  memcmp gives 0 if string matches, any other value while no match */
	else if (!isEqual(rcv_msg.mBody.RCV_ID, dev_ident.HMID, 3)) 							// not for us, only show as log message
		rcv_msg.intend = MSG_INTENT::LOGGING;

	/* because of the previous check, message is for us, check against master */
	else if (isEqual(rcv_msg.mBody.SND_ID, dev_operate.MAID, 3))							// coming from master
		rcv_msg.intend = MSG_INTENT::MASTER;

	/* message is for us, but not from master, maybe it is a peer message */
	else if (rcv_msg.cnl = is_peer_valid(rcv_msg.peer))										// check if the peer is known and remember the channel
		rcv_msg.intend = MSG_INTENT::PEER;

	/* message is for us, but not from pair or peer, check if we were the sender and flag it as internal */
	else if (isEqual(rcv_msg.mBody.SND_ID, dev_ident.HMID, 3))								// we were the sender, internal message
		rcv_msg.intend = MSG_INTENT::INTERN;

	/* message is for us, but not from pair or peer or internal - check if we miss the master id because we are not paired */
	else if (isEmpty(dev_operate.MAID, 3))													// for us, but pair is empty
		rcv_msg.intend = MSG_INTENT::NOT_PAIRED;

	else																					// should never happens
		rcv_msg.intend = MSG_INTENT::ERROR;
}

/**
* @brief Receive handler: Process received messages
* Process message is called while rcv_msg.hasdata is set to TRUE. It takes all information from the rcv_msg struct.
* Sort out by several if's which message we received and delegates the reuqest for answer to the according class/function.
* 
*/
void AS::process_message(void) {
	cmMaster *pCM;

	/* first we check if AES is enabled and if the message is not an ACK type message or was already challanged, if so, we send back an AES_REQ */
	if ((*dev_operate.AES_FLAG) && (rcv_msg.mBody.MSG_TYP != BY03(MSG_TYPE::ACK_MSG)) && (aes_key.active == MSG_AES::NONE)) {
		/* received command
		*  x> 10 01 A0 01 63 19 64 00 11 22 00 05 00 00 00 00 00  (17641)
		*  we send an AES_REQ                 ___6 byte random_ __7th byte HM Key Number
		*  <-11 01 A0 02 00 11 22 63 19 64 04 B8 91 CC 01 F3 73 00  (17671)
		*  we receive an AES_REPLY          __encrypeted message start at byte 10, len is 16 byte
		*  x> 19 01 A0 03 63 19 64 00 11 22 30 EB DC A1 C4 18 2A A2 2E F0 9B EC 96 9B 72 6A (17810)
		*/
		send_AES_REQ();																		// create an ACK_REQ message
																							// nothing to do here any more
		return;	
	}


	if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::DEVICE_INFO)) {
		/* not sure what to do with while received, probably nothing */

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) {
		/* config request messages are used to configure a devive by writing registers and peers -
		*  check the channel and forward for processing to the respective function */
		if (rcv_msg.mBody.BY10 >= cnl_max) return;											// channel is out of range, return

		uint8_t by11 = rcv_msg.mBody.BY11;													// short hand to byte 11 in the received string
		pCM = ptr_CM[rcv_msg.mBody.BY10];													// short hand to the respective channel module instance
		s_config_mode *cm = &config_mode;													// short hand to config mode struct

		if      (by11 == BY11(MSG_TYPE::CONFIG_PEER_ADD))       pCM->CONFIG_PEER_ADD(&rcv_msg.m01xx01);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_REMOVE))    pCM->CONFIG_PEER_REMOVE(&rcv_msg.m01xx02);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_LIST_REQ))  pCM->CONFIG_PEER_LIST_REQ(&rcv_msg.m01xx03);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PARAM_REQ))      pCM->CONFIG_PARAM_REQ(&rcv_msg.m01xx04);
		else if (by11 == BY11(MSG_TYPE::CONFIG_START))          pCM->CONFIG_START(&rcv_msg.m01xx05);
		else if (by11 == BY11(MSG_TYPE::CONFIG_END))            pCM->CONFIG_END(&rcv_msg.m01xx06);
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX1))   pCM->CONFIG_WRITE_INDEX1(&rcv_msg.m01xx07);
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX2))   pCM->CONFIG_WRITE_INDEX2(&rcv_msg.m01xx08);
		else if (by11 == BY11(MSG_TYPE::CONFIG_SERIAL_REQ))     pCM->CONFIG_SERIAL_REQ(&rcv_msg.m01xx09);
		else if (by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL))    pCM->CONFIG_PAIR_SERIAL(&rcv_msg.m01xx0a);
		else if (by11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST)) pCM->CONFIG_STATUS_REQUEST(&rcv_msg.m01xx0e);

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::ACK_MSG)) {
		/* at the moment we need the ACK message only for avoiding resends, so let the send_msg struct know about
		*  a received ACK/NACK whatever - probably we have to change this function in the future */
		if (rcv_msg.mBody.MSG_CNT == snd_msg.mBody.MSG_CNT) snd_msg.retr_cnt = 0xff;		// check if the message counter is similar and let the send function know

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::AES_REPLY)) {
		/* we received an AES_REPLY, first we tell the send function that we received an answer. as the receive flag is not cleared, we will come back again */
		if (snd_msg.active) {
			snd_msg.retr_cnt = 0xff;														// we received an answer to our request, no need to resend
			return;
		}
		aes_key.check_AES_REPLY(dev_ident.HMKEY, rcv_msg.buf);								// check the data, if ok, the last message will be restored, otherwise the hasdata flag will be 0
		return;																				// next round to work on the restored message

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SEND_AES)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::REPLY_MSG)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::INSTRUCTION_MSG)) {
		uint8_t by10 = rcv_msg.mBody.BY10;													// short hand to byte 10 in the received string

		/* not channel related, do it in any case and clear rcv_msg */
		pCM = ptr_CM[0];																	// we will process it in AS???
		if      (by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER))   pCM->INSTRUCTION_ENTER_BOOTLOADER(&rcv_msg.m1183xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER2))  pCM->INSTRUCTION_ENTER_BOOTLOADER2(&rcv_msg.m11caxx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_ADAPTION_DRIVE_SET)) pCM->INSTRUCTION_ADAPTION_DRIVE_SET(&rcv_msg.m1187xx);

		/* everything below is channel related */
		pCM = ptr_CM[rcv_msg.mBody.BY11];													// short hand to respective channel module instance
		if      (by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_OFF))        pCM->INSTRUCTION_INHIBIT_OFF(&rcv_msg.m1100xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_ON))         pCM->INSTRUCTION_INHIBIT_ON(&rcv_msg.m1101xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SET))                pCM->INSTRUCTION_SET(&rcv_msg.m1102xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_STOP_CHANGE))        pCM->INSTRUCTION_STOP_CHANGE(&rcv_msg.m1103xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_RESET))              pCM->INSTRUCTION_RESET(&rcv_msg.m1104xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LED))                pCM->INSTRUCTION_LED(&rcv_msg.m1180xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LED_ALL))            pCM->INSTRUCTION_LED_ALL(&rcv_msg.m1181xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LEVEL))              pCM->INSTRUCTION_LEVEL(&rcv_msg.m1181xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SLEEPMODE))          pCM->INSTRUCTION_SLEEPMODE(&rcv_msg.m1182xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SET_TEMP))           pCM->INSTRUCTION_SET_TEMP(&rcv_msg.m1186xx);

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::HAVE_DATA)) {

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SWITCH)) {
		/* to process this message we need to load the right list table for the respective peer index into memory
		* need also to find the right channel to issue the message, depending on the peer address */
		memcpy(rcv_msg.peer, rcv_msg.m3Exxxx.PEER, 3);										// prepare a peer string
		rcv_msg.peer[3] = rcv_msg.m3Exxxx.P_CNL;
		rcv_msg.cnl = is_peer_valid(rcv_msg.peer);											// search for the peer channel
		if (!rcv_msg.cnl) return;															// peer not found in any channel, return

		pCM = ptr_CM[rcv_msg.cnl];															// short hand to the respective channel module
		pCM->lstP.load_list(pCM->peerDB.get_idx(rcv_msg.peer));								// load the respective list 3 with the respective index 
		pCM->SWITCH(&rcv_msg.m3Exxxx);

	} else if (rcv_msg.intend == MSG_INTENT::PEER) {
		/* it is a peer message, which was checked in the receive class, so reload the respective list 3/4 */
		pCM = ptr_CM[rcv_msg.cnl];															// we remembered on the channel by checking validity of peer
		pCM->lstP.load_list(ptr_CM[rcv_msg.cnl]->peerDB.get_idx(rcv_msg.peer));				// load the respective list 3
		if      (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::TIMESTAMP))         pCM->TIMESTAMP(&rcv_msg.m3fxxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::REMOTE))            pCM->REMOTE(&rcv_msg.m40xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SENSOR_EVENT))      pCM->SENSOR_EVENT(&rcv_msg.m41xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SWITCH_LEVEL))      pCM->SWITCH_LEVEL(&rcv_msg.m42xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SENSOR_DATA))       pCM->SENSOR_DATA(&rcv_msg.m53xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::GAS_EVENT))         pCM->GAS_EVENT(&rcv_msg.m54xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CLIMATE_EVENT))     pCM->CLIMATE_EVENT(&rcv_msg.m58xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::SET_TEAM_TEMP))     pCM->SET_TEAM_TEMP(&rcv_msg.m59xxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::THERMAL_CONTROL))   pCM->THERMAL_CONTROL(&rcv_msg.m5axxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::POWER_EVENT_CYCLE)) pCM->POWER_EVENT_CYCLE(&rcv_msg.m5exxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::POWER_EVENT))       pCM->POWER_EVENT(&rcv_msg.m5fxxxx);
		else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::WEATHER_EVENT))     pCM->WEATHER_EVENT(&rcv_msg.m70xxxx);
	} else {
		dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';
		DBG(AS, F("AS:message not known - please report: "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
	}

	rcv_msg.buf[0] = 0;
	//rcv_msg.hasdata = 0;
	return;

	/*

	uint8_t by10 = rcv_msg.mBody.BY10 - 1;

	// check which type of message was received
	if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_DEVINFO) {
		//TODO: do something with the information

	}
	else if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_CONFIG) {

		if (rcv_msg.mBody.BY11 == AS_CONFIG_SERIAL_REQ) {
			//processMessageConfigSerialReq();

		}
		else if (rcv_msg.mBody.BY11 == AS_CONFIG_PAIR_SERIAL) {
			//processMessageConfigPairSerial();

		}
		else if (rcv_msg.mBody.BY11 == AS_CONFIG_STATUS_REQUEST) {
			//processMessageConfigStatusRequest(by10);

		}
		else {
			processMessageConfigAESProtected();
		}

	}
	else if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_RESPONSE) {
		/*
		* This is an response (ACK) to an active message.
		* In exception of AS_RESPONSE_AES_CHALLANGE message, we set retrCnt to 0xFF
		*/
/*		if ((snd_msg.active) && (rcv_msg.mBody.MSG_CNT == snd_msg.temp_MSG_CNT) && (rcv_msg.mBody.BY10 != AS_RESPONSE_AES_CHALLANGE)) {
			//snd_msg.retr_cnt = 0xFF;
		}

		if (rcv_msg.mBody.BY10 == AS_RESPONSE_ACK) {
			/*
			* Message description:
			*             Sender__ Receiver ACK
			* 0A 05 80 02 63 19 63 01 02 04 00
			*/
			// nothing to do yet
/*
		}
		else if (rcv_msg.mBody.BY10 == AS_RESPONSE_ACK_STATUS) {
			/*
			* Message description:
			*             Sender__ Receiver ACK Channel State Action RSSI
			* 0E 08 80 02 1F B7 4A 23 70 D8 01  01      C8    80     27
			*
			* Action: Down=0x20, UP=0x10, LowBat=&0x80
			*/
			// nothing to do yet
/*
		}
		else if (rcv_msg.mBody.BY10 == AS_RESPONSE_ACK2) {
			// nothing to do yet

#ifdef SUPPORT_AES
		}
		else if (rcv_msg.mBody.BY10 == AS_RESPONSE_AES_CHALLANGE) {
			processMessageResponseAES_Challenge();

			memcpy(snd_msg.buf + 10, snd_msg.prev_buf, 16);
			prepareToSend(rcv_msg.mBody.MSG_CNT, AS_MESSAGE_RESPONSE_AES, rcv_msg.mBody.SND_ID);
#endif

		}
		else if (rcv_msg.mBody.BY10 == AS_RESPONSE_NACK) {
			// nothing to do yet

		}
		else if (rcv_msg.mBody.BY10 == AS_RESPONSE_NACK_TARGET_INVALID) {
			// nothing to do yet

		}

#ifdef SUPPORT_AES
	}
	else if ((rcv_msg.mBody.MSG_TYP == AS_MESSAGE_RESPONSE_AES)) {
		/*
		* Message description:
		*             Sender__ Receiver AES-Response-Data
		* 0E 08 80 02 1F B7 4A 23 70 D8 6E 55 89 7F 12 6E 63 55 15 FF 54 07 69 B3 D8 A5
		*/
/*		snd_msg.clear();																		// cleanup send module data;

		uint8_t iv[16];																		// 16 bytes initial vector
		memset(iv, 0x00, 16);																// fill IV with 0x00;
		memcpy(iv, rcv_msg.prev_buf + 11, rcv_msg.prev_buf[0] - 10);
		aes128_dec(rcv_msg.buf + 10, &ctx);														// decrypt payload with temporarily key first time

		for (uint8_t i = 0; i < 16; i++) rcv_msg.buf[i + 10] ^= iv[i];							// xor encrypted payload with iv

		uint8_t authAck[4];
		//memcpy(authAck, rcv_msg.buf + 10, 4);
		authAck[0] = rcv_msg.buf[10];
		authAck[1] = rcv_msg.buf[11];
		authAck[2] = rcv_msg.buf[12];
		authAck[3] = rcv_msg.buf[13];

		aes128_dec(rcv_msg.buf + 10, &ctx);														// decrypt payload with temporarily key again

																								/**
																								* Compare decrypted message with original message
																								*/
/*#ifdef AES_DBG
		dbg << F(">>> compare: ") << _HEX(rcv_msg.buf + 10, 16) << " | " << _HEX(rcv.prevBuf, 11) << '\n';
#endif

		// memcmp returns 0 if compare true
		if (!memcmp(rcv_msg.buf + 16, rcv_msg.prev_buf + 1, 10)) {										// compare bytes 7-17 of decrypted data with bytes 2-12 of msgOriginal
#ifdef AES_DBG
			dbg << F("Signature check OK\n");
#endif

			sendAckAES(authAck);															// send AES-Ack

			if (keyPartIndex == AS_STATUS_KEYCHANGE_INACTIVE) {
				memcpy(rcv_msg.buf, rcv_msg.prev_buf, rcv_msg.prev_buf[0] + 1);								// restore the last received message for processing from saved buffer
				rcv_msg.use_prev_buf = 0;

				if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_CONFIG) {
					//processMessageConfig();

				}
				else if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_ACTION) {
					//processMessageAction11();

				}
				else if (rcv_msg.mBody.MSG_TYP >= AS_MESSAGE_SWITCH_EVENT) {
					//uint8_t pIdx;
					//uint8_t cnl = getChannelFromPeerDB(&pIdx);
					//if (cnl > 0) {
						//processMessageAction3E(cnl, pIdx);
					//}
				}

			}
			else if (keyPartIndex == AS_STATUS_KEYCHANGE_ACTIVE2) {
				setEEPromBlock(15, 16, newHmKey);											// store HMKEY
				getEEPromBlock(15, 16, dev_ident.HMKEY);
				setEEPromBlock(14, 1, newHmKeyIndex);										// store used key index
				dev_ident.HMKEY_INDEX = newHmKeyIndex[0];
#ifdef AES_DBG
				dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << F(" ID: ") << _HEXB(hmKeyIndex[0]) << '\n';
#endif

				keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;
			}

		}
		else {
#ifdef AES_DBG
			dbg << F("Signature check FAIL\n");
#endif

			// ToDo: Check if needed.
			send_NACK();
		}

	}
	else if ((rcv_msg.mBody.MSG_TYP == AS_MESSAGE_KEY_EXCHANGE)) {									// AES Key Exchange
		processMessageKeyExchange();

#endif

	}
	else if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_ACTION) {												// action message
#ifdef SUPPORT_AES

		uint8_t aesActiveForReset = 0;
		if (rcv_msg.mBody.BY10 == AS_ACTION_RESET && rcv_msg.mBody.BY11 == 0x00) {						// device reset requested
			aesActiveForReset = checkAnyChannelForAES();									// check if AES activated for any channel			}
		}

		// check if AES for the current channel active or aesActiveForReset @see above
		if (*ptr_CM[rcv_msg.mBody.BY11]->list[1]->ptr_to_val(AS_REG_L1_AES_ACTIVE) == 1 || aesActiveForReset == 1) {
		//if (ee_list.getRegAddr(rcv_msg.mBody->BY11, 1, 0, AS_REG_L1_AES_ACTIVE) == 1 || aesActiveForReset == 1) {
				sendSignRequest(1);

		}
		else {
#endif

			//processMessageAction11();
			if (rcv_msg.mBody.FLAG.BIDI || resetStatus == AS_RESET) {
				if (resetStatus == AS_RESET) {   //(ee.getRegListIdx(1, 3) == 0xFF || resetStatus == AS_RESET) {
					send_ACK();
				}
				else {
					uint8_t channel = rcv_msg.mBody.BY11;
					if (rcv_msg.mBody.BY10 == AS_ACTION_RESET && rcv_msg.mBody.BY11 == 0x00) {
						channel = 1;
					}
					//send_ACK_STATUS(channel, 0, 0);
				}
			}

#ifdef SUPPORT_AES
		}
#endif

	}
	else if (rcv_msg.mBody.MSG_TYP == AS_MESSAGE_HAVE_DATA) {											// HAVE_DATA
																										// TODO: Make ready

	}
	else if (rcv_msg.mBody.MSG_TYP >= AS_MESSAGE_SWITCH_EVENT) {
		/*
		* used by message type 3E (SWITCH), 3F (TIMESTAMP), 40 (REMOTE), 41 (SENSOR_EVENT),
		*                      53 (SENSOR_DATA), 58 (CLIMATE_EVENT), 70 (WEATHER_EVENT)
		*
		* Message description:
		*                             Sender__ Receiver channel counter
		* Remote example: 0B 2D B4 40 23 70 D8 01 02 05 06      05
		*
		*                             Sender__ Receiver Destination na  channel counter
		* Switch example: 0F 18 B0 3E FD 24 BE 01 02 05 23 70 D8    40  06      00
		*
		* "3E"          => { txt => "SWITCH"      , params => {
		* 				DST      => "00,6",
		* 				UNKNOWN  => "06,2",
		* 				CHANNEL  => "08,2",
		* 				COUNTER  => "10,2", } },
		*/

/*		uint8_t pIdx;
		uint8_t cnl;// = getChannelFromPeerDB(&pIdx);

		//dbg << "cnl: " << cnl << " pIdx: " << pIdx << " mTyp: " << _HEXB(rcv.mBdy.mTyp) << " by10: " << _HEXB(rcv.mBdy.by10)  << " by11: " << _HEXB(rcv.mBdy.by11) << " data: " << _HEX((rcv_msg.buf+10),(rcv.mBdy.mLen-9)) << '\n'; _delay_ms(100);

		if (cnl > 0) {
#ifdef SUPPORT_AES
			// check if AES for the current channel active
			if (*ptr_CM[cnl]->list[1]->ptr_to_val(AS_REG_L1_AES_ACTIVE) == 1) {
			//if (ee_list.getRegAddr(cnl, 1, 0, AS_REG_L1_AES_ACTIVE) == 1) {
					sendSignRequest(1);

			}
			else {
#endif

				//processMessageAction3E(cnl, pIdx);

#ifdef SUPPORT_AES
			}
#endif
		}

	}
*/
	//rcv_msg.clear();																	// nothing to do any more
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

		led.set(ack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while
		return;
	}

	/*  return while no ACK received and timer is running */
	if (!sm->timer.done()) return;


	/* check for first time and prepare the send */
	if (!sm->retr_cnt) {

		/* check based on active flag if it is a message which needs to be prepared or only processed */
		if (sm->active >= MSG_ACTIVE::ANSWER) {
			sm->mBody.FLAG.RPTEN = 1;														// every message need this flag
			if (snd_msg.type == MSG_TYPE::AES_REQ) {										// only on AES_REQ we require an ACK
				sm->mBody.FLAG.BIDI = 1;
			} else {																		// default is no
				sm->mBody.FLAG.BIDI = 0;
			}
			memcpy(sm->mBody.SND_ID, dev_ident.HMID, 3);									// we always send the message in our name

			sm->mBody.MSG_TYP = BY03(sm->type);												// msg type
			if (BY10(sm->type) != 0xff) sm->mBody.BY10 = BY10(sm->type);					// byte 10
			if (BY11(sm->type) != 0xff) sm->mBody.BY11 = BY11(sm->type);					// byte 11
			if (MLEN(sm->type) != 0xff) sm->mBody.MSG_LEN = MLEN(sm->type);					// msg len
		}

		/* now more in detail in regards to the active flag */
		if (sm->active == MSG_ACTIVE::ANSWER) {
			/* answer means - msg_cnt and rcv_id from received string, no bidi needed, but bidi is per default off */
			memcpy(sm->mBody.RCV_ID, rcv_msg.mBody.SND_ID, 3);
			sm->mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;

		} else if (sm->active == MSG_ACTIVE::PAIR) {
			/* pair means - msg_cnt from snd_msg struct, rcv_id is master_id, bidi not needed */
			memcpy(sm->mBody.RCV_ID, dev_operate.MAID, 3);
			sm->mBody.MSG_CNT = sm->MSG_CNT;
			sm->MSG_CNT++;
			sm->mBody.FLAG.BIDI = 1;														// ACK required, will be detected later if not paired 

		} else if (sm->active == MSG_ACTIVE::PEER_BIDI) {
			sm->mBody.FLAG.BIDI = 1;														// ACK required, will be detected later if not paired 
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

		/* Copy the complete message to msgToSign. We need them for later AES signing.
		*  We need copy the message to position after 5 of the buffer.
		*  The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
		*  will fill with 6 bytes random data later.	*/
		memcpy(&sm->prev_buf[5], sm->buf, (sm->buf[0] > 26) ? 27 : sm->buf[0] + 1);
	}


	/* check the retr count if there is something to send, while message timer was checked earlier */
	if (sm->retr_cnt < sm->temp_max_retr) {													// not all sends done and timing is OK
		uint8_t tBurst = sm->mBody.FLAG.BURST;												// get burst flag, while string will get encoded
		cc.sndData(sm->buf, tBurst);														// send to communication module
		sm->retr_cnt++;																		// remember that we had send the message

		if (sm->mBody.FLAG.BIDI) sm->timer.set(sm->max_time);								// timeout is only needed while an ACK is requested
		led.set(send);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("<- "), _HEX(sm->buf, sm->buf[0] + 1), ' ', _TIME, '\n');					// some debug

	} else {
	/* if we are here, message was send one or multiple times and the timeout was raised if an ack where required */
	/* seems, nobody had got our message, other wise we had received an ACK */
		sm->clear();																		// clear the struct, while nothing to do any more

		if (!sm->mBody.FLAG.BIDI) return;													// everything fine, ACK was not required

		sm->timeout = 1;																	// set the time out only while an ACK or answer was requested
		led.set(noack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("  timed out "), _TIME, '\n');											// some debug
	}
}


















/*

#ifdef SUPPORT_AES
	/**
	 * @brief Send an ACK of previous AES handshake
	 *
	 * Message description:
	 *             Sender__ Receiver 04 bytes AES-Ack Data
	 * 0A 24 80 02 1F B7 4A 63 19 63 XX XX XX XX
	 *
	 * @param data pointer to aes ack data
	 */
/*	inline void AS::sendAckAES(uint8_t *data) {
		snd_msg.mBody.MSG_LEN = 0x0E;
		snd_msg.mBody.FLAG.BIDI = 0;
		snd_msg.mBody.BY10 = AS_RESPONSE_ACK;
		snd_msg.mBody.BY11 = data[0];
		memcpy(snd_msg.mBody.PAYLOAD, data+1, 3);

		prepareToSend(rcv_msg.mBody.MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody.SND_ID);
	}
#endif






void AS::sendINFO_POWER_EVENT(uint8_t *data) {
	snd_msg.mBody.MSG_LEN = 15; // 15, 16 works somehow but 12 + 6 = 18
	uint8_t cnt;

	if ((rcv_msg.mBody.MSG_TYP == AS_MESSAGE_CONFIG) && (rcv_msg.mBody.BY11 == AS_CONFIG_STATUS_REQUEST)) {
		cnt = rcv_msg.mBody.MSG_CNT;
	} else {
		cnt = snd_msg.MSG_CNT++;
	}
	#ifdef AS_DBG
		Serial << F("sendINFO_POWER_EVENT cnt: ");
		Serial.print(cnt, DEC);
	#endif

	//char* myBytes = reinterpret_cast<char*>(snd.mBdy.pyLd);


	snd_msg.mBody.FLAG.BIDI = (isEmpty(dev_operate.MAID,3))?0:1;
	//snd.mBdy.by10      = AS_MESSAGE_POWER_EVENT_CYCLIC;

	// set payload
	snd_msg.mBody.BY10 = data[0];
	snd_msg.mBody.BY11 = data[1]; // first byte of payload
	for (uint8_t i = 2; i < 6; i++){
		//dbg << "AS::sendINFO_POWER_EVENT BYTES: ("<< i <<" " << _HEXB(myBytes[i]) << " = " <<_HEXB(snd.mBdy.pyLd[i]) << "\n";
		snd_msg.mBody.PAYLOAD[i-2] = data[i];
	}
	//snd.mBdy.pyLd[0]   = state;
	//snd.mBdy.pyLd[1]   = flag; // | (bt.getStatus() << 7);
	//snd.mBdy.pyLd[2]   = cc.rssi;
	#ifdef AS_DBG
		Serial << F(" BIDI: ") << snd_msg.mBody->FLAG.BIDI << "\n";
	#endif
	prepareToSend(cnt, AS_MESSAGE_POWER_EVENT_CYCLIC, dev_operate.MAID);
}

void AS::sendINFO_TEMP(void) {
	//TODO: make ready

	//"10;p01=0A"   => { txt => "INFO_TEMP", params => {
	//SET     => '2,4,$val=(hex($val)>>10)&0x3F',
	//ACT     => '2,4,$val=hex($val)&0x3FF',
	//ERR     => "6,2",
	//VALVE   => "6,2",
	//MODE    => "6,2" } },
	// --------------------------------------------------------------------
}

void AS::sendHAVE_DATA(void) {
	//TODO: make ready#

	//"12"          => { txt => "HAVE_DATA"},
	// --------------------------------------------------------------------
}

/*void AS::sendSWITCH(void) {
	//TODO: make ready#

	//"3E"          => { txt => "SWITCH"      , params => {
	//DST      => "00,6",
	//UNKNOWN  => "06,2",
	//CHANNEL  => "08,2",
	//COUNTER  => "10,2", } },
	// --------------------------------------------------------------------
}*/

/*void AS::sendTimeStamp(void) {
	//TODO: make ready#

	//"3F"          => { txt => "TimeStamp"   , params => {
	//UNKNOWN  => "00,4",
	//TIME     => "04,2", } },
	// --------------------------------------------------------------------
}*/

/**
 * @brief Send a remote Event
 *
 * TODO: need to be rework
 *
 * Message description:
               Sender__ Receiver buttonByte counter
 * 0B 0A A4 40 23 70 EC 1E 7A AD 02         01
 *
 * btnByte: bits 0-5=Button Number (0-31), bit6=long press, bit7=low battery
 * counter: the counter increased at every button release.
 *
 * @param channel
 * @param burst
 * @param payload: pointer to payload
 */
/*void AS::sendREMOTE(uint8_t channel, uint8_t *payload, uint8_t msg_flag) {
	// burst flag is not needed, has to come out of list4, as well as AES flag
	sendEvent(channel, 0, AS_MESSAGE_REMOTE_EVENT, payload, 2);
}*/

/**
 * @brief Send a sensor Event
 *
 * TODO: need to be rework
 *
 * Message description:
 *             Sender__ Receiver buttonByte counter value
 * 0C 0A A4 41 23 70 EC 1E 7A AD 02         01      200
 *
 * btnByte: bits 0-5=Button Number (0-31), bit6=long press, bit7=low battery
 * counter: the counter increased at every button release.
 * value:   the sensor value
 *
 * @param channel
 * @param burst
 * @param payload: pointer to payload
 */
/*void AS::sendSensor_event(uint8_t channel, uint8_t burst, uint8_t *payload) {
	sendEvent(channel, AS_MESSAGE_SENSOR_EVENT, burst, payload, 3);
}*/

/**
 * @brief Send an event with arbitrary payload

 * TODO: need to be rework
 *
 * Message description:
 *             Sender__ Receiver buttonByte counter value
 * 0C 0A A4 41 23 70 EC 1E 7A AD 02         01      200
 *
 * btnByte: bits 0-5=Button Number (0-31), bit6=long press, bit7=low battery
 * counter: the counter increased at every button release.
 * value:   the sensor value

 * Take care when sending generic events, since there are no consistency
 * checks if the specified event type and payload make any sense. Rather use
 * predefined special send methods.
 *
 * @param channel The channel
 * @param msg_type    Message type
 * @param msg_flag    Set to 1 for burst mode, or 0
 * @param payload     Pointer to payload
 * @param pyl_len     Length of payload in bytes, not more than 16
 * @attention The payload length may not exceed 16 bytes. If a greater value
 * for len is given, it is limited to 16 to prevent HM-CFG-LAN (v0.961) to crash.
 */
/*void AS::sendEvent(uint8_t channel, uint8_t msg_type, uint8_t msg_flag, uint8_t *ptr_payload, uint8_t len_payload) {
	if (len_payload>16) {
		#ifdef AS_DBG
		dbg << "AS::sendGenericEvent(" << channel << "," << msg_flag << ",0x" << _HEX(&msg_type,1) << "," << len_payload << ",...): payload exceeds max len of 16\n";
		#endif
		len_payload = 16;
	}

	stcPeer.ptr_payload = ptr_payload;
	stcPeer.len_payload = len_payload + 1;
	stcPeer.channel     = channel;
	stcPeer.burst       = (msg_flag & AS_BURST) ? 1 : 0;										// not sure if it can be different for a whole peer list and has to come out of list4 of the respective channel
	stcPeer.bidi        = (msg_flag & AS_ACK_REQ) ? 1 : 0;
	//stcPeer.bidi        = (~payload[0] & AS_BUTTON_BYTE_LONGPRESS_BIT) ? 0 : 1;				// depends on long-key-press-bit (long didn't need ACK)	stcPeer.bidi   = (isEmpty(MAID,3)) ? 0 : 1;
	//stcPeer.bidi        = (isEmpty(MAID,3)) ? 0 : 1;
	stcPeer.msg_type    = msg_type;
	stcPeer.active      = 1;
}*/

/*void AS::sendSensorData(void) {
	//TODO: make ready#

	//"53"          => { txt => "SensorData"  , params => {
	//CMD => "00,2",
	//Fld1=> "02,2",
	//Val1=> '04,4,$val=(hex($val))',
	//Fld2=> "08,2",
	//Val2=> '10,4,$val=(hex($val))',
	//Fld3=> "14,2",
	//Val3=> '16,4,$val=(hex($val))',
	//Fld4=> "20,2",
	//Val4=> '24,4,$val=(hex($val))'} },
}*/

/*void AS::sendClimateEvent(void) {
	//TODO: make ready#

	//"58"          => { txt => "ClimateEvent", params => {
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
}*/

/*void AS::sendSetTeamTemp(void) {
	//TODO: make ready#

	//"59"          => { txt => "setTeamTemp" , params => {
	//CMD      => "00,2",
	//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
	//mode     => '02,2,$val=(hex($val) & 0x3)',} },
}*/

/*void AS::sendWeatherEvent(void) {
	//TODO: make ready#

	//"70"          => { txt => "WeatherEvent", params => {
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
}*/



/*
 * @brief get peered channel from peer db.
 *
 * @param pIdx must be a variable to receive the peer index
 *
 * @return channel number
 */
/*uint8_t AS::getChannelFromPeerDB(uint8_t *pIdx) {
	uint8_t cnl = 0;
	uint8_t tmp;

	// check if we have the peer in the database to get the channel
	if ((rcv_msg.mBody.MSG_TYP == AS_MESSAGE_SWITCH_EVENT) && (rcv_msg.mBody.MSG_LEN == 0x0F)) {
		tmp = rcv_msg.buf[13];																		// save byte13, because we will replace it
		rcv_msg.buf[13] = rcv_msg.buf[14];																// copy the channel byte to the peer
		cnl = is_peer_valid(rcv_msg.buf+10);													// check with the right part of the string
		if (cnl != 0xff) {
			*pIdx = ptr_CM[cnl]->peerDB.get_idx(rcv_msg.buf + 10);										// get the index of the respective peer in the channel store
		}
		rcv_msg.buf[13] = tmp;																		// get it back

	} else {
		cnl = is_peer_valid(rcv_msg.peer);
		if (cnl != 0xff) {
			*pIdx = ptr_CM[cnl]->peerDB.get_idx(rcv_msg.peer);										// get the index of the respective peer in the channel store
		}
	}

	return cnl;
}*/

//#ifdef SUPPORT_AES
	/*
	 * @brief Loop thru channels and check if AES is activated for any channel.
	 */
/*	uint8_t AS::checkAnyChannelForAES(void) {
		uint8_t i;
		for (i = 1; i < cnl_max; i++) {														// check if AES activated for any channel
			if (*ptr_CM[i]->list[1]->ptr_to_val(AS_REG_L1_AES_ACTIVE)) {
			//if (ee_list.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
				return 1;
			}
		}

		return 0;
	}

	/*
	 * @brief Process message MESSAGE_KEY_EXCHANGE.
	 *
	 * Message description:
	 *             Sender__ Receiver Decrypted Payload with one key part
	 * 0E 08 80 02 1F B7 4A 23 70 D8 81 78 5C 37 30 65 61 93 1A 63 CF 90 44 31 60 4D
	 */
/*	inline void AS::processMessageKeyExchange(void) {
		memcpy(rcv_msg.prev_buf, rcv_msg.buf, rcv_msg.buf[0]+1);												// remember this message
//		rcv.prevBufUsed = 1;																		// ToDo: check if we need this here

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
				dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << ", keyPartIndex: " << _HEXB(keyPartIndex) << '\n';
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











inline void AS::processMessageConfigAESProtected() {
	#ifdef SUPPORT_AES
		uint8_t aesActive = checkAnyChannelForAES();											// check if AES activated for any channel
		if (aesActive == 1) {
			sendSignRequest(1);

		} else {
	#endif
			//uint8_t ackOk = processMessageConfig();
			//check_send_ACK_NACK(ackOk);																// send appropriate answer

	#ifdef SUPPORT_AES
		}
	#endif
}












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





/**
 * @brief Set message type, sender and receiver address
 *        and set snd.active, so the message should send next time
 *
 * @param mCounter the message counter
 * @param mType    the message type
 * @param addrTo   pointer to receiver address
 */
/*void AS::prepareToSend(uint8_t mCounter, uint8_t mType, uint8_t *receiverAddr) {
	uint8_t i;

	snd_msg.mBody.MSG_CNT = mCounter;
	snd_msg.mBody.MSG_TYP = mType;
	snd_msg.mBody.SND_ID[0] = dev_ident.HMID[0];
	snd_msg.mBody.SND_ID[1] = dev_ident.HMID[1];
	snd_msg.mBody.SND_ID[2] = dev_ident.HMID[2];
	memcpy(snd_msg.mBody.RCV_ID, receiverAddr, 3);

	snd_msg.active = MSG_ACTIVE::FORWARD;																				// remember to fire the message
}



void AS::sendINFO_PARAMETER_CHANGE(void) {
	// TODO: make ready

	//"10;p01=04"   => { txt => "INFO_PARAMETER_CHANGE", params => {
	//CHANNEL => "2,2",
	//PEER    => '4,8,$val=CUL_HM_id2Name($val)',
	//PARAM_LIST => "12,2",
	//DATA => '14,,$val =~ s/(..)(..)/ $1:$2/g', } },
	// --------------------------------------------------------------------
}




// - AES Signing related methods -------------------

#ifdef SUPPORT_AES
	/**
	 * @brief Send sign request to receiver
	 *
	 * @param rememberBuffer   set to 1 to force remember the current message for later processing
	 *
	 * Message description:
	 *             Sender__ Receiver SigningRequest Challenge         KeyIndex
	 * 11 24 80 02 1F B7 4A 63 19 63 04             XX XX XX XX XX XX 00
	 *
	 * The Challenge consists 6 random bytes.
	 */
/*	void AS::sendSignRequest(uint8_t rememberBuffer) {
		if (rememberBuffer) {
			memcpy(rcv_msg.prev_buf, rcv_msg.buf, rcv_msg.buf[0]+1);											// remember the message from buffer
			rcv_msg.use_prev_buf = 1;
		}

		snd_msg.mBody.MSG_LEN = 0x11;
		snd_msg.mBody.FLAG.BIDI = (isEmpty(dev_operate.MAID,3)) ? 0 : 1;
		snd_msg.mBody.BY10 = AS_RESPONSE_AES_CHALLANGE;											// AES Challenge

		initPseudoRandomNumberGenerator();

		uint8_t i = 0;
		for (i = 0; i < 6; i++) {																// random bytes to the payload
			snd_msg.buf[11 + i] = (uint8_t)rand();
		}
		snd_msg.buf[17] = dev_ident.HMKEY_INDEX;																// the 7th byte is the key index

		/*
		 * Here we make a temporarily key with the challenge and the HMKEY.
		 * We need this for later signature verification.
		 */
/*		makeTmpKey(snd_msg.buf+11);

		#ifdef AES_DBG
			dbg << F(">>> signingRequestData  : ") << _HEX(snd_msg.buf+10, 7) << F(" <<<") << '\n';
		#endif

		prepareToSend(rcv_msg.mBody.MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody.SND_ID);
	}


#endif

/**
* @brief Initialize the random number generator
*/
/*void AS::initPseudoRandomNumberGenerator() {
	srand(randomSeed ^ uint16_t(millis() & 0xFFFF));
}

/**
* @brief Initialize the pseudo random number generator
*        Take all bytes from uninitialized RAM and xor together
*/
/*inline void AS::initRandomSeed() {
	uint16_t *p = (uint16_t*)(RAMEND + 1);
	extern uint16_t __heap_start;
	while (p >= &__heap_start + 1) {
		randomSeed ^= *(--p);
	}
	initPseudoRandomNumberGenerator();
}*/






	
	
	
//- some helpers ----------------------------------------------------------------------------------------------------------
uint32_t byteTimeCvt(uint8_t tTime) {
	const uint16_t c[8] = { 1,10,50,100,600,3000,6000,36000 };
	return (uint32_t)(tTime & 0x1F) * c[tTime >> 5] * 100;
}

uint32_t intTimeCvt(uint16_t iTime) {
	if (iTime == 0) return 0;

	// take care of the byte order
	#define LIT_ENDIAN ((1 >> 1 == 0) ? 1 : 0)
	#if LIT_ENDIAN
		iTime = (iTime >> 8) | (iTime << 8);
	#endif

	// process the conversation
	uint8_t tByte;
	if ((iTime & 0x1F) != 0) {
		tByte = 2;
		for (uint8_t i = 1; i < (iTime & 0x1F); i++) tByte *= 2;
	} else tByte = 1;
	return (uint32_t)tByte*(iTime >> 5) * 100;
}

uint8_t  isEmpty(void *ptr, uint8_t len) {
	while (len > 0) {
		len--;
		if (*((uint8_t*)ptr + len)) return 0;
	}
	return 1;
}
//- -----------------------------------------------------------------------------------------------------------------------



