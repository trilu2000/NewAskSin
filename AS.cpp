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

s_recv rcv_msg;																				// struct to process received strings
s_send snd_msg;																				// same for send strings


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
	getEEPromBlock(0, sizeof(dev_ident), &dev_ident);						// get magic byte and all other information from eeprom
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
		* and written to the eeprom. nothing to do any more, while defaults are in the value arrays 
		* with the same information as in the eeprom */
		for (uint8_t i = 0; i < cnl_max; i++) {												// write the defaults in respective list0/1
			cmMaster *pCM = pcnlModule[i];													// short hand to respective channel master	
			pCM->lstC.load_default();														// copy from progmem into array
			pCM->lstC.save_list();															// write it into the eeprom
			pCM->peerDB.clear_all();
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
	//cc.init();																			// init the rf module

	initRandomSeed();

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

	if (resetStatus == AS_RESET || resetStatus == AS_RESET_CLEAR_EEPROM) {
		deviceReset(resetStatus);
	}

	// handle the slice send functions
	if (stcSlice.active) {
		sendSliceList();																		// poll the slice list send function
	}

	if (stcPeer.active) {
		sendPeerMsg();																			// poll the peer message sender
	}
	
	// time out the config flag
	if (config_mode.active) {																	// check only if we are still in config mode
		if ( config_mode.timer.done() ) config_mode.active = 0;									// when timer is done, set config flag to inactive
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
		pcnlModule[i]->poll();
	}

	btn.poll();																			// poll the config button
	led.poll();																					// poll the led's
	bat.poll();																					// poll the battery check
		
	// check if we could go to standby
	pom.poll();																					// poll the power management
}


/**
* @brief Receive handler: Process received messages
* Process message is called while rcv_msg.hasdata is set to TRUE. It takes all information from the rcv_msg struct.
* Sort out by several if's which message we received and delegates the reuqest for answer to the according class/function.
* 
*/
void AS::processMessage(void) {
	/* first we sort out the message on base of the type in byte 03 */
	if        (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::DEVICE_INFO)) {
		/* not sure what to do with while received */

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::CONFIG_REQ)) {
		/* config request messages are used to configure a devive by writing registers and peers -
		*  find the right channel and forward processing to cmMaster.
		*  check upfront the channel in byte 10 if it is out of range */

		uint8_t cnl = rcv_msg.mBody->BY10;													// shorthand to channel information
		if (cnl >= cnl_max) {																// check if channel is in the range
			rcv_msg.clear();																// nothing to do any more
			DBG(AS, F("channel out of range "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
			return;																			// no further processing needed
		}

		cmMaster *pCM = pcnlModule[cnl];													// short hand to respective channel module
		uint8_t by11 = rcv_msg.mBody->BY11;													// short hand to byte 11 in the received string

		if      (by11 == BY11(MSG_TYPE::CONFIG_PEER_ADD))       pCM->CONFIG_PEER_ADD(     (s_m01xx01*)rcv_msg.buf );
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_REMOVE))    pCM->CONFIG_PEER_REMOVE(  (s_m01xx02*)rcv_msg.buf );
		else if (by11 == BY11(MSG_TYPE::CONFIG_PEER_LIST_REQ))  pCM->CONFIG_PEER_LIST_REQ( );
		else if (by11 == BY11(MSG_TYPE::CONFIG_PARAM_REQ))      pCM->CONFIG_PARAM_REQ(     );
		else if (by11 == BY11(MSG_TYPE::CONFIG_START))          pCM->CONFIG_START(         );
		else if (by11 == BY11(MSG_TYPE::CONFIG_END))            pCM->CONFIG_END(           );
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX1))   pCM->CONFIG_WRITE_INDEX1(  );
		else if (by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX2))   pCM->CONFIG_WRITE_INDEX2(  );
		else if (by11 == BY11(MSG_TYPE::CONFIG_SERIAL_REQ))     pCM->CONFIG_SERIAL_REQ(    );
		else if (by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL))    pCM->CONFIG_PAIR_SERIAL(   );
		else if (by11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST)) pCM->CONFIG_STATUS_REQUEST();
		else {
			dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';
			DBG(AS, F("AS:message not known - please report: "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
		}

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::ACK_MSG)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::AES_REPLY)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SEND_AES)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::REPLY_MSG)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::INSTRUCTION_MSG)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::HAVE_DATA)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SWITCH)) {

	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::TIMESTAMP)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::REMOTE)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SENSOR_EVENT)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SWITCH_LEVEL)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SENSOR_DATA)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::GAS_EVENT)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::CLIMATE_EVENT)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::SET_TEAM_TEMP)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::THERMAL_CONTROL)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::POWER_EVENT_CYCLE)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::POWER_EVENT)) {
	} else if (rcv_msg.mBody->MSG_TYP == BY03(MSG_TYPE::WEATHER_EVENT)) {
	} else {
		dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';
		DBG(AS, F("AS:message not known - please report: "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), '\n');
	}





	uint8_t by10 = rcv_msg.mBody->BY10 - 1;

	// check which type of message was received
	if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_DEVINFO) {
		//TODO: do something with the information

	}
	else if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_CONFIG) {

		if (rcv_msg.mBody->BY11 == AS_CONFIG_PEER_LIST_REQ) {
			processMessageConfigPeerListReq();

		}
		else if (rcv_msg.mBody->BY11 == AS_CONFIG_PARAM_REQ) {
			processMessageConfigParamReq();

		}
		else if (rcv_msg.mBody->BY11 == AS_CONFIG_SERIAL_REQ) {
			processMessageConfigSerialReq();

		}
		else if (rcv_msg.mBody->BY11 == AS_CONFIG_PAIR_SERIAL) {
			processMessageConfigPairSerial();

		}
		else if (rcv_msg.mBody->BY11 == AS_CONFIG_STATUS_REQUEST) {
			processMessageConfigStatusRequest(by10);

		}
		else {
			processMessageConfigAESProtected();
		}

	}
	else if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_RESPONSE) {
		/*
		* This is an response (ACK) to an active message.
		* In exception of AS_RESPONSE_AES_CHALLANGE message, we set retrCnt to 0xFF
		*/
		if ((snd_msg.active) && (rcv_msg.mBody->MSG_CNT == snd_msg.temp_MSG_CNT) && (rcv_msg.mBody->BY10 != AS_RESPONSE_AES_CHALLANGE)) {
			snd_msg.retr_cnt = 0xFF;
		}

		if (rcv_msg.mBody->BY10 == AS_RESPONSE_ACK) {
			/*
			* Message description:
			*             Sender__ Receiver ACK
			* 0A 05 80 02 63 19 63 01 02 04 00
			*/
			// nothing to do yet

		}
		else if (rcv_msg.mBody->BY10 == AS_RESPONSE_ACK_STATUS) {
			/*
			* Message description:
			*             Sender__ Receiver ACK Channel State Action RSSI
			* 0E 08 80 02 1F B7 4A 23 70 D8 01  01      C8    80     27
			*
			* Action: Down=0x20, UP=0x10, LowBat=&0x80
			*/
			// nothing to do yet

		}
		else if (rcv_msg.mBody->BY10 == AS_RESPONSE_ACK2) {
			// nothing to do yet

#ifdef SUPPORT_AES
		}
		else if (rcv_msg.mBody->BY10 == AS_RESPONSE_AES_CHALLANGE) {
			processMessageResponseAES_Challenge();

			memcpy(snd_msg.buf + 10, snd_msg.prev_buf, 16);
			prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE_AES, rcv_msg.mBody->SND_ID);
#endif

		}
		else if (rcv_msg.mBody->BY10 == AS_RESPONSE_NACK) {
			// nothing to do yet

		}
		else if (rcv_msg.mBody->BY10 == AS_RESPONSE_NACK_TARGET_INVALID) {
			// nothing to do yet

		}

