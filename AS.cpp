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

//#define AES_DBG




#include "AS.h"
#include <avr/wdt.h>

#ifdef SUPPORT_AES
	#include "aes.h"
	aes128_ctx_t ctx; 																		// the context where the round keys are stored
#endif


s_pair_mode   pair_mode;																	// helper structure for keeping track of active pairing mode
s_config_mode config_mode;																	// helper structure for keeping track of active config mode

s_ee_start    dev_ident;																	// struct to hold the device identification related information									
uint8_t       *MAID;																		// pointer to the master id, which is hold in cmMaintenance

s_recv        rcv_msg;																		// struct to process received strings
s_send        snd_msg;																		// same for send strings

s_config_list_answer_slice config_list_answer_slice;										// defined in AS.h, holds information to answer config list requests for peer or param lists


// public:		//---------------------------------------------------------------------------------------------------------
AS::AS()  {
	DBG_START(AS, F("AS.\n"));																// ...and some information
}

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
	dbg << "fl: magic: " << flashCRC << ", " << dev_ident.MAGIC << '\n';
	//dbg << "ee: magic: " << dev_ident.MAGIC << ", hmid: " << _HEX(dev_ident.HMID, 3) << ", serial: " << _HEX(dev_ident.SERIAL_NR, 10) << ", index: " << _HEXB(dev_ident.HMKEY_INDEX) << ", key: " << _HEX(dev_ident.HMKEY, 16) << ", sizeof: " << sizeof(dev_ident) << ", addr: " << (uint16_t)&dev_ident.MAGIC << '\n';
	DBG(AS, F("AS:init crc- flash:"), flashCRC, F(", eeprom: "), dev_ident.MAGIC, '\n');	// some debug

	if (flashCRC != dev_ident.MAGIC) {	

		/* - First time detected
		* get the homematic id, serial number, aes key index and homematic aes key from flash and copy it into the eeprom
		* order in HMSerialData[]                 * HMID *, * Serial number *, * Default-Key *, * Key-Index *   
		* order in dev_ident struct   *	MAGIC *, * HMID[3] *, * SERIAL_NR[10] *, * HMKEY[16] *, * HMKEY_INDEX *
		* we can copy the complete struct with a 2 byte offset in regards to the magic byte */
		memcpy_P( dev_ident.HMID, HMSerialData, sizeof(dev_ident) - 2);						// copy from PROGMEM
		dev_ident.MAGIC = flashCRC;															// set new magic number

		//dbg << "fl: magic: " << dev_ident.MAGIC << ", hmid: " << _HEX(dev_ident.HMID, 3) << ", serial: " << _HEX(dev_ident.SERIAL_NR, 10) << ", index: " << _HEXB(dev_ident.HMKEY_INDEX) << ", key: " << _HEX(dev_ident.HMKEY, 16) << ", sizeof: " << sizeof(dev_ident) << ", addr: " << (uint16_t)&dev_ident << '\n';
		setEEPromBlock(0, sizeof(dev_ident), &dev_ident.MAGIC);
																									//setEEPromBlock(0, sizeof(dev_ident), &dev_ident);									// store defaults to EEprom
		DBG(AS, F("AS:writing new magic byte\n") );											// some debug


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

		/* - function to be placed in register.h, to setup default values on first time start */
		firstTimeStart();				
	}

	/* - Initialize the hardware. All this functions are defined in HAL.h and HAL_extern.h 	*/
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts
	initMillis();																			// start the millis counter
	
	cc.init();																				// init the rf module

	initRandomSeed();

	/* load list 0 and 1 defaults and inform the channel modules */
	for (uint8_t i = 0; i < cnl_max; i++) {													// step through all channels
		cmMaster *pCM = ptr_CM[i];															// short hand to respective channel master	
		pCM->lstC.load_list();																// read the defaults in respective list0/1
		pCM->info_config_change();															// inform the channel modules
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
		rcv.poll();																				// and poll the receive function to get intent and some basics
	}
	if (rcv_msg.hasdata) processMessage();														// check if we have to handle the receive buffer

	/* handle the send module */
	snd.poll();																					// check if there is something to send

	/* time out the config flag */
	if (config_mode.active) {																	// check only if we are still in config mode
		if (config_mode.timer.done()) config_mode.active = 0;									// when timer is done, set config flag to inactive
	}

	if (resetStatus == AS_RESET || resetStatus == AS_RESET_CLEAR_EEPROM) {
		deviceReset(resetStatus);
	}


	if (stcPeer.active) {
		sendPeerMsg();																			// poll the peer message sender
	}
	

	// time out the pairing timer
	if (pair_mode.active) { 
		if (pair_mode.timer.done()) {
			pair_mode.active = 0;
			isEmpty(MAID, 3)? led.set(pair_err) : led.set(pair_suc);	
		}
	}

	// regular polls
	for (uint8_t i = 0; i < cnl_max; i++) {													// poll the channel modules
		ptr_CM[i]->poll();
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

/**
* @brief Receive handler: Process received messages
* Process message is called while rcv_msg.hasdata is set to TRUE. It takes all information from the rcv_msg struct.
* Sort out by several if's which message we received and delegates the reuqest for answer to the according class/function.
* 
*/
void AS::processMessage(void) {
	cmMaster *pCM;

	if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::DEVICE_INFO)) {
		/* not sure what to do with while received, probably nothing */

	} else if (rcv_msg.mBody.MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) {
		/* config request messages are used to configure a devive by writing registers and peers -
		*  check the channel and forward for processing to the respective function */

		uint8_t by11 = rcv_msg.mBody.BY11;													// short hand to byte 11 in the received string
		pCM = ptr_CM[rcv_msg.mBody.BY10];													// short hand to the respective channel module instance
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
		if      (by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_OFF)) pCM->INSTRUCTION_INHIBIT_OFF(&rcv_msg.m1100xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_ON))  pCM->INSTRUCTION_INHIBIT_ON(&rcv_msg.m1101xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SET))         pCM->INSTRUCTION_SET(&rcv_msg.m1102xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_STOP_CHANGE)) pCM->INSTRUCTION_STOP_CHANGE(&rcv_msg.m1103xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_RESET))       pCM->INSTRUCTION_RESET(&rcv_msg.m1104xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LED))         pCM->INSTRUCTION_LED(&rcv_msg.m1180xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LED_ALL))     pCM->INSTRUCTION_LED_ALL(&rcv_msg.m1181xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_LEVEL))       pCM->INSTRUCTION_LEVEL(&rcv_msg.m1181xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SLEEPMODE))   pCM->INSTRUCTION_SLEEPMODE(&rcv_msg.m1182xx);
		else if (by10 == BY10(MSG_TYPE::INSTRUCTION_SET_TEMP))    pCM->INSTRUCTION_SET_TEMP(&rcv_msg.m1186xx);

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

	} else if (rcv_msg.intent == MSG_INTENT::PEER) {
		/* it is a peer message, which was checked in the receive class, so reload the respective list 3/4 */
		pCM = ptr_CM[rcv_msg.cnl];															// we remembered on the channel by checking validity of peer
		pCM->lstP.load_list(ptr_CM[rcv_msg.cnl]->peerDB.get_idx(rcv_msg.peer));				// load the respective class 3
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

	rcv_msg.clear();
	//return;












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
		if ((snd_msg.active) && (rcv_msg.mBody.MSG_CNT == snd_msg.temp_MSG_CNT) && (rcv_msg.mBody.BY10 != AS_RESPONSE_AES_CHALLANGE)) {
			//snd_msg.retr_cnt = 0xFF;
		}

		if (rcv_msg.mBody.BY10 == AS_RESPONSE_ACK) {
			/*
			* Message description:
			*             Sender__ Receiver ACK
			* 0A 05 80 02 63 19 63 01 02 04 00
			*/
			// nothing to do yet

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
		snd_msg.clear();																		// cleanup send module data;

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
#ifdef AES_DBG
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

		uint8_t pIdx;
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

	//rcv_msg.clear();																	// nothing to do any more
}











/* ------------------------------------------------------------------------------------------------------------------------
* - send functions --------------------------------------------------------------------------------------------------------
* @brief Here are the send functions for the device, this library is focused on client communication,
* so you will find send functions for client communication only. The send functions are sorted by
* message type and sub type. Only for the peer related messages there is the need to define the receiver.
* Configuration and status answers send only to HMID, ACK and subtypes are always the response to a received string
*/

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
void AS::send_DEVICE_INFO(MSG_REASON::E reason) {
	s_m00xxxx *msg = &snd_msg.m00xxxx;														// short hand to send buffer
	uint8_t *rcv_id;																		// pointer to an array address for the answer

	/* copy the payload from different sources */
	memcpy_P(&msg->FIRMWARE, dev_static, 3);												// copy firmware and modelID
	memcpy(&msg->SERIALNO, dev_ident.SERIAL_NR, 10);										// copy the serial number
	memcpy_P(&msg->CLASS, dev_static + 3, 4);												// copy subtype and device info

	/* is it an answer to a CONFIG_PAIR_SERIAL request, or while we initiate the pairing process */
	if (reason == MSG_REASON::ANSWER) {
		msg->MSG_CNT = rcv_msg.mBody.MSG_CNT;												// set the message counter accordingly
		rcv_id = rcv_msg.mBody.SND_ID;														// respond to sender
	}
	else {
		msg->MSG_CNT = snd_msg.MSG_CNT++;
		rcv_id = MAID;																		// we initiated, so it has to go to the master id
	}

	/* BIDI is asked all time, will removed automatically if MAID is empty */
	snd_msg.set_msg(MSG_TYPE::DEVICE_INFO, rcv_id, 1);

	//pair_mode.active = 1;																	// set pairing flag
	//pair_mode.timer.set(20000);															// set pairing time
	led.set(pairing);																		// and visualize the status
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
	snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;											// as it is an answer, we reflect the counter in the answer
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
void AS::send_ACK_STATUS(uint8_t chnl, uint8_t stat, uint8_t actn) {
	if (!rcv_msg.mBody.FLAG.BIDI) return;													// send ACK only while required
	s_m0201xx *msg = &snd_msg.m0201xx;														// short hand to ACK Status struct while easier to fill
	msg->MSG_CNL = chnl;
	msg->MSG_STATUS = stat;
	*(uint8_t*)&msg->MSG_FLAG = actn;
	msg->MSG_FLAG.LOWBAT = bat.getStatus();
	msg->MSG_RSSI = cc.rssi;
	snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;											// as it is an answer, we reflect the counter in the answer
	snd_msg.set_msg(MSG_TYPE::ACK_STATUS, rcv_msg.mBody.SND_ID);
}
void AS::send_ACK2(void) {
}
void AS::send_AES_REQ(s_m0204xx *buf) {
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
	snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;											// as it is an answer, we reflect the counter in the answer
	snd_msg.set_msg(MSG_TYPE::NACK, rcv_msg.mBody.SND_ID);									// length and flags are set within the snd_msg struct
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
	snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;											// as it is an answer, we reflect the counter in the answer
	snd_msg.set_msg(MSG_TYPE::NACK_TARGET_INVALID, rcv_msg.mBody.SND_ID);
}
void AS::send_ACK_NACK_UNKNOWN() {
}

void AS::send_AES_REPLY() {
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

	snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;											// as it is an answer, we reflect the counter in the answer
	snd_msg.set_msg(MSG_TYPE::INFO_SERIAL, rcv_msg.mBody.SND_ID);
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
void AS::send_INFO_PARAM_RESPONSE_PAIRS(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
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
void AS::send_INFO_PARAM_RESPONSE_SEQ(uint8_t cnl, uint8_t lst, uint8_t *peer_id) {
	//config_list_answer_slice.type = LIST_ANSWER::PARAM_RESPONSE_PAIRS;;						// we want to get the param list sequential
	//config_list_answer_slice.list = ptr_CM[cnl]->list[lst];									// pointer to the respective list struct
	//config_list_answer_slice.max_slc = ptr_CM[cnl]->list[lst]->get_nr_slices();				// get an idea of the total needed slices
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
void AS::send_INFO_ACTUATOR_STATUS(uint8_t cnl, uint8_t stat, uint8_t flag) {
	s_m1006xx *msg = &snd_msg.m1006xx;														// struct is easier to fill
	s_mBody *rcvBody = &rcv_msg.mBody;														// short hand to received string
	uint8_t bidi = 0;																		// per default we don't need an ACK

	msg->MSG_CNL = cnl;																		// copy in the channel
	msg->MSG_STAT = stat;																	// the status of the channel
	msg->UNKNOWN = 0;																		// needs investigation
	msg->MSG_RSSI = cc.rssi;																// received rssi value

	if ((rcvBody->MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) && (rcvBody->BY11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST))) {
		snd_msg.mBody.MSG_CNT = rcvBody->MSG_CNT;											// if it is an answer, take the message counter from received message
	} else {
		snd_msg.mBody.MSG_CNT = snd_msg.MSG_CNT++;											// we initiated, take the message counter from the request
		bidi = 1;																			// want to get an ACK
	}
	snd_msg.set_msg(MSG_TYPE::INFO_ACTUATOR_STATUS, MAID, bidi);							// all the time send to the master
}

void AS::send_INFO_TEMP() {
}

void AS::send_HAVE_DATA() {
}

void AS::send_SWITCH(s_peer_table *peerDB) {
}
void AS::send_TIMESTAMP(s_peer_table *peerDB) {
}
void AS::send_REMOTE(s_peer_table *peerDB) {
}
void AS::send_SENSOR_EVENT(s_peer_table *peerDB) {
}
void AS::send_SWITCH_LEVEL(s_peer_table *peerDB) {
}
void AS::send_SENSOR_DATA(s_peer_table *peerDB) {
}
void AS::send_GAS_EVENT(s_peer_table *peerDB) {
}
void AS::send_CLIMATE_EVENT(s_peer_table *peerDB) {
}
void AS::send_SET_TEAM_TEMP(s_peer_table *peerDB) {
}
void AS::send_THERMAL_CONTROL(s_peer_table *peerDB) {
}
void AS::send_POWER_EVENT_CYCLE(s_peer_table *peerDB) {
}
void AS::send_POWER_EVENT(s_peer_table *peerDB) {
}
void AS::send_WEATHER_EVENT(s_peer_table *peerDB) {
}











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
	inline void AS::sendAckAES(uint8_t *data) {
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


	snd_msg.mBody.FLAG.BIDI = (isEmpty(MAID,3))?0:1;
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
	prepareToSend(cnt, AS_MESSAGE_POWER_EVENT_CYCLIC, MAID);
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

void AS::sendSWITCH(void) {
	//TODO: make ready#

	//"3E"          => { txt => "SWITCH"      , params => {
	//DST      => "00,6",
	//UNKNOWN  => "06,2",
	//CHANNEL  => "08,2",
	//COUNTER  => "10,2", } },
	// --------------------------------------------------------------------
}

void AS::sendTimeStamp(void) {
	//TODO: make ready#

	//"3F"          => { txt => "TimeStamp"   , params => {
	//UNKNOWN  => "00,4",
	//TIME     => "04,2", } },
	// --------------------------------------------------------------------
}

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
void AS::sendREMOTE(uint8_t channel, uint8_t *payload, uint8_t msg_flag) {
	// burst flag is not needed, has to come out of list4, as well as AES flag
	sendEvent(channel, 0, AS_MESSAGE_REMOTE_EVENT, payload, 2);
}

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
void AS::sendSensor_event(uint8_t channel, uint8_t burst, uint8_t *payload) {
	sendEvent(channel, AS_MESSAGE_SENSOR_EVENT, burst, payload, 3);
}

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
void AS::sendEvent(uint8_t channel, uint8_t msg_type, uint8_t msg_flag, uint8_t *ptr_payload, uint8_t len_payload) {
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
}

void AS::sendSensorData(void) {
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
}

void AS::sendClimateEvent(void) {
	//TODO: make ready#

	//"58"          => { txt => "ClimateEvent", params => {
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
}

void AS::sendSetTeamTemp(void) {
	//TODO: make ready#

	//"59"          => { txt => "setTeamTemp" , params => {
	//CMD      => "00,2",
	//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
	//mode     => '02,2,$val=(hex($val) & 0x3)',} },
}

void AS::sendWeatherEvent(void) {
	//TODO: make ready#

	//"70"          => { txt => "WeatherEvent", params => {
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
}

// private:		//---------------------------------------------------------------------------------------------------------
// - poll functions --------------------------------


inline void AS::sendPeerMsg(void) {
	cmMaster *pCM = ptr_CM[stcPeer.channel];
	uint8_t retries_max;

	retries_max = (stcPeer.bidi) ? 3 : 1;
	
	if (snd_msg.active) return;																		// check if send function has a free slot, otherwise return
	
	// first run, prepare amount of slots
	if (!stcPeer.idx_max) {
		stcPeer.idx_max = pCM->peerDB.max;														// get amount of messages of peer channel

		if (!pCM->peerDB.used_slots() ) {															// check if at least one peer exist in db, otherwise send to master and stop function
			preparePeerMessage(MAID, retries_max);
			snd_msg.MSG_CNT++;																		// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));										// clean out and return
			return;
		}
	}
	
	// all slots of channel processed, start next round or end processing
	if (stcPeer.idx_cur >= stcPeer.idx_max) {													// check if all peer slots are done
		stcPeer.retries++;																		// increase the round counter
		
		if ((stcPeer.retries >= retries_max) || (isEmpty(stcPeer.slot,8))) {					// all rounds done or all peers reached
			//dbg << "through\n";
			snd_msg.MSG_CNT++;																		// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));										// clean out and return
			
		} else {																				// start next round
			//dbg << "next round\n";
			stcPeer.idx_cur = 0;

		}
		return;

	} else if ((stcPeer.idx_cur) && (!snd_msg.timeout)) {											// peer index is >0, first round done and no timeout
		uint8_t idx = stcPeer.idx_cur -1;
		stcPeer.slot[idx >> 3] &=  ~(1 << (idx & 0x07));										// clear bit, because message got an ACK
	}
	
	// set respective bit to check if ACK was received
	if (!stcPeer.retries) {
		stcPeer.slot[stcPeer.idx_cur >> 3] |= (1<<(stcPeer.idx_cur & 0x07));					// set bit in slt table										// clear bit in slt and increase counter
	}


	// exit while bit is not set
	if (!(stcPeer.slot[stcPeer.idx_cur >> 3] & (1<<(stcPeer.idx_cur & 0x07)))) {
		stcPeer.idx_cur++;																		// increase counter for next time
		return;
	}

	uint8_t *tmp_peer = ptr_CM[stcPeer.channel]->peerDB.get_peer(stcPeer.idx_cur);
	
	#ifdef AS_DBG
		dbg << "a: " << stcPeer.idx_cur << " m " << stcPeer.idx_max << '\n';
	#endif

	if (isEmpty(tmp_peer,4)) {																	// if peer is 0, set done bit in slt and skip
		stcPeer.slot[stcPeer.idx_cur >> 3] &=  ~(1<<(stcPeer.idx_cur & 0x07));					// remember empty peer in slot table										// clear bit in slt and increase counter
		stcPeer.idx_cur++;																		// increase counter for next time
		return;																					// wait for next round
	}

	// if we are here, there is something to send
	//dbg << "cnl:" << stcPeer.channel << " cIdx:" << stcPeer.idx_cur << " mIdx:" << stcPeer.idx_max << " slt:" << _HEX(stcPeer.slot,8) << '\n';
	
	// get the respective list4 entries and take care while sending the message
	// peerNeedsBurst  =>{a=>  1.0,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"peer expects burst",lit=>{off=>0,on=>1}},
	// expectAES       =>{a=>  1.7,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"expect AES"        ,lit=>{off=>0,on=>1}},
	ptr_CM[stcPeer.channel]->list[4]->load_list(stcPeer.idx_cur);
	l4_0x01.ui = *ptr_CM[stcPeer.channel]->list[4]->ptr_to_val(0x01);
	//l4_0x01.ui = ee_list.getRegAddr(stcPeer.channel, 4, stcPeer.idx_cur, 0x01);
	// fillLvlUpThr    =>{a=>  4.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level upper threshold"},
	// fillLvlLoThr    =>{a=>  5.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level lower threshold"},
	//dbg << F("s_l4_0x01=") << _HEXB(l4_0x01.ui) << F("\n");
	//l4_0x01.ui = 0;		// disable burst - hardcoded
	
	preparePeerMessage(tmp_peer, 1);
	
	if (!snd_msg.mBody.FLAG.BIDI) {
		stcPeer.slot[stcPeer.idx_cur >> 3] &=  ~(1<<(stcPeer.idx_cur & 0x07));					// clear bit, because it is a message without need to be repeated
	}

	stcPeer.idx_cur++;																			// increase counter for next time
}

void AS::preparePeerMessage(uint8_t *xPeer, uint8_t retries) {

	// description --------------------------------------------------------
	//    len  cnt  flg  typ  reID      toID      pl
	// l> 0B   0A   A4   40   23 70 EC  1E 7A AD  02 01
	// description --------------------------------------------------------
	//                        reID      toID      BLL  Cnt  Val
	// l> 0C   0A   A4   41   23 70 EC  1E 7A AD  02   01   200
	// do something with the information ----------------------------------
	//"41"          => { txt => "Sensor_event", params => {
	// BUTTON = bit 0 - 5
	// LONG   = bit 6
	// LOWBAT = bit 7

	snd_msg.mBody.MSG_LEN = stcPeer.len_payload + 9;												// set message length
	snd_msg.mBody.FLAG.CFG   = 1;
	snd_msg.mBody.FLAG.BIDI  = stcPeer.bidi;															// message flag
	snd_msg.mBody.FLAG.BURST = l4_0x01.s.peerNeedsBurst;
	
	prepareToSend(snd_msg.MSG_CNT, stcPeer.msg_type, xPeer);

	if (snd_msg.mBody.MSG_TYP == 0x41) {
		snd_msg.mBody.BY10 = stcPeer.channel;
		snd_msg.mBody.BY10 |= (bat.getStatus() << 7);													// battery bit
		memcpy(snd_msg.buf+11, stcPeer.ptr_payload, stcPeer.len_payload);							// payload
		snd_msg.mBody.MSG_LEN++;
	} else {
		memcpy(snd_msg.buf+10, stcPeer.ptr_payload, stcPeer.len_payload);							// payload
	}
	snd_msg.max_retr = retries;																		// send only one time
}



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

#ifdef SUPPORT_AES
	/*
	 * @brief Loop thru channels and check if AES is activated for any channel.
	 */
	uint8_t AS::checkAnyChannelForAES(void) {
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
	inline void AS::processMessageKeyExchange(void) {
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
	inline void AS::processMessageResponseAES_Challenge(void) {
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
void AS::deviceReset(uint8_t clearEeprom) {
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
void AS::prepareToSend(uint8_t mCounter, uint8_t mType, uint8_t *receiverAddr) {
	uint8_t i;

	snd_msg.mBody.MSG_CNT = mCounter;
	snd_msg.mBody.MSG_TYP = mType;
	snd_msg.mBody.SND_ID[0] = dev_ident.HMID[0];
	snd_msg.mBody.SND_ID[1] = dev_ident.HMID[1];
	snd_msg.mBody.SND_ID[2] = dev_ident.HMID[2];
	memcpy(snd_msg.mBody.RCV_ID, receiverAddr, 3);

	snd_msg.active = 1;																				// remember to fire the message
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
	void AS::sendSignRequest(uint8_t rememberBuffer) {
		if (rememberBuffer) {
			memcpy(rcv_msg.prev_buf, rcv_msg.buf, rcv_msg.buf[0]+1);											// remember the message from buffer
			rcv_msg.use_prev_buf = 1;
		}

		snd_msg.mBody.MSG_LEN = 0x11;
		snd_msg.mBody.FLAG.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
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
		makeTmpKey(snd_msg.buf+11);

		#ifdef AES_DBG
			dbg << F(">>> signingRequestData  : ") << _HEX(snd_msg.buf+10, 7) << F(" <<<") << '\n';
		#endif

		prepareToSend(rcv_msg.mBody.MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody.SND_ID);
	}

	/**
	 * @brief Make a temporarily key for encrypting the sign response.
	 *        The temporarily key was built by XORing the key with the challenge
	 *
	 * @param challenge   pointer to the challenge
	 */
	void AS::makeTmpKey(uint8_t *challenge) {
		for (uint8_t i = 0; i < 16; i++) {
			tempHmKey[i] = (i<6) ? (dev_ident.HMKEY[i] ^ challenge[i]) : dev_ident.HMKEY[i];
		}

		aes128_init(tempHmKey, &ctx);														// generating the round keys from the 128 bit key
	}

#endif


// - some helpers ----------------------------------
// public:		//---------------------------------------------------------------------------------------------------------
//- some helpers ----------------------------------------------------------------------------------------------------------


inline uint8_t  isEmpty(void *ptr, uint8_t len) {
	while (len > 0) {
		len--;
		if (*((uint8_t*)ptr + len)) return 0;
	}
	return 1;
}
//- -----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize the random number generator
 */
void AS::initPseudoRandomNumberGenerator() {
	srand(randomSeed ^ uint16_t (millis() & 0xFFFF));
}

/**
 * @brief Initialize the pseudo random number generator
 *        Take all bytes from uninitialized RAM and xor together
 */
inline void AS::initRandomSeed() {
	uint16_t *p = (uint16_t*) (RAMEND + 1);
	extern uint16_t __heap_start;
	while (p >= &__heap_start + 1) {
		randomSeed ^= * (--p);
	}

	initPseudoRandomNumberGenerator();
}




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

	return (uint32_t)tByte*(iTime>>5)*100;
}