#ifdef SUPPORT_AES
	}
	else if ((rcv_msg.mBody->MSG_TYP == AS_MESSAGE_RESPONSE_AES)) {
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

				if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_CONFIG) {
					processMessageConfig();

				}
				else if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_ACTION) {
					processMessageAction11();

				}
				else if (rcv_msg.mBody->MSG_TYP >= AS_MESSAGE_SWITCH_EVENT) {
					uint8_t pIdx;
					uint8_t cnl = getChannelFromPeerDB(&pIdx);
					if (cnl > 0) {
						processMessageAction3E(cnl, pIdx);
					}
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
			sendNACK();
		}

	}
	else if ((rcv_msg.mBody->MSG_TYP == AS_MESSAGE_KEY_EXCHANGE)) {									// AES Key Exchange
		processMessageKeyExchange();

#endif

	}
	else if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_ACTION) {												// action message
#ifdef SUPPORT_AES

		uint8_t aesActiveForReset = 0;
		if (rcv_msg.mBody->BY10 == AS_ACTION_RESET && rcv_msg.mBody->BY11 == 0x00) {						// device reset requested
			aesActiveForReset = checkAnyChannelForAES();									// check if AES activated for any channel			}
		}

		// check if AES for the current channel active or aesActiveForReset @see above
		if (ee_list.getRegAddr(rcv_msg.mBody->BY11, 1, 0, AS_REG_L1_AES_ACTIVE) == 1 || aesActiveForReset == 1) {
			sendSignRequest(1);

		}
		else {
#endif

			processMessageAction11();
			if (rcv_msg.mBody->FLAG.BIDI || resetStatus == AS_RESET) {
				if (resetStatus == AS_RESET) {   //(ee.getRegListIdx(1, 3) == 0xFF || resetStatus == AS_RESET) {
					sendACK();
				}
				else {
					uint8_t channel = rcv_msg.mBody->BY11;
					if (rcv_msg.mBody->BY10 == AS_ACTION_RESET && rcv_msg.mBody->BY11 == 0x00) {
						channel = 1;
					}
					sendACK_STATUS(channel, 0, 0);
				}
			}

#ifdef SUPPORT_AES
		}
#endif

	}
	else if (rcv_msg.mBody->MSG_TYP == AS_MESSAGE_HAVE_DATA) {											// HAVE_DATA
																										// TODO: Make ready

	}
	else if (rcv_msg.mBody->MSG_TYP >= AS_MESSAGE_SWITCH_EVENT) {
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
		uint8_t cnl = getChannelFromPeerDB(&pIdx);

		//dbg << "cnl: " << cnl << " pIdx: " << pIdx << " mTyp: " << _HEXB(rcv.mBdy.mTyp) << " by10: " << _HEXB(rcv.mBdy.by10)  << " by11: " << _HEXB(rcv.mBdy.by11) << " data: " << _HEX((rcv_msg.buf+10),(rcv.mBdy.mLen-9)) << '\n'; _delay_ms(100);

		if (cnl > 0) {
#ifdef SUPPORT_AES
			// check if AES for the current channel active
			if (ee_list.getRegAddr(cnl, 1, 0, AS_REG_L1_AES_ACTIVE) == 1) {
				sendSignRequest(1);

			}
			else {
#endif

				processMessageAction3E(cnl, pIdx);

#ifdef SUPPORT_AES
			}
#endif
		}

	}

	rcv_msg.clear();																	// nothing to do any more
}



/**
 * @brief Send device info
 *
 * Message description:
 *             Sender__ Receiver fwVer type   Serial number                     class  pCnlA  pCnlB  unknown
 * 1A 94 84 00 1F B7 4A 01 02 04 15    00 6C  4B 45 51 30 32 33 37 33 39 36  10 41     01     00
 */
void AS::sendDEVICE_INFO(void) {
	uint8_t msgCount;
	if ((rcv_msg.mBody->FLAG.CFG == AS_MESSAGE_CONFIG) && (rcv_msg.mBody->BY11 == AS_CONFIG_PAIR_SERIAL)) {
		msgCount = rcv_msg.mBody->MSG_CNT;															// send counter - is it an answer or a initial message
	} else {
		msgCount = snd_msg.MSG_CNT++;
	}

	snd_msg.mBody->MSG_LEN = 0x1A;
	//snd_msg.mBody->FLAG.CFG = 1;
	snd_msg.mBody->FLAG.BIDI = (isEmpty(MAID,3)) ? 0 : 1;

	memcpy_P(snd_msg.buf+10, dev_static, 3);
	memcpy(snd_msg.buf+13, dev_ident.SERIAL_NR, 10);
	memcpy_P(snd_msg.buf+23, dev_static +3, 4);

	prepareToSend(msgCount, AS_MESSAGE_DEVINFO, MAID);

	pair_mode.active = 1;																		// set pairing flag
	pair_mode.timer.set(20000);															// set pairing time
	led.set(pairing);																			// and visualize the status
}

/**
 * @brief Check if ACK required and send ACK or NACK
 */
void AS::checkSendACK(uint8_t ackOk) {
	if (rcv_msg.mBody->FLAG.BIDI) {
		if (ackOk) {
			sendACK();
		} else {
			sendNACK();
		}
	}
}

/**
 * @brief Send ACK message
 *
 * Message description:
 *             Sender__ Receiver ACK
 * 0A 24 80 02 1F B7 4A 63 19 63 00
 */
void AS::sendACK(void) {
	if (rcv_msg.mBody->FLAG.BIDI) {																// prevent answer for requests from a user class on repeated key press
		snd_msg.mBody->MSG_LEN = 0x0A;
		snd_msg.mBody->FLAG.CFG = 0;
		snd_msg.mBody->FLAG.BIDI = 0;
		snd_msg.mBody->BY10 = 0x00;
		prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
	}
}

/**
 * @brief Send a NACK (not ACK)
 *
 * Message description:
 *             Sender__ Receiver NACK
 * 0A 24 80 02 1F B7 4A 63 19 63 80
 */
inline void AS::sendNACK(void) {
	snd_msg.mBody->MSG_LEN = 0x0A;
	snd_msg.mBody->BY10 = AS_RESPONSE_NACK;
	prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
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
		snd_msg.mBody->MSG_LEN = 0x0E;
		snd_msg.mBody->FLAG.BIDI = 0;
		snd_msg.mBody->BY10 = AS_RESPONSE_ACK;
		snd_msg.mBody->BY11 = data[0];
		memcpy(snd_msg.mBody->PAYLOAD, data+1, 3);

		prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
	}
#endif

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
 * @param channel
 * @param state
 * @param action
 */
void AS::sendACK_STATUS(uint8_t channel, uint8_t state, uint8_t action) {
	if (rcv_msg.mBody->FLAG.BIDI) {																	// prevent answer for requests from a user class on repeated key press
		snd_msg.mBody->MSG_LEN = 0x0E;
		snd_msg.mBody->FLAG.BIDI = 0;
		snd_msg.mBody->BY10 = 0x01;
		snd_msg.mBody->BY11 = channel;
		snd_msg.mBody->PAYLOAD[0]   = state;
		snd_msg.mBody->PAYLOAD[1]   = action | (bat.getStatus() << 7);
		snd_msg.mBody->PAYLOAD[2] = cc.rssi;
		//snd_msg.mBody->PAYLOAD[2] = cc.rssi;
		prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
	}
}

/**
 * @brief Send a NACK (not ACK and target invalid)
 *
 * TODO: remove? don't used yet
 *
 * Message description:
 *             Sender__ Receiver NACK_TAGRET_INVALID
 * 0A 24 80 02 1F B7 4A 63 19 63 84
 */
void AS::sendNACK_TARGET_INVALID(void) {
	snd_msg.mBody->MSG_LEN = 0x0A;
	snd_msg.mBody->BY10 = AS_RESPONSE_NACK_TARGET_INVALID;
	prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
}

/**
 * @brief Send info about an actor status
 *
 * Message description:
               Sender__ Receiver    Cnl Stat flag RSSI
 * 0E 40 A4 10 1F B7 4A 63 19 63 06 01   00  00   48 (148679)
 *
 * @param channel
 * @param state
 * @param flag: TODO: to be specified
 */
void AS::sendINFO_ACTUATOR_STATUS(uint8_t channel, uint8_t state, uint8_t flag) {
	snd_msg.mBody->MSG_LEN = 0x0E;
	uint8_t cnt = snd_msg.MSG_CNT++;

	if ((rcv_msg.mBody->MSG_TYP == AS_MESSAGE_CONFIG) && (rcv_msg.mBody->BY11 == AS_CONFIG_STATUS_REQUEST)) {
		cnt = rcv_msg.mBody->MSG_CNT;
	}

	snd_msg.mBody->FLAG.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
	snd_msg.mBody->BY10 = AS_INFO_ACTUATOR_STATUS;
	snd_msg.mBody->BY11 = channel;
	snd_msg.mBody->PAYLOAD[0]   = state;
	snd_msg.mBody->PAYLOAD[1]   = flag; // | (bt.getStatus() << 7);
	snd_msg.mBody->PAYLOAD[2]   = cc.rssi;

	prepareToSend(cnt, AS_MESSAGE_INFO, MAID);
}

void AS::sendINFO_POWER_EVENT(uint8_t *data) {
	snd_msg.mBody->MSG_LEN = 15; // 15, 16 works somehow but 12 + 6 = 18
	uint8_t cnt;

	if ((rcv_msg.mBody->MSG_TYP == AS_MESSAGE_CONFIG) && (rcv_msg.mBody->BY11 == AS_CONFIG_STATUS_REQUEST)) {
		cnt = rcv_msg.mBody->MSG_CNT;
	} else {
		cnt = snd_msg.MSG_CNT++;
	}
	#ifdef AS_DBG
		Serial << F("sendINFO_POWER_EVENT cnt: ");
		Serial.print(cnt, DEC);
	#endif

	//char* myBytes = reinterpret_cast<char*>(snd.mBdy.pyLd);


	snd_msg.mBody->FLAG.BIDI = (isEmpty(MAID,3))?0:1;
	//snd.mBdy.by10      = AS_MESSAGE_POWER_EVENT_CYCLIC;

	// set payload
	snd_msg.mBody->BY10 = data[0];
	snd_msg.mBody->BY11 = data[1]; // first byte of payload
	for (uint8_t i = 2; i < 6; i++){
		//dbg << "AS::sendINFO_POWER_EVENT BYTES: ("<< i <<" " << _HEXB(myBytes[i]) << " = " <<_HEXB(snd.mBdy.pyLd[i]) << "\n";
		snd_msg.mBody->PAYLOAD[i-2] = data[i];
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
inline void AS::sendSliceList(void) {
	uint8_t cnt;

	if (snd_msg.active) return;																	// check if send function has a free slot, otherwise return

	if        (stcSlice.peer) {			// INFO_PEER_LIST
		s_peer_table *peerDB = &pcnlModule[stcSlice.cnl]->peerDB;								// short hand to peer table
		cnt = peerDB->get_slice(stcSlice.curSlc, snd_msg.buf + 11);								// get the slice and the amount of bytes
		sendINFO_PEER_LIST(cnt);																// create the body
		stcSlice.curSlc++;																		// increase slice counter
		//dbg << "peer slc: " << _HEX(snd_msg.buf,snd_msg.buf[0]+1) << '\n';								// write to send buffer

	} else if (stcSlice.reg2) {			// INFO_PARAM_RESPONSE_PAIRS
		s_list_table *list = pcnlModule[stcSlice.cnl]->list[stcSlice.lst];						// short hand to respective list table
		if (stcSlice.curSlc + 1 == stcSlice.totSlc) {											// last slice, terminating message
			memset(snd_msg.buf + 11, 0, 2);
			cnt = 2;
		} else {
			cnt = list->get_slice_pairs(stcSlice.idx, stcSlice.curSlc, snd_msg.buf + 11);		// get the slice and the amount of bytes
		}
		//dbg << "cnt: " << cnt << '\n';
		sendINFO_PARAM_RESPONSE_PAIRS(cnt);
		stcSlice.curSlc++;																		// increase slice counter
		//dbg << "reg2 slc: " << _HEX(snd_msg.buf,snd_msg.buf[0]+1) << '\n';					// write to send buffer
		
	} else if (stcSlice.reg3) {																	// INFO_PARAM_RESPONSE_SEQ

	}

	if (stcSlice.curSlc == stcSlice.totSlc) {													// if everything is send, we could empty the struct
		memset((void*)&stcSlice, 0, 10);														// by memset
		//dbg << "end: " << stcSlice.active << stcSlice.peer << stcSlice.reg2 << stcSlice.reg3 << '\n';
	}
}

inline void AS::sendPeerMsg(void) {
	cmMaster *pCM = pcnlModule[stcPeer.channel];
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

	//uint8_t *tmp_peer;																		// get the respective peer address
	uint8_t *tmp_peer = pcnlModule[stcPeer.channel]->peerDB.get_peer(stcPeer.idx_cur);
	//ee_peer.getPeerByIdx(stcPeer.channel, stcPeer.idx_cur, tmp_peer);
	
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
	l4_0x01.ui = ee_list.getRegAddr(stcPeer.channel, 4, stcPeer.idx_cur, 0x01);
	// fillLvlUpThr    =>{a=>  4.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level upper threshold"},
	// fillLvlLoThr    =>{a=>  5.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level lower threshold"},
	//dbg << F("s_l4_0x01=") << _HEXB(l4_0x01.ui) << F("\n");
	//l4_0x01.ui = 0;		// disable burst - hardcoded
	
	preparePeerMessage(tmp_peer, 1);
	
	if (!snd_msg.mBody->FLAG.BIDI) {
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

	snd_msg.mBody->MSG_LEN = stcPeer.len_payload + 9;												// set message length
	snd_msg.mBody->FLAG.CFG   = 1;
	snd_msg.mBody->FLAG.BIDI  = stcPeer.bidi;															// message flag
	snd_msg.mBody->FLAG.BURST = l4_0x01.s.peerNeedsBurst;
	
	prepareToSend(snd_msg.MSG_CNT, stcPeer.msg_type, xPeer);

	if (snd_msg.mBody->MSG_TYP == 0x41) {
		snd_msg.mBody->BY10 = stcPeer.channel;
		snd_msg.mBody->BY10 |= (bat.getStatus() << 7);													// battery bit
		memcpy(snd_msg.buf+11, stcPeer.ptr_payload, stcPeer.len_payload);							// payload
		snd_msg.mBody->MSG_LEN++;
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
uint8_t AS::getChannelFromPeerDB(uint8_t *pIdx) {
	uint8_t cnl = 0;
	uint8_t tmp;

	// check if we have the peer in the database to get the channel
	if ((rcv_msg.mBody->MSG_TYP == AS_MESSAGE_SWITCH_EVENT) && (rcv_msg.mBody->MSG_LEN == 0x0F)) {
		tmp = rcv_msg.buf[13];																		// save byte13, because we will replace it
		rcv_msg.buf[13] = rcv_msg.buf[14];																// copy the channel byte to the peer
		cnl = is_peer_valid(rcv_msg.buf+10);													// check with the right part of the string
		if (cnl != 0xff) {
			*pIdx = pcnlModule[cnl]->peerDB.get_idx(rcv_msg.buf + 10);										// get the index of the respective peer in the channel store
		}
		rcv_msg.buf[13] = tmp;																		// get it back

	} else {
		cnl = is_peer_valid(rcv_msg.peer);
		if (cnl != 0xff) {
			*pIdx = pcnlModule[cnl]->peerDB.get_idx(rcv_msg.peer);										// get the index of the respective peer in the channel store
		}
	}

	return cnl;
}

#ifdef SUPPORT_AES
	/*
	 * @brief Loop thru channels and check if AES is activated for any channel.
	 */
	uint8_t AS::checkAnyChannelForAES(void) {
		uint8_t i;
		for (i = 1; i < cnl_max; i++) {														// check if AES activated for any channel
			if (ee_list.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
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
		snd_msg.mBody->MSG_LEN = 0x19;
	}
#endif

/*
 * @brief Process message CONFIG_PAIR_SERIAL.
 *
 * Message description:
 *             Sender__ Receiver Channel
 * 15 93 B4 01 63 19 63 00 00 00 01      0E
 */
inline void AS::processMessageConfigStatusRequest(uint8_t by10) {
	// check if a module is registered and send the information, otherwise report an empty status
	//RG::s_modTable *pModTbl = &modTbl[by10];													// pointer to the respective line in the module table

	//if (pModTbl->isActive) {
	//		pModTbl->mDlgt(rcv.mBdy.mTyp, rcv.mBdy.by10, rcv.mBdy.by11, rcv.mBdy.pyLd, rcv.mBdy.mLen - 11);
	//} else {
	//	sendINFO_ACTUATOR_STATUS(rcv.mBdy.by10, 0, 0);
	//}
	pcnlModule[by10]->request_pair_status();

}

/*
 * @brief Process message CONFIG_STATUS_REQUEST.
 *
 * Message description:
 *             Sender__ Receiver       SerialNumber
 * 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
 */
inline void AS::processMessageConfigPairSerial(void) {
	if (isEqual(rcv_msg.buf+12, dev_ident.SERIAL_NR, 10)) {															// compare serial and send device info
		sendDEVICE_INFO();
	}
}

/*
 * @brief Process message CONFIG_SERIAL_REQ.
 *
 * Message description:
 *             Sender__ Receiver
 * 0B 77 A0 01 63 19 63 01 02 04 00 09
 */
inline void AS::processMessageConfigSerialReq(void) {
	sendINFO_SERIAL();

}

/*
 * @brief Process message CONFIG_PARAM_REQ.
 *
 * Message description:
 *             Sender__ Receiver     Channel PeerID__  PeerChannel  ParmList
 * 10 04 A0 01 63 19 63 01 02 04 01  04      00 00 00  00           01
 */
inline void AS::processMessageConfigParamReq(void) {
	/* check if we have the required list in the channel, channel check was already done in processMessage */
	if ((rcv_msg.buf[16] == 0x03) || (rcv_msg.buf[16] == 0x04)) {										// only list 3 and list 4 needs an peer id and idx
		stcSlice.idx = pcnlModule[rcv_msg.mBody->BY10]->peerDB.get_idx(rcv_msg.buf + 12);					// get peer index
		stcSlice.totSlc = pcnlModule[rcv_msg.mBody->BY10]->lstP.get_nr_slices_pairs();					// how many slices are need
	} else {
		stcSlice.idx = 0;																		// otherwise peer index is 0
		stcSlice.totSlc = pcnlModule[rcv_msg.mBody->BY10]->lstC.get_nr_slices_pairs();;					// how many slices are need
	}
	stcSlice.totSlc++;																			// increase by one, while we have to terminate the message flow with a seperate message

	//stcSlice.totSlc = ee_list.countRegListSlc(rcv_msg.mBody->BY10, rcv_msg.buf[16]);					// how many slices are need
	stcSlice.mCnt = rcv_msg.mBody->MSG_CNT;														// remember the message count
	memcpy(stcSlice.toID, rcv_msg.mBody->SND_ID, 3);
	stcSlice.cnl = rcv_msg.mBody->BY10;															// send input to the send peer function
	stcSlice.lst = rcv_msg.buf[16];																	// send input to the send peer function
	stcSlice.reg2 = 1;																			// set the type of answer

	#ifdef AS_DBG
		dbg << "cnl: " << rcv_msg.mBody->BY10 << " s: " << stcSlice.idx << '\n';
		dbg << "totSlc: " << stcSlice.totSlc << '\n';
	#endif

	if ((stcSlice.idx != 0xFF) && (stcSlice.totSlc > 0)) {
		stcSlice.active = 1;																	// only send register content if something is to send															// start the send function
	} else {
		memset((void*)&stcSlice, 0, 10);														// otherwise empty variable
	}
}

/*
 * @brief Process message CONFIG_PEER_LIST_REQ.
 *
 * Message description:
 *             Sender__ Receiver    Channel
 * 0C 0A A4 01 23 70 EC 1E 7A AD 02 01
 */
inline void AS::processMessageConfigPeerListReq(void) {
	cmMaster *pCM = pcnlModule[rcv_msg.mBody->BY10];
	stcSlice.totSlc = pCM->peerDB.get_nr_slices(4);												// how many slices are needed
	//stcSlice.totSlc = ee_peer.countPeerSlc(rcv_msg.mBody->BY10);									// how many slices are need
	stcSlice.mCnt = rcv_msg.mBody->MSG_CNT;														// remember the message count
	memcpy(stcSlice.toID, rcv_msg.mBody->SND_ID, 3);
	stcSlice.cnl = rcv_msg.mBody->BY10;															// send input to the send peer function
	stcSlice.peer = 1;																			// set the type of answer
	stcSlice.active = 1;																		// start the send function
	// answer will send from sendsList(void)
}

inline void AS::processMessageConfigAESProtected() {
	#ifdef SUPPORT_AES
		uint8_t aesActive = checkAnyChannelForAES();											// check if AES activated for any channel
		if (aesActive == 1) {
			sendSignRequest(1);

		} else {
	#endif
			uint8_t ackOk = processMessageConfig();
			checkSendACK(ackOk);																// send appropriate answer

	#ifdef SUPPORT_AES
		}
	#endif
}

/**
 * @brief Process all configuration messages with write actions.
 *        TODO: respect AES signing
 */
uint8_t AS::processMessageConfig() {
	uint8_t ackOk = 1;

	if (rcv_msg.mBody->BY11 == AS_CONFIG_PEER_ADD) {													// CONFIG_PEER_ADD
		//ackOk = configPeerAdd();

	} else if (rcv_msg.mBody->BY11 == AS_CONFIG_PEER_REMOVE) {											// CONFIG_PEER_REMOVE
		//ackOk = configPeerRemove();

	} else if (rcv_msg.mBody->BY11 == AS_CONFIG_START) {												// CONFIG_START
		configStart();

	} else if (rcv_msg.mBody->BY11 == AS_CONFIG_END) {													// CONFIG_END
		configEnd ();

	} else if (rcv_msg.mBody->BY11 == AS_CONFIG_WRITE_INDEX) {											// CONFIG_WRITE_INDEX
		configWriteIndex();

	}

	return ackOk;
}

/**
 * @brief Process CONFIG_PEER_ADD messages
 *        by10 is the channel were the peer has to be added, 
 *        but reworked with -1 to reflect the logix of the module table
 *
 * Message description:
 *             Sender__ Receiver Byte10    Channel Peer-ID_ PeerChannelA  PeerChannelB
 * 0C 0A A4 01 23 70 EC 1E 7A AD 01        01      1F A6 5C 06            05
 */
//inline uint8_t AS::configPeerAdd() {

	// set the peers in the peerdatabase
	//uint8_t ackOk = ee_peer.addPeers(rcv_msg.mBody->BY10, rcv_msg.buf+12);							// send to addPeer function

	//#ifdef AS_DBG																				// only if ee debug is set
	//dbg << F("configPeerAdd, cnl:") << _HEXB(rcv_msg.buf[11]) << F(", data:") << _HEX(rcv_msg.buf + 12, 5) << '\n';
	//#endif

	//return ackOk;																				// return the status
//}

/**
 * @brief Process CONFIG_PEER_REMOVE messages
 *
 * Message description:
 *             Sender__ Receiver    Channel Peer-ID_ PeerChannelA  PeerChannelB
 * 0C 0A A4 01 23 70 EC 1E 7A AD 02 01      1F A6 5C 06            05
 */
//inline uint8_t AS::configPeerRemove() {
//	return ee_peer.remPeers(rcv_msg.mBody->BY10, rcv_msg.buf+12);									// call the remPeer function
//}

/**
 * @brief Process CONFIG_START messages
 *
 * Message description:
 *             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
 * 10 04 A0 01 63 19 63 01 02 04 01 05      00 00 00 00          00
 */
inline void AS::configStart() {
	config_mode.cnl = rcv_msg.mBody->BY10;															// fill structure to remember where to write
	config_mode.lst = rcv_msg.buf[16];
	if ((config_mode.lst == 3) || (config_mode.lst == 4)) {
		config_mode.idx_peer = pcnlModule[rcv_msg.mBody->BY10]->peerDB.get_idx(rcv_msg.buf + 12) ;
		//config_mode.idx_peer = ee_peer.getIdxByPeer(rcv_msg.mBody->BY10, rcv_msg.buf + 12);
	} else {
		config_mode.idx_peer = 0;
	}

	if (config_mode.idx_peer != 0xFF) {
		config_mode.active = 1;																		// set active if there is no error on index
		config_mode.timer.set(20000);																		// set timeout time, will be checked in poll function
		// TODO: set message id flag to config in send module
	}
}

/**
 * @brief Process CONFIG_END messages
 *
 * Message description:
 *             Sender__ Receiver    Channel
 * 10 04 A0 01 63 19 63 01 02 04 01 06
 */
inline void AS::configEnd() {
	cmMaster *pCM = pcnlModule[config_mode.cnl];													// short hand to channel module
	config_mode.active = 0;																			// set inactive

	if ( (config_mode.lst == 0) || (config_mode.lst == 1) ) {												// only list 0 or list 1 to load, while list 3 or list 4 are refreshed with a peer message
		getEEPromBlock( pCM->lstC.ee_addr, pCM->lstC.len, pCM->lstC.val );						// get the respective eeprom block into the channel module array
		pCM->info_config_change();																// and inform the module
	}
}

/**
 * @brief Process CONFIG_WRITE_INDEX messages
 *
 * Message description:
 *             Sender__ Receiver        Channel ConfigData: Register:BytePairs
 * 13 02 A0 01 63 19 63 01 02 04 00  08 02      01 0A 63 0B 19 0C 63
 */
 inline void AS::configWriteIndex(void) {
	if ((config_mode.active) && (config_mode.cnl == rcv_msg.mBody->BY10)) {								// check if we are in config mode and if the channel fit
		ee_list.setListArray(config_mode.cnl, config_mode.lst, config_mode.idx_peer, rcv_msg.buf[0]-11, rcv_msg.buf+12);	// write the string to EEprom

		/*if ((cFlag.channel == 0) && (cFlag.list == 0)) {										// check if we got somewhere in the string a 0x0a, as indicator for a new masterid
			uint8_t maIdFlag = 0;
			for (uint8_t i = 0; i < (rcv_msg.buf[0]+1-12); i+=2) {
				if (rcv_msg.buf[12+i] == 0x0A) maIdFlag = 1;
				#ifdef AS_DBG
					dbg << "x" << i << " :" << _HEXB(rcv_msg.buf[12+i]) << '\n';
				#endif
			}
			if (maIdFlag) {
				ee.getMasterID();
				#ifdef AS_DBG
					dbg << "new masterid\n" << '\n';
				#endif
			}
		}*/
	}
}

/**
 * @brief Process all action (11) messages
 */
void AS::processMessageAction11() {
	if (rcv_msg.mBody->BY10 == AS_ACTION_RESET && rcv_msg.mBody->BY11 == 0x00) {								// RESET
		/*
		 * Message description:
		 *             Sender__ Receiver
		 * 0B 1C B0 11 63 19 63 1F B7 4A 04 00
		 */
		resetStatus = AS_RESET_CLEAR_EEPROM;													// schedule a device reset with clear eeprom

	} else if (rcv_msg.mBody->BY10 == AS_ACTION_ENTER_BOOTLOADER) {								// We should enter the Bootloader
		dbg << "AS_ACTION_ENTER_BOOTLOADER\n";
		/*
		 * Message description:
		 *             Sender__ Receiver
		 * 0B 1C B0 11 63 19 63 1F B7 4A CA
		 */
		resetStatus = AS_RESET;																	// schedule a device reset without eeprom
		rcv_msg.mBody->FLAG.BIDI = 1;

	} else {
		/*
		 * All other action types like STOP_CHANGE, LED, LEDALL, LEVEL, SLEEPMODE and do on
		 *
		 * Message description:
		 *             Sender__ Receiver type actionType channel data
		 * 0E 5E B0 11 63 19 63 1F B7 4A 02   01         01      C8 00 00 00 00
		 */
		//RG::s_modTable *pModTbl = &modTbl[rcv.mBdy.by11];										// pointer to the respective line in the module table
		//if (pModTbl->isActive) {
		//	pModTbl->mDlgt(rcv.mBdy.mTyp, rcv.mBdy.by10, rcv.mBdy.by11, rcv_msg.buf+12, rcv.mBdy.mLen-11);
		//}
		pcnlModule[rcv_msg.mBody->BY11]->message_trigger11(rcv_msg.buf[12], (rcv_msg.mBody->MSG_LEN > 13) ? rcv_msg.buf + 13 : NULL, (rcv_msg.mBody->MSG_LEN > 15) ? rcv_msg.buf + 15 : NULL);
	}
}

/**
 * @brief Process all action (3E, 3F, 40, 41, ...) messages
 * 
 * Within the function we load the respective list3/4 into the
 * list pointer in module table registered. Identification of the list
 * is done by a lookup in the peertable and following the plink into the 
 * the respective line in the channel table.
 *
 */
void AS::processMessageAction3E(uint8_t cnl, uint8_t peer_idx) {

	cmMaster *pCM = pcnlModule[cnl];															// short hand for the channel module pointer
	uint16_t pAddr = pCM->lstP.ee_addr + ( pCM->lstP.len * peer_idx );							// calculate the eeprom address
	getEEPromBlock( pAddr, pCM->lstP.len, pCM->lstP.val );										// get list3 or list4 loaded into the user module

	struct structFlag {
		uint8_t CNL : 4;
		uint8_t UP : 1; // 0x10
		uint8_t DOWN : 1; // 0x20
		uint8_t LONG : 1; // 0x40
		uint8_t LOWBAT : 1; // 0x80
		uint8_t COUNT;
		uint8_t VALUE;
	} sF;

	uint8_t *buf = (rcv_msg.mBody->MSG_TYP == 0x3E) ? rcv_msg.buf + 14 : rcv_msg.buf + 10;
	memcpy( &sF, buf, 3);

	if      (rcv_msg.mBody->MSG_TYP == 0x3E) pCM->message_trigger3E(sF.LONG, sF.COUNT);		// call the user module
	else if (rcv_msg.mBody->MSG_TYP == 0x40) pCM->message_trigger40(sF.LONG, sF.COUNT);
	else if (rcv_msg.mBody->MSG_TYP == 0x41) pCM->message_trigger41(sF.LONG, sF.COUNT, sF.VALUE);
	else sendACK();
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
 * @brief Send the serial number of the device.
 *
 * Message description:
 *             Sender__ Receiver    Serial number
 * 14 77 80 10 1E 7A AD 63 19 63 00 4A 45 51 30 37 33 31 39 30 35
 */
inline void AS::sendINFO_SERIAL(void) {
	snd_msg.mBody->MSG_LEN = 0x14;
	snd_msg.mBody->BY10 = AS_INFO_SERIAL;
	memcpy(snd_msg.buf+11, dev_ident.SERIAL_NR, 10);
	prepareToSend(rcv_msg.mBody->MSG_LEN, AS_MESSAGE_INFO, rcv_msg.mBody->SND_ID);
}

/**
 * @brief Send the peer list
 *
 * Message description:
 *             Sender__ Receiver    peer1        peer2
 * 1A 44 A0 10 1F B7 4A 63 19 63 01 22 66 08 02  22 66 08 01  22 66 08 04  22 66 08 03
 *
 * @param length
 */
inline void AS::sendINFO_PEER_LIST(uint8_t length) {
	snd_msg.mBody->MSG_LEN = length + 10;
	snd_msg.mBody->FLAG.BIDI = 1;
	snd_msg.mBody->BY10 = AS_INFO_PEER_LIST;															//stcSlice.cnl;
	prepareToSend(stcSlice.mCnt++, AS_MESSAGE_INFO, stcSlice.toID);
}

/**
 * @brief Send the peer list
 *
 * Message description:
 *             Sender__ Receiver byte10 reg data reg data
 * 16 79 A0 10 01 02 04 63 19 63 02     02  01   05  40 0A 63 0B 19 0C 63 12 69
 *
 * @param length
 */
inline void AS::sendINFO_PARAM_RESPONSE_PAIRS(uint8_t length) {
	snd_msg.mBody->MSG_LEN = length + 10;
	snd_msg.mBody->FLAG.BIDI = 1;
	snd_msg.mBody->BY10 = AS_INFO_PARAM_RESPONSE_PAIRS;
	//snd_msg.mBody->BY10 = (length < 3) ? AS_INFO_PARAM_RESPONSE_SEQ : AS_INFO_PARAM_RESPONSE_PAIRS;
	prepareToSend(stcSlice.mCnt++, AS_MESSAGE_INFO, stcSlice.toID);
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

	snd_msg.mBody->MSG_CNT = mCounter;
	snd_msg.mBody->MSG_TYP = mType;
	snd_msg.mBody->SND_ID[0] = dev_ident.HMID[0];
	snd_msg.mBody->SND_ID[1] = dev_ident.HMID[1];
	snd_msg.mBody->SND_ID[2] = dev_ident.HMID[2];
	memcpy(snd_msg.mBody->RCV_ID, receiverAddr, 3);

	snd_msg.active = 1;																				// remember to fire the message
}

void AS::sendINFO_PARAM_RESPONSE_SEQ(uint8_t len) {
	/*
	 * TODO: make ready
	 *
	 * Message description:
	 *             Sender__ Receiver byte10 Offset  Data
	 * 16 90 A0 10 01 02 04 63 19 63 03     02      00 00 32 64 00 FF 00 FF 01 13 33
	 */
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

// - homematic specific functions ------------------

/**
 * @brief Decode the incoming messages
 *        Note: this is no encryption!
 *
 * @param buf   pointer to buffer
 */
/*void AS::decode(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i = 2; i < buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xDC) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}*/

/**
 * @brief Encode the outgoing messages
 *        Note: this is no encryption!
 *
 * @param buf   pointer to buffer
 */
/*void AS::encode(uint8_t *buf) {
	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i = 2; i < buf[0]; i++) {
		prev = (prev + 0xDC) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}*/

#ifdef RV_DBG_EX																				// only if extended AS debug is set
	/**
	 * @brief Debuging: Explain the Messages
	 *
	 * @param buf   pointer to buffer
	 */
	void AS::explainMessage(uint8_t *buf) {
		dbg << F("   ");																		// save some byte and send 3 blanks once, instead of having it in every if

		if        ((buf[3] == AS_MESSAGE_DEVINFO)) {
			dbg << F("DEVICE_INFO; fw: ") << _HEX((buf+10),1) << F(", type: ") << _HEX((buf+11),2) << F(", serial: ") << _HEX((buf+13),10) << '\n';
			dbg << F("              , class: ") << _HEXB(buf[23]) << F(", pCnlA: ") << _HEXB(buf[24]) << F(", pCnlB: ") << _HEXB(buf[25]) << F(", na: ") << _HEXB(buf[26]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_PEER_ADD)) {
			dbg << F("CONFIG_PEER_ADD; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnlA: ") << _HEXB(buf[15]) << F(", pCnlB: ") << _HEXB(buf[16]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_PEER_REMOVE)) {
			dbg << F("CONFIG_PEER_REMOVE; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnlA: ") << _HEXB(buf[15]) << F(", pCnlB: ") << _HEXB(buf[16]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_PEER_LIST_REQ)) {
			dbg << F("CONFIG_PEER_LIST_REQ; cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_PARAM_REQ)) {
			dbg << F("CONFIG_PARAM_REQ; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnl: ") << _HEXB(buf[15]) << F(", lst: ") << _HEXB(buf[16]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_START)) {
			dbg << F("CONFIG_START; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnl: ") << _HEXB(buf[15]) << F(", lst: ") << _HEXB(buf[16]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_END)) {
			dbg << F("CONFIG_END; cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_WRITE_INDEX)) {
			dbg << F("CONFIG_WRITE_INDEX; cnl: ") << _HEXB(buf[10]) << F(", data: ") << _HEX((buf+12),(buf[0]-11));

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_SERIAL_REQ)) {
			dbg << F("CONFIG_SERIAL_REQ");

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_PAIR_SERIAL)) {
			dbg << F("PAIR_SERIAL, serial: ") << _HEX((buf+12),10);

		} else if ((buf[3] == AS_MESSAGE_CONFIG) && (buf[11] == AS_CONFIG_STATUS_REQUEST)) {
			dbg << F("CONFIG_STATUS_REQUEST, cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_ACK)) {
			if (buf[0] == 0x0A) dbg << F("ACK");
			else dbg << F("ACK; data: ") << _HEX((buf+11),buf[0]-10);

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_ACK_STATUS)) {
			dbg << F("ACK_STATUS; cnl: ") << _HEXB(buf[11]) << F(", status: ") << _HEXB(buf[12]) << F(", down/up/loBat: ") << _HEXB(buf[13]);
			if (buf[0] > 13) dbg << F(", rssi: ") << _HEXB(buf[14]);

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_ACK2)) {
			dbg << F("ACK2");

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_AES_CHALLANGE)) {
			dbg << F("REQUEST_AES_RESPONSE, challange: ") << _HEX((buf+11),6);

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_NACK)) {
			dbg << F("NACK");

		} else if ((buf[3] == AS_MESSAGE_RESPONSE) && (buf[10] == AS_RESPONSE_NACK_TARGET_INVALID)) {
			dbg << F("NACK_TARGET_INVALID");

		} else if ((buf[3] == AS_MESSAGE_RESPONSE_AES)) {
			dbg << F("RESPONSE_AES; data: ") << _HEX((buf+10),buf[0]-9);

		} else if ((buf[3] == AS_MESSAGE_KEY_EXCHANGE) && (buf[10] == 0x01)) {
			// TODO: check if byte 10 must be 0x01; the doc says byte 10 must 0x00
			dbg << F("TO_HMLAN:SEND_AES_CODE; cnl: ") << _HEXB(buf[11]);

		} else if ((buf[3] == AS_MESSAGE_KEY_EXCHANGE)) {
			dbg << F("TO_ACTOR:KEY_EXCHANGE; encrypted data: ") << _HEX((buf+10), 16);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_SERIAL)) {
			dbg << F("INFO_SERIAL; serial: ") << _HEX((buf+11),10);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_PEER_LIST)) {
			dbg << F("INFO_PEER_LIST; peer1: ") << _HEX((buf+11),4);
			if (buf[0] >= 19) dbg << F(", peer2: ") << _HEX((buf+15),4);
			if (buf[0] >= 23) dbg << F(", peer3: ") << _HEX((buf+19),4);
			if (buf[0] >= 27) dbg << F(", peer4: ") << _HEX((buf+23),4);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_PARAM_RESPONSE_PAIRS)) {
			dbg << F("INFO_PARAM_RESPONSE_PAIRS; data: ") << _HEX((buf+11),buf[0]-10);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_PARAM_RESPONSE_SEQ)) {
			dbg << F("INFO_PARAM_RESPONSE_SEQ; offset: ") << _HEXB(buf[11]) << F(", data: ") << _HEX((buf+12),buf[0]-11);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_PARAMETER_CHANGE)) {
			dbg << F("INFO_PARAMETER_CHANGE; cnl: ") << _HEXB(buf[11]) << F(", peer: ") << _HEX((buf+12),4) << F(", pLst: ") << _HEXB(buf[16]) << F(", data: ") << _HEX((buf+17),buf[0]-16);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_ACTUATOR_STATUS)) {
			dbg << F("INFO_ACTUATOR_STATUS; cnl: ") << _HEXB(buf[11]) << F(", status: ") << _HEXB(buf[12]) << F(", na: ") << _HEXB(buf[13]);
			if (buf[0] > 13) dbg << F(", rssi: ") << _HEXB(buf[14]);

		} else if ((buf[3] == AS_MESSAGE_INFO) && (buf[10] == AS_INFO_RT_STATUS)) {
			dbg << F("INFO_RT_STATUS; dstTmp: ") << (buf[11]>>3) << F(".") << ((buf[11]>>2)&1)*5 << F(", Tmp: ") << (((buf[11]&3)<<8)+buf[12])/10 << F(".") << (((buf[11]&3)<<8)+buf[12])%10;
			dbg << F(", bat: ") << ((buf[13]&0x1f)+15)/10 << F(".") << ((buf[13]&0x1f)+15)%10;
			dbg << F(", err: ") << _HEXB(buf[13]>>5) << F(", valve: ") << _HEXB(buf[14]) << F(", ctrlMode: ") << _HEXB(buf[15]);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_SET)) {
			dbg << F("SET; cnl: ") << _HEXB(buf[11]) << F(", value: ") << _HEXB(buf[12]) << F(", rampTime: ") << _HEX((buf+13),2) << F(", duration: ") << _HEX((buf+15),2);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_STOP_CHANGE)) {
			dbg << F("STOP_CHANGE; cnl: ") << _HEXB(buf[11]);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_RESET) && (buf[11] == 0x00)) {
			dbg << F("RESET");

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_LED)) {
			dbg << F("LED; cnl: ") << _HEXB(buf[11]) << F(", color: ") << _HEXB(buf[12]);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_LEDALL) && (buf[11] == 0x00)) {
			dbg << F("LED_ALL; Led1To16: ") << _HEX((buf+12),4);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_LEVEL)) {
			dbg << F("LED; cnl: ") << _HEXB(buf[11]) << F(", time: ") << _HEXB(buf[12]) << F(", speed: ") << _HEXB(buf[13]);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_SLEEPMODE)) {
			dbg << F("SLEEPMODE; cnl: ") << _HEXB(buf[11]) << F(", mode: ") << _HEXB(buf[12]);

		} else if ((buf[3] == AS_MESSAGE_ACTION) && (buf[10] == AS_ACTION_ENTER_BOOTLOADER)) {
			dbg << F("ENTER_BOOTLOADER;");

		} else if ((buf[3] == AS_MESSAGE_HAVE_DATA)) {
			dbg << F("HAVE_DATA");

		} else if ((buf[3] == AS_MESSAGE_SWITCH_EVENT)) {
			dbg << F("SWITCH; dst: ") << _HEX((buf+10),3) << F(", na: ") << _HEXB(buf[13]) << F(", cnl: ") << _HEXB(buf[14]) << F(", counter: ") << _HEXB(buf[15]);

		} else if ((buf[3] == AS_MESSAGE_TIMESTAMP)) {
			dbg << F("TIMESTAMP; na: ") << _HEX((buf+10),2) << F(", time: ") << _HEX((buf+12),2);

		} else if ((buf[3] == AS_MESSAGE_REMOTE_EVENT)) {
			dbg << F("REMOTE; button: ") << _HEXB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", counter: ") << _HEXB(buf[11]);

		} else if ((buf[3] == AS_MESSAGE_SENSOR_EVENT)) {
			dbg << F("SENSOR_EVENT; button: ") <<_HEXB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", value: ") << _HEXB(buf[11]) << F(", next: ") << _HEXB(buf[12]);

		} else if ((buf[3] == AS_MESSAGE_SENSOR_DATA)) {
			dbg << F("SENSOR_DATA; cmd: ") << _HEXB(buf[10]) << F(", fld1: ") << _HEXB(buf[11]) << F(", val1: ") << _HEX((buf+12),2) << F(", fld2: ") << _HEXB(buf[14]) << F(", val2: ") << _HEX((buf+15),2) << F(", fld3: ") << _HEXB(buf[17]) << F(", val3: ") << _HEX((buf+18),2) << F(", fld4: ") << _HEXB(buf[20]) << F(", val4: ") << _HEX((buf+21),2);

		} else if ((buf[3] == AS_MESSAGE_CLIMATE_EVENT)) {
			dbg << F("CLIMATE_EVENT; cmd: ") << _HEXB(buf[10]) << F(", valvePos: ") << _HEXB(buf[11]);

		} else if ((buf[3] == AS_MESSAGE_CLIMATECTRL_EVENT)) {
					dbg << F("CLIMATE_EVENT; cmd: ???");  //<< _HEXB(buf[10]) << F(", valvePos: ") << _HEXB(buf[11]);

		} else if ((buf[3] == AS_MESSAGE_WEATHER_EVENT)) {
			dbg << F("WEATHER_EVENT; temp: ") << _HEX(buf+10,2) << F(", hum: ") << _HEXB(buf[12]);


		} else {
			dbg << F("Unknown Message, please report! MsgType: ") << _HEXB(buf[3]) << _HEXB(buf[10]);
		}
		dbg << F("\n\n");
	}
#endif

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

		snd_msg.mBody->MSG_LEN = 0x11;
		snd_msg.mBody->FLAG.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
		snd_msg.mBody->BY10 = AS_RESPONSE_AES_CHALLANGE;											// AES Challenge

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

		prepareToSend(rcv_msg.mBody->MSG_CNT, AS_MESSAGE_RESPONSE, rcv_msg.mBody->SND_ID);
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

	uint8_t tByte;
	if ((iTime & 0x1F) != 0) {
		tByte = 2;
		for (uint8_t i = 1; i < (iTime & 0x1F); i++) tByte *= 2;
	} else tByte = 1;

	return (uint32_t)tByte*(iTime>>5)*100;
}
