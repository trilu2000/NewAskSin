//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define AS_DBG
//#define RV_DBG_EX

#include "AS.h"

waitTimer cnfTmr;																			// config timer functionality
waitTimer pairTmr;																			// pair timer functionality

// public:		//---------------------------------------------------------------------------------------------------------
AS::AS() {
}

void AS::init(void) {
	#ifdef AS_DBG																			// only if cc debug is set
		dbgStart();																			// serial setup
		dbg << F("AS.\n");																	// ...and some information
	#endif
	
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts

	ee.init();																				// eeprom init
	cc.init();																				// init the rf module

	sn.init(this);																			// send module
	rv.init(this);																			// receive module
	rg.init(this);																			// module registrar
	confButton.init(this);																	// config button
	pw.init(this);																			// power management
	bt.init(this);																			// battery check
	
	initMillis();																			// start the millis counter

	#ifdef SUPPORT_AES
		initRandomSeed();
	#endif

	// everything is setuped, enable RF functionality
	enableGDO0Int();																		// enable interrupt to get a signal while receiving data
}

void AS::poll(void) {

	// check if something received
	if (ccGetGDO0()) {																		// check if something was received
		cc.rcvData(rv.buf);																	// copy the data into the receiver module
		if (rv.hasData) decode(rv.buf);														// decode the string

/*
		// TEST Begin
		if (rv.hasData) {
			if (rv.nextIsEncrypted == 0) {
				if (rv.buf[03] == 02 && rv.buf[10] == 04) {
					dbg << "Request AES" << '\n';

					uint8_t challenge[6];
					memcpy(challenge, rv.buf+11, 6);											// get challenge
					this->makeTmpKey(challenge);

					dbg << F(">>> ch :  ") << _HEX(challenge, 6) << F(" <<<") << "\n";
					rv.nextIsEncrypted = 1;
				} else {
					memcpy(rv.prevBuf, rv.buf, rv.buf[0]+1);
				}
			} else {
				uint8_t payload[16];
				memcpy(payload, rv.buf+10, 16);
				dbg << F(">>> pl   : ") << _HEX(payload, 16) << F(" <<<") << "\n";

				this->payloadDecrypt(payload, rv.prevBuf);
				dbg << "\n\n";
				rv.nextIsEncrypted = 0;
			}
		}
		// TEST End
 */
	}

	// handle send and receive buffer
	if (rv.hasData) rv.poll();																// check if there is something in the received buffer
	if (sn.active) sn.poll();																// check if there is something to send

	// handle the slice send functions
	if (stcSlice.active) sendSliceList();													// poll the slice list send function
	if (stcPeer.active) sendPeerMsg();														// poll the peer message sender
	
	// time out the config flag
	if (cFlag.active) {																		// check only if we are still in config mode
		if (cnfTmr.done()) cFlag.active = 0;												// when timer is done, set config flag to inactive
	}

	// time out the pairing timer
	if (pairActive) { 
		if (pairTmr.done()) {
			pairActive = 0;
			isEmpty(MAID, 3)? ld.set(pair_err) : ld.set(pair_suc);	
		}
	}

	// regular polls
	rg.poll();																				// poll the channel module handler
	confButton.poll();																		// poll the config button
	ld.poll();																				// poll the led's
	bt.poll();																				// poll the battery check
		
	// check if we could go to standby
	pw.poll();																				// poll the power management
	
	// some sanity poll routines
}

// - send functions --------------------------------
void AS::sendDEVICE_INFO(void) {
	// description --------------------------------------------------------
	//                 reID      toID      fw  type   serial                         class  pCnlA  pCnlB  unknown
	// l> 1A 94 84 00  1F B7 4A  01 02 04  15  00 6C  4B 45 51 30 32 33 37 33 39 36  10     41     01     00
	// do something with the information ----------------------------------

	uint8_t xCnt;
	if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == 0x0A)) {
		xCnt = rv.mBdy.mLen;																// send counter - is it an answer or a initial message
	} else {
		xCnt = sn.msgCnt++;
	}

	sn.mBdy.mLen = 0x1a;
//	sn.mBdy.mCnt = xCnt;
	sn.mBdy.mFlg.CFG = 0;
	sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3)) ? 0 : 1;

	memcpy_P(sn.buf+10, devDef.devIdnt, 3);
	memcpy(sn.buf+13, HMSR, 10);
	memcpy_P(sn.buf+23, devDef.devIdnt+3, 4);

	prepareToSend(xCnt, AS_MESSAGE_TYPE_DEVINFO, MAID);

	pairActive = 1;																			// set pairing flag
	pairTmr.set(20000);
	ld.set(pairing);																		// and visualize the status
}

void AS::sendACK(void) {
	// description --------------------------------------------------------
	//                reID      toID      ACK
	// l> 0A 24 80 02 1F B7 4A  63 19 63  00
	// do something with the information ----------------------------------

	if (!rv.mBdy.mFlg.BIDI) return;															// overcome the problem to answer from a user class on repeated key press

	sn.mBdy.mLen = 0x0A;
//	sn.mBdy.mCnt = rv.mBdy.mCnt;
	sn.mBdy.by10 = 0x00;
	prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_TYPE_RESPONSE, rv.mBdy.reID);
}

void AS::checkSendACK(uint8_t ackOk) {
	if (rv.ackRq) {
		if (ackOk) {
			sendACK();
		} else {
			sendNACK();
		}
	}
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
	void AS::sendAckAES(uint8_t *data) {
		if (!rv.mBdy.mFlg.BIDI) return;															// overcome the problem to answer from a user class on repeated key press

		sn.mBdy.mLen = 0x0E;
//		sn.mBdy.mFlg.BIDI = 0;
//		sn.mBdy.mCnt = rv.mBdy.mCnt;
		sn.mBdy.by10 = AS_RESPONSE_TYPE_ACK;
		sn.mBdy.by11 = data[0];
		memcpy(sn.mBdy.pyLd, data+1, 3);

		prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_TYPE_RESPONSE, rv.mBdy.reID);
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
	if (!rv.mBdy.mFlg.BIDI) return;															// overcome the problem to answer from a user class on repeated key press
	
	sn.mBdy.mLen      = 0x0e;
//	sn.mBdy.mCnt = rv.mBdy.mCnt;
	sn.mBdy.mFlg.BIDI = 0;
//	sn.mBdy.mTyp = 0x02;
//	memcpy(sn.mBdy.reID, HMID, 3);
//	memcpy(sn.mBdy.toID, rv.mBdy.reID, 3);
	sn.mBdy.by10      = 0x01;
	sn.mBdy.by11      = channel;
	sn.mBdy.pyLd[0]   = state;
	sn.mBdy.pyLd[1]   = action | (bt.getStatus() << 7);
	sn.mBdy.pyLd[2]   = cc.rssi;

	prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_TYPE_RESPONSE, rv.mBdy.reID);
}

/**
 * @brief Send a NACK (not ACK)
 *
 * Message description:
 *             Sender__ Receiver NACK
 * 0A 24 80 02 1F B7 4A 63 19 63 80
 */
void AS::sendNACK(void) {
	sn.mBdy.mLen = 0x0a;
//	sn.mBdy.mCnt = rv.mBdy.mLen;
//	sn.mBdy.mTyp = 0x02;
//	memcpy(sn.mBdy.reID,HMID,3);
//	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = AS_RESPONSE_TYPE_NACK;

	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_TYPE_RESPONSE, rv.mBdy.reID);
}

/**
 * @brief Send a NACK (not ACK and target invalid)
 *
 * Message description:
 *             Sender__ Receiver NACK_TAGRET_INVALID
 * 0A 24 80 02 1F B7 4A 63 19 63 84
 */
void AS::sendNACK_TARGET_INVALID(void) {
	sn.mBdy.mLen = 0x0a;
//	sn.mBdy.mCnt = rv.mBdy.mLen;
//	sn.mBdy.mTyp = 0x02;
//	memcpy(sn.mBdy.reID,HMID,3);
//	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = AS_RESPONSE_TYPE_NACK_TARGET_INVALID;
	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_TYPE_RESPONSE, rv.mBdy.reID);
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
	sn.mBdy.mLen = 0x0e;
	uint8_t cnt;

	if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == 0x0e)) {
		cnt = rv.mBdy.mCnt;
	} else {
		cnt = sn.msgCnt++;
	}

	sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3))?0:1;
//	sn.mBdy.mTyp = 0x10;
//	memcpy(sn.mBdy.reID, HMID, 3);
//	memcpy(sn.mBdy.toID, MAID, 3);
	sn.mBdy.by10      = 0x06;
	sn.mBdy.by11      = channel;
	sn.mBdy.pyLd[0]   = state;
	sn.mBdy.pyLd[1]   = flag; // | (bt.getStatus() << 7);
	sn.mBdy.pyLd[2]   = cc.rssi;

	prepareToSend(cnt, AS_MESSAGE_TYPE_INFO, MAID);
}

void AS::sendINFO_TEMP(void) {
	//TODO: make ready#

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
void AS::sendREMOTE(uint8_t channel, uint8_t burst, uint8_t *payload) {
	sendEvent(channel, burst, AS_MESSAGE_TYPE_REMOTE_EVENT, payload, 2);
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
	sendEvent(channel, burst, AS_MESSAGE_TYPE_SENSOR_EVENT, payload, 3);
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
 * @param burst   Set to 1 for burst mode, or 0
 * @param mType   Message type
 * @param pLen    Length of payload in bytes, not more than 16
 * @param payload pointer to payload
 *
 * @attention The payload length may not exceed 16 bytes. If a greater value
 * for len is given, it is limited to 16 to prevent HM-CFG-LAN (v0.961) to crash.
 */
void AS::sendEvent(uint8_t channel, uint8_t burst, uint8_t mType, uint8_t *payload, uint8_t pLen) {
	if (pLen>16) {
		#ifdef AS_DBG
			dbg << "AS::send_generic_event("<<cnl<<","<<burst<<",0x"<<_HEX(&mTyp,1)<<","<<len<<",...): payload exceeds maximum length of 16\n";
		#endif
		pLen = 16;
	}

	stcPeer.pL     = payload;
	stcPeer.lenPL  = pLen + 1;
	stcPeer.cnl    = channel;
	stcPeer.burst  = burst;
	stcPeer.bidi   = (~payload[0] & AS_BUTTON_BYTE_LONGPRESS_BIT) ? 0 : 1;		// depends on long-key-press-bit (long didn't need ACK)	stcPeer.bidi   = (isEmpty(MAID,3)) ? 0 : 1;
	stcPeer.bidi   = (isEmpty(MAID,3)) ? 0 : 1;
	stcPeer.mTyp   = mType;
	stcPeer.active = 1;
	// --------------------------------------------------------------------
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
	// --------------------------------------------------------------------
}

void AS::sendClimateEvent(void) {
	//TODO: make ready#

	//"58"          => { txt => "ClimateEvent", params => {
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
	// --------------------------------------------------------------------
}

void AS::sendSetTeamTemp(void) {
	//TODO: make ready#

	//"59"          => { txt => "setTeamTemp" , params => {
	//CMD      => "00,2",
	//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
	//mode     => '02,2,$val=(hex($val) & 0x3)',} },
	// --------------------------------------------------------------------
}

void AS::sendWeatherEvent(void) {
	//TODO: make ready#

	//"70"          => { txt => "WeatherEvent", params => {
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
	// --------------------------------------------------------------------
}

// private:		//---------------------------------------------------------------------------------------------------------
// - poll functions --------------------------------
void AS::sendSliceList(void) {
	if (sn.active) return;																	// check if send function has a free slot, otherwise return

	uint8_t cnt;

	if        (stcSlice.peer) {			// INFO_PEER_LIST
		cnt = ee.getPeerListSlc(stcSlice.cnl, stcSlice.curSlc, sn.buf+11);					// get the slice and the amount of bytes
		sendINFO_PEER_LIST(cnt);															// create the body
		stcSlice.curSlc++;																	// increase slice counter
		//dbg << "peer slc: " << _HEX(sn.buf,sn.buf[0]+1) << '\n';							// write to send buffer

	} else if (stcSlice.reg2) {			// INFO_PARAM_RESPONSE_PAIRS
		cnt = ee.getRegListSlc(stcSlice.cnl, stcSlice.lst, stcSlice.idx, stcSlice.curSlc, sn.buf+11); // get the slice and the amount of bytes
		//dbg << "cnt: " << cnt << '\n';
		sendINFO_PARAM_RESPONSE_PAIRS(cnt);
		stcSlice.curSlc++;																	// increase slice counter
		//dbg << "reg2 slc: " << _HEX(sn.buf,sn.buf[0]+1) << '\n';							// write to send buffer
		
	} else if (stcSlice.reg3) {			// INFO_PARAM_RESPONSE_SEQ

	}

	if (stcSlice.curSlc == stcSlice.totSlc) {												// if everything is send, we could empty the struct
		memset((void*)&stcSlice, 0, 10);													// by memset
		//dbg << "end: " << stcSlice.active << stcSlice.peer << stcSlice.reg2 << stcSlice.reg3 << '\n';
	}
}

void AS::sendPeerMsg(void) {
	uint8_t maxRetries;

	if (stcPeer.bidi) maxRetries = 3;
	else maxRetries = 1;
	
	if (sn.active) return;																	// check if send function has a free slot, otherwise return
	
	// first run, prepare amount of slots
	if (!stcPeer.maxIdx) {
		stcPeer.maxIdx = ee.getPeerSlots(stcPeer.cnl);										// get amount of messages of peer channel
	
		if (stcPeer.maxIdx == ee.countFreeSlots(stcPeer.cnl) ) {							// check if at least one peer exist in db, otherwise send to master and stop function
			prepPeerMsg(MAID, maxRetries);
			sn.msgCnt++;																	// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));									// clean out and return
			return;
		}
	}
	
	// all slots of channel processed, start next round or end processing
	if (stcPeer.curIdx >= stcPeer.maxIdx) {													// check if all peer slots are done
		stcPeer.rnd++;																		// increase the round counter
		
		if ((stcPeer.rnd >= maxRetries) || (isEmpty(stcPeer.slt,8))) {						// all rounds done or all peers reached
			//dbg << "through\n";
			sn.msgCnt++;																	// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));									// clean out and return
			
		} else {																			// start next round
			//dbg << "next round\n";
			stcPeer.curIdx = 0;

		}
		return;

	} else if ((stcPeer.curIdx) && (!sn.timeOut)) {											// peer index is >0, first round done and no timeout
		uint8_t idx = stcPeer.curIdx-1;
		stcPeer.slt[idx >> 3] &=  ~(1 << (idx & 0x07));										// clear bit, because message got an ACK
	}
	
	// set respective bit to check if ACK was received
	if (!stcPeer.rnd) stcPeer.slt[stcPeer.curIdx >> 3] |= (1<<(stcPeer.curIdx & 0x07));		// set bit in slt table										// clear bit in slt and increase counter


	// exit while bit is not set
	if (!stcPeer.slt[stcPeer.curIdx >> 3] & (1<<(stcPeer.curIdx & 0x07))) {
		stcPeer.curIdx++;																	// increase counter for next time
		return;
	}

	uint8_t tPeer[4];																		// get the respective peer
	ee.getPeerByIdx(stcPeer.cnl, stcPeer.curIdx, tPeer);
	
	#ifdef AS_DBG
		dbg << "a: " << stcPeer.curIdx << " m " << stcPeer.maxIdx << '\n';
	#endif

	if (isEmpty(tPeer,4)) {																	// if peer is 0, set done bit in slt and skip
		stcPeer.slt[stcPeer.curIdx >> 3] &=  ~(1<<(stcPeer.curIdx & 0x07));					// remember empty peer in slt table										// clear bit in slt and increase counter
		stcPeer.curIdx++;																	// increase counter for next time
		return;																				// wait for next round
	}

	// if we are here, there is something to send
	//dbg << "cnl:" << stcPeer.cnl << " cIdx:" << stcPeer.curIdx << " mIdx:" << stcPeer.maxIdx << " slt:" << _HEX(stcPeer.slt,8) << '\n';
	
	// todo: get the respective list4 entries and take care while sending the message
	// peerNeedsBurst  =>{a=>  1.0,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"peer expects burst",lit=>{off=>0,on=>1}},
	// expectAES       =>{a=>  1.7,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"expect AES"        ,lit=>{off=>0,on=>1}},
	// fillLvlUpThr    =>{a=>  4.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level upper threshold"},
	// fillLvlLoThr    =>{a=>  5.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level lower threshold"},
	l4_0x01 = *(s_l4_0x01*)ee.getRegAddr(stcPeer.cnl, 4, stcPeer.curIdx, 0x01);
	
	prepPeerMsg(tPeer, 1);
	
	if (!sn.mBdy.mFlg.BIDI)
	stcPeer.slt[stcPeer.curIdx >> 3] &=  ~(1<<(stcPeer.curIdx & 0x07));						// clear bit, because it is a message without need to be repeated

	stcPeer.curIdx++;																		// increase counter for next time
}

void AS::prepPeerMsg(uint8_t *xPeer, uint8_t retr) {

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

	sn.mBdy.mLen = stcPeer.lenPL +9;														// set message length
//	sn.mBdy.mCnt = sn.msgCnt;																// set message counter

	sn.mBdy.mFlg.CFG = 1;
	sn.mBdy.mFlg.BIDI = stcPeer.bidi;														// message flag
	sn.mBdy.mFlg.BURST = l4_0x01.peerNeedsBurst;
	
//	sn.mBdy.mTyp = stcPeer.mTyp;															// message type
	//uint8_t t1[] = {0x23,0x70,0xD8};
	//memcpy(sn.mBdy.reID, t1, 3);															// sender id
//	memcpy(sn.mBdy.reID, HMID, 3);															// sender id
//	memcpy(sn.mBdy.toID, xPeer, 3);															// receiver id
	sn.mBdy.by10 = stcPeer.cnl;
	sn.mBdy.by10 |= (bt.getStatus() << 7);													// battery bit
	memcpy(sn.buf+11, stcPeer.pL, stcPeer.lenPL);											// payload
	
	sn.maxRetr = retr;																		// send only one time

	prepareToSend(sn.msgCnt, stcPeer.mTyp, xPeer);
}

/**
 * @brief Receive handler: Process received messages
 */
void AS::recvMessage(void) {
	uint8_t by10 = rv.mBdy.by10 - 1;
	uint8_t cnl1 = cFlag.cnl - 1;

	// check which type of message was received
	if         (rv.mBdy.mTyp == AS_MESSAGE_TYPE_DEVINFO) {												// Device info
		//TODO: do something with the information

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PEER_ADD)) {		// CONFIG_PEER_ADD
		/* Message description:
		 *             Sender__ Receiver    Channel Peer-ID_ PeerChannelA  PeerChannelB
		 * 0C 0A A4 01 23 70 EC 1E 7A AD 01 01      1F A6 5C 06            05
		 */

		ee.remPeer(rv.mBdy.by10, rv.buf+12);															// first call remPeer to avoid doubles
		uint8_t ret = ee.addPeer(rv.mBdy.by10, rv.buf+12);												// send to addPeer function

		// let module registrations know of the change
		if ((ret) && (modTbl[by10].cnl)) {
			modTbl[by10].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+15, 4);
		}

		checkSendACK(ret);																				// send appropriate answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PEER_REMOVE)) {	// CONFIG_PEER_REMOVE
		/* Message description:
		 *             Sender__ Receiver    Channel Peer-ID_ PeerChannelA  PeerChannelB
		 * 0C 0A A4 01 23 70 EC 1E 7A AD 02 01      1F A6 5C 06            05
		 */

		uint8_t ret = ee.remPeer(rv.mBdy.by10,rv.buf+12);												// call the remPeer function

		checkSendACK(ret);																				// send appropriate answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PEER_LIST_REQ)) {	// CONFIG_PEER_LIST_REQ
		/* Message description:
		 *             Sender__ Receiver    Channel
		 * 0C 0A A4 01 23 70 EC 1E 7A AD 02 01
		 */

		stcSlice.totSlc = ee.countPeerSlc(rv.mBdy.by10);												// how many slices are need
		stcSlice.mCnt = rv.mBdy.mCnt;																	// remember the message count
		memcpy(stcSlice.toID, rv.mBdy.reID, 3);
		stcSlice.cnl = rv.mBdy.by10;																	// send input to the send peer function
		stcSlice.peer = 1;																				// set the type of answer
		stcSlice.active = 1;																			// start the send function
		// answer will send from sendsList(void)

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PARAM_REQ)) {		// CONFIG_PARAM_REQ
		/* Message description:
		 *             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
		 * 10 04 A0 01 63 19 63 01 02 04 01  04     00 00 00 00          01
		 */

		dbg << "CONFIG_PARAM_REQ \n";
		
		if ((rv.buf[16] == 0x03) || (rv.buf[16] == 0x04)) {												// only list 3 and list 4 needs an peer id and idx
			stcSlice.idx = ee.getIdxByPeer(rv.mBdy.by10, rv.buf+12);									// get peer index
		} else {
			stcSlice.idx = 0;																			// otherwise peer index is 0
		}
 
		stcSlice.totSlc = ee.countRegListSlc(rv.mBdy.by10, rv.buf[16]);									// how many slices are need
		stcSlice.mCnt = rv.mBdy.mCnt;																	// remember the message count
		memcpy(stcSlice.toID, rv.mBdy.reID, 3);
		stcSlice.cnl = rv.mBdy.by10;																	// send input to the send peer function
		stcSlice.lst = rv.buf[16];																		// send input to the send peer function
		stcSlice.reg2 = 1;																				// set the type of answer
		
		#ifdef AS_DBG
			dbg << "cnl: " << rv.mBdy.by10 << " s: " << stcSlice.idx << '\n';
			dbg << "totSlc: " << stcSlice.totSlc << '\n';
		#endif

		if ((stcSlice.idx != 0xFF) && (stcSlice.totSlc > 0)) {
			stcSlice.active = 1;																		// only send register content if something is to send															// start the send function
		} else {
			memset((void*)&stcSlice, 0, 10);															// otherwise empty variable
		}
		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_START)) {			// CONFIG_START
		/* Message description:
		 *             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
		 * 10 04 A0 01 63 19 63 01 02 04 01 05      00 00 00 00          00
		 */

		cFlag.cnl = rv.mBdy.by10;																		// fill structure to remember where to write
		cFlag.lst = rv.buf[16];
		if ((cFlag.lst == 3) || (cFlag.lst == 4)) {
			cFlag.idx = ee.getIdxByPeer(rv.mBdy.by10, rv.buf+12);
		} else {
			cFlag.idx = 0;
		}
		
		if (cFlag.idx != 0xff) {
			cFlag.active = 1;																			// set active if there is no error on index
			cnfTmr.set(20000);																			// set timeout time, will be checked in poll function
			// TODO: set message id flag to config in send module
			
		}
	
		checkSendACK(1);																				// send appropriate answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_END)) {			// CONFIG_END
		/*
		 * Message description:
		 *             Sender__ Receiver    Channel
		 * 10 04 A0 01 63 19 63 01 02 04 01 06
		 */

		cFlag.active = 0;																				// set inactive
		if ((cFlag.cnl == 0) && (cFlag.idx == 0)) {
			ee.getMasterID();
		}
		// remove message id flag to config in send module

		if ((cFlag.cnl > 0) && (modTbl[cnl1].cnl)) {
			/*
			 * Check if a new list1 was written and reload.
			 * No need for reload list3/4 because they will be loaded on an peer event.
			 */
			if (cFlag.lst == 1) {
				ee.getList(cFlag.cnl, 1, cFlag.idx, modTbl[cnl1].lstCnl); 								// load list1 in the respective buffer
			}
			modTbl[cnl1].mDlgt(0x01, 0, 0x06, NULL, 0);													// inform the module of the change
		}
		
		checkSendACK(1);																				// send appropriate answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_WRITE_INDEX)) {	// CONFIG_WRITE_INDEX
		/*
		 * Message description:
		 *             Sender__ Receiver        Channel ConfigData: Register:BytePairs
		 * 13 02 A0 01 63 19 63 01 02 04 00  08 02      01 0A 63 0B 19 0C 63
		 */

		if ((cFlag.active) && (cFlag.cnl == rv.mBdy.by10)) {											// check if we are in config mode and if the channel fit
			ee.setListArray(cFlag.cnl, cFlag.lst, cFlag.idx, rv.buf[0]+1-11, rv.buf+12);				// write the string to EEprom
			
			if ((cFlag.cnl == 0) && (cFlag.lst == 0)) {													// check if we got somewhere in the string a 0x0a, as indicator for a new masterid
				uint8_t maIdFlag = 0;				
				for (uint8_t i = 0; i < (rv.buf[0]+1-11); i+=2) {
					if (rv.buf[12+i] == 0x0a) maIdFlag = 1;
					#ifdef AS_DBG
						dbg << "x" << i << " :" << _HEXB(rv.buf[12+i]) << '\n';
					#endif
				}
				if (maIdFlag) {
					ee.getMasterID();
					#ifdef AS_DBG
						dbg << "new masterid\n" << '\n';
					#endif
				}
			}
		}

		checkSendACK(1);																				// send appropriate answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_SERIAL_REQ)) {	// CONFIG_SERIAL_REQ
		/*
		 * Message description:
		 *             Sender__ Receiver
		 * 0B 77 A0 01 63 19 63 01 02 04 00 09
		 */
		sendINFO_SERIAL();																				// jump to create the answer

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PAIR_SERIAL)) {	// PAIR_SERIAL
		/*
		 * Message description:
		 *             Sender__ Receiver       SerialNumber
		 * 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
		 */

		// compare serial and send device info
		if (compArray(rv.buf+12,HMSR,10)) {
			sendDEVICE_INFO();
		}

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_STATUS_REQUEST)) { // CONFIG_STATUS_REQUEST
		/*
		 * Message description:
		 *             Sender__ Receiver Channel
		 * 15 93 B4 01 63 19 63 00 00 00 01      0E
		 */

		// check if a module is registered and send the information, otherwise report an empty status
		if (modTbl[by10].cnl) {
			modTbl[by10].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.mBdy.pyLd, rv.mBdy.mLen-11);
		} else {
			sendINFO_ACTUATOR_STATUS(rv.mBdy.by10, 0, 0);	
		}

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_ACK)) {			// ACK
		/*
		 * Message description:
		 *             Sender__ Receiver ACK
		 * 0A 05 80 02 63 19 63 01 02 04 00
		 */
	
		if ((sn.active) && (rv.mBdy.mCnt == sn.lastMsgCnt)) {
			sn.retrCnt = 0xFF;																					// was an ACK to an active message, message counter is similar - set retrCnt to 255
		}
		//dbg << "act:" << sn.active << " rC:" << rv.mBdy.mLen << " sC:" << sn.lastMsgCnt << " cntr:" << sn.retrCnt << '\n';

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_ACK_STATUS)) {	// ACK_STATUS
		/*
		 * Message description:
		 *             Sender__ Receiver ACK Channel State Action RSSI
		 * 0E 08 80 02 1F B7 4A 23 70 D8 01  01      C8    80     27
		 *
		 * Action: Down=0x20, UP=0x10, LowBat=&0x80
		 */
	
		if ((sn.active) && (rv.mBdy.mLen == sn.lastMsgCnt)) {
			sn.retrCnt = 0xFF;																					// was an ACK to an active message, message counter is similar - set retrCnt to 255
		}

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_ACK2)) {			// ACK2
		// TODO: Make ready
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_AES_CHALLANGE)) {// ACK_PROC AES-Challenge
		// TODO: Make ready

		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------
		//Para1          => "02,4",
		//Para2          => "06,4",
		//Para3          => "10,4",
		//Para4          => "14,2",}}, # remote?

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_NACK)) {			// NACK
		// TODO: Make ready

		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// for test
		//static uint8_t x2[2];
		//x2[0] = 0x02;
		//x2[1] += 1;
		//sendREMOTE(1, 1, x2);

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE) && (rv.mBdy.by10 == AS_RESPONSE_TYPE_NACK_TARGET_INVALID)) {	// NACK_TARGET_INVALID
		// TODO: Make ready

	#ifdef SUPPORT_AES
		} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_RESPONSE_AES)) {											// AES Response
			/*
			 * Message description:
			 *             Sender__ Receiver AES-Response-Data
			 * 0E 08 80 02 1F B7 4A 23 70 D8 6E 55 89 7F 12 6E 63 55 15 FF 54 07 69 B3 D8 A5
			 */
			uint8_t pBuf[16];																					// We need a 16 bytes buffer
			memcpy(pBuf, rv.buf+10, 16);

			if (checkPayloadDecrypt(pBuf, rv.prevBuf)) {														// check the decrypted result of previous received message
				if (hmKeyIndex > 2) {																			// hmKeyIndex > 2: we have the two key parts
					hmKeyIndex = 0;
					// todo: here we must save the new key (newHmKey)
					dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << "\n";
				} else {

					// todo: here we must trigger action was requestes AES sign
				}
			}

		} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_KEY_EXCHANGE)) {											// AES Key Exchange
			/*
			 * Message description:
			 *             Sender__ Receiver Decrypted Payload with one keypart
			 * 0E 08 80 02 1F B7 4A 23 70 D8 81 78 5C 37 30 65 61 93 1A 63 CF 90 44 31 60 4D
			 */

			uint8_t pBuf[16];																					// We need a 16 bytes buffer
			memcpy(pBuf, rv.buf+10, 16);

			aes128_init(HMKEY, &ctx);																			// load HMKEY
			aes128_dec(pBuf, &ctx);																				// decrypt payload width HMKEY first time

			dbg << F("dec Buffer: ") << _HEX(pBuf, 16) << "\n";

			if (pBuf[0] == 0x01) {																				// the decrypted data must start with 0x01
				hmKeyIndex = pBuf[1];
				dbg << F("hmKeyIndex: ") << _HEXB(hmKeyIndex) << "\n";

				uint8_t index = (hmKeyIndex > 2) ? 8 : 0;
				memcpy(newHmKey+index, pBuf+2, 8);
//				memcpy(newHmKey+((hmKeyIndex - 2)*8), pBuf+2, 8);

				dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << "\n";

				memcpy(rv.prevBuf, rv.buf, rv.buf[0]+1);													// remember this message
				sendSignRequest();

			} else {
				hmKeyIndex = 0;
			}
	#endif

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x02)) {			// SET
		// description --------------------------------------------------------
		//                                      cnl  stat  ramp   dura
		// l> 0E 5E B0 11 63 19 63 1F B7 4A 02  01   C8    00 00  00 00
		// l> 0E 5E 80 02 1F B7 4A 63 19 63 01 01 C8 80 41 
		// do something with the information ----------------------------------

		if (modTbl[rv.mBdy.by11-1].cnl) {
			modTbl[rv.mBdy.by11-1].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+12, rv.mBdy.mLen-11);
		}
		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x03)) {			// STOP_CHANGE
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x04) && (rv.mBdy.by11 == 0x00)) {	// RESET
		// description --------------------------------------------------------
		//
		// l> 0B 1C B0 11 63 19 63 1F B7 4A 04 00 (234116)
		// l> 0E 1C 80 02 1F B7 4A 63 19 63 01 01 00 80 14 (234243)
		// do something with the information ----------------------------------

		ee.clearPeers();
		ee.clearRegs();
		ee.getMasterID();
		ld.set(defect);

		uint8_t xI = ee.getRegListIdx(1,3);
		if (rv.ackRq) {
			if (xI == 0xFF) {
				sendACK();
			} else {
				sendACK_STATUS(0, 0, 0);
			}
		}
		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x80)) {				// LED
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x81) && (rv.mBdy.by11 == 0x00)) {	// LEDALL
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x81)) {				// LEVEL
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SET) && (rv.mBdy.by10 == 0x82)) {				// SLEEPMODE
		// description --------------------------------------------------------
		//
		// b>
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------


	} else if  (rv.mBdy.mTyp == AS_MESSAGE_TYPE_HAVE_DATA) {									// HAVE_DATA
		// description --------------------------------------------------------
		//
		// b> 
		// do something with the information ----------------------------------

		// --------------------------------------------------------------------

	} else if  (rv.mBdy.mTyp >= 0x3E) {															// 3E SWITCH, 3F TIMESTAMP, 40 REMOTE, 41 SENSOR_EVENT, 53 SENSOR_DATA, 58 CLIMATE_EVENT, 70 WEATHER_EVENT
		// description --------------------------------------------------------
		//                 from      to        cnl  cnt
		// p> 0B 2D B4 40  23 70 D8  01 02 05  06   05 - Remote
		// do something with the information ----------------------------------

		//                 from      to        dst       na  cnl  cnt
		// m> 0F 18 B0 3E  FD 24 BE  01 02 05  23 70 D8  40  06   00 - Switch
		//"3E"          => { txt => "SWITCH"      , params => {
		//				DST      => "00,6",
		//				UNKNOWN  => "06,2",
		//				CHANNEL  => "08,2",
		//				COUNTER  => "10,2", } },

		uint8_t cnl = 0, pIdx, tmp;
		
		// check if we have the peer in the database to get the channel
		if ((rv.mBdy.mTyp == AS_MESSAGE_TYPE_SWITCH_EVENT) && (rv.mBdy.mLen == 0x0f)) {
			tmp = rv.buf[13];																// save byte13, because we will replace it
			rv.buf[13] = rv.buf[14];														// copy the channel byte to the peer
			cnl = ee.isPeerValid(rv.buf+10);												// check with the right part of the string
			if (cnl) pIdx = ee.getIdxByPeer(cnl, rv.buf+10);								// get the index of the respective peer in the channel store
			rv.buf[13] = tmp;																// get it back

		} else {
			cnl = ee.isPeerValid(rv.peerId);
			if (cnl) pIdx = ee.getIdxByPeer(cnl, rv.peerId);								// get the index of the respective peer in the channel store
			
		}
		//dbg << "cnl: " << cnl << " pIdx: " << pIdx << " mTyp: " << _HEXB(rv.mBdy.mTyp) << " by10: " << _HEXB(rv.mBdy.by10)  << " by11: " << _HEXB(rv.mBdy.by11) << " data: " << _HEX((rv.buf+10),(rv.mBdy.mLen-9)) << '\n'; _delay_ms(100);
		if (cnl == 0) return;
		
		// check if a module is registered and send the information, otherwise report an empty status
		if (modTbl[cnl-1].cnl) {

			//dbg << "pIdx:" << pIdx << ", cnl:" << cnl << '\n';
			ee.getList(cnl, modTbl[cnl-1].lst, pIdx, modTbl[cnl-1].lstPeer);				// get list3 or list4 loaded into the user module
			
			// call the user module
			modTbl[cnl-1].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+10, rv.mBdy.mLen-9);

		} else {
			sendACK();

		}
		// --------------------------------------------------------------------

	}

}

// - send functions --------------------------------
void AS::sendINFO_SERIAL(void) {
	// description --------------------------------------------------------
	// l> 0B 77 A0 01 63 19 63 1E 7A AD 00 09
	//                reID      toID     by10  serial
	// l> 14 77 80 10 1E 7A AD  63 19 63 00    4A 45 51 30 37 33 31 39 30 35
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 0x14;
//	sn.mBdy.mCnt = rv.mBdy.mLen;
//	sn.mBdy.mTyp = 0x10;
//	memcpy(sn.mBdy.reID,HMID,3);
//	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = 0x00;
	memcpy(sn.buf+11, HMSR, 10);
	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_TYPE_INFO, rv.mBdy.reID);
}

void AS::sendINFO_PEER_LIST(uint8_t len) {
	// description --------------------------------------------------------
	// l> 0B 44 A0 01 63 19 63 1F B7 4A 01 03
	//                reID      toID     by10  peer1        peer2
	// l> 1A 44 A0 10 1F B7 4A 63 19 63  01    22 66 08 02  22 66 08 01  22 66 08 04  22 66 08 03
	//
	// l> 0A 44 80 02 63 19 63 1F B7 4A 00
	// l> 1A 45 A0 10 1F B7 4A 63 19 63 01 24 88 2D 01 24 88 2D 02 24 88 2D 03 24 88 2D 04
	// l> 0A 45 80 02 63 19 63 1F B7 4A 00
	// l> 1A 46 A0 10 1F B7 4A 63 19 63 01 23 70 D8 02 23 70 D8 01 23 70 D8 04 23 70 D8 03
	// l> 0A 46 80 02 63 19 63 1F B7 4A 00
	// l> 1A 47 A0 10 1F B7 4A 63 19 63 01 23 70 D8 06 23 70 D8 05 22 6C 12 02 22 6C 12 01
	// l> 0A 47 80 02 63 19 63 1F B7 4A 00
	// l> 1A 48 A0 10 1F B7 4A 63 19 63 01 23 70 EC 02 23 70 EC 01 23 70 EC 04 23 70 EC 03
	// l> 0A 48 80 02 63 19 63 1F B7 4A 00
	// l> 16 49 A0 10 1F B7 4A 63 19 63 01 23 70 EC 06 23 70 EC 05 00 00 00 00
	// l> 0A 49 80 02 63 19 63 1F B7 4A 00
	// do something with the information ----------------------------------

	sn.mBdy.mLen = len + 10;
//	sn.mBdy.mCnt = stcSlice.mCnt++;
	sn.mBdy.mFlg.BIDI = 1;
//	sn.mBdy.mTyp = 0x10;
//	memcpy(sn.mBdy.reID, HMID, 3);
//	memcpy(sn.mBdy.toID, stcSlice.toID, 3);
	sn.mBdy.by10 = 0x01;																	//stcSlice.cnl;
	prepareToSend(stcSlice.mCnt++, AS_MESSAGE_TYPE_INFO, stcSlice.toID);
}

void AS::sendINFO_PARAM_RESPONSE_PAIRS(uint8_t len) {
	// description --------------------------------------------------------
	// l> 10 79 B0 01 63 19 63 01 02 04 00 04 00 00 00 00 00
	//                reID      toID      by10  reg  data  reg  data
	// l> 16 79 A0 10 01 02 04  63 19 63  02    02   01    05   40 0A 63 0B 19 0C 63 12 69
	//
	// l> 0A 79 80 02 63 19 63 01 02 04 00
	// l> 0C 7A A0 10 01 02 04 63 19 63 02 00 00
	// l> 0A 7A 80 02 63 19 63 01 02 04 00
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 10+len;
//	sn.mBdy.mCnt = stcSlice.mCnt++;
	sn.mBdy.mFlg.BIDI = 1;
//	sn.mBdy.mTyp = 0x10;
//	memcpy(sn.mBdy.reID, HMID, 3);
//	memcpy(sn.mBdy.toID, stcSlice.toID, 3);
	sn.mBdy.by10 = (len < 3) ? 0x03 : 0x02;
	prepareToSend(stcSlice.mCnt++, AS_MESSAGE_TYPE_INFO, stcSlice.toID);
}

/**
 * Set message type, sender and receiver address
 * and set sn.active, so the message should send next time
 */
void AS::prepareToSend(uint8_t mCnt, uint8_t mTyp, uint8_t *addrTo) {
	sn.mBdy.mCnt = mCnt;
	sn.mBdy.mTyp = mTyp;
	memcpy(sn.mBdy.reID, HMID, 3);
	memcpy(sn.mBdy.toID, addrTo, 3);
	sn.active = 1;																			// remember to fire the message
}

void AS::sendINFO_PARAM_RESPONSE_SEQ(uint8_t len) {
	// description --------------------------------------------------------
	// l> 10 90 A0 01 63 19 63 01 02 04 01 04 24 88 2D 03 03
	//                reID      toID      by10  Offset  Data
	// l> 16 90 A0 10 01 02 04  63 19 63  03    02      00 00 32 64 00 FF 00 FF 01 13 33
	//
	// l> 0A 90 80 02 63 19 63 01 02 04 00
	// l> 16 91 A0 10 01 02 04 63 19 63 03 82 00 00 32 64 00 FF 00 FF 21 13 33
	// l> 0A 91 80 02 63 19 63 01 02 04 00
	// l> 0C 92 A0 10 01 02 04 63 19 63 03 00 00
	// l> 0A 92 80 02 63 19 63 01 02 04 00
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}

void AS::sendINFO_PARAMETER_CHANGE(void) {
	//"10;p01=04"   => { txt => "INFO_PARAMETER_CHANGE", params => {
	//CHANNEL => "2,2",
	//PEER    => '4,8,$val=CUL_HM_id2Name($val)',
	//PARAM_LIST => "12,2",
	//DATA => '14,,$val =~ s/(..)(..)/ $1:$2/g', } },
	// --------------------------------------------------------------------
}

// - homematic specific functions ------------------
void AS::decode(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i=2; i<buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xdc) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}

void AS::encode(uint8_t *buf) {
	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i=2; i<buf[0]; i++) {
		prev = (prev + 0xdc) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}

#ifdef RV_DBG_EX																			// only if extended AS debug is set
	void AS::explainMessage(uint8_t *buf) {
		dbg << F("   ");																	// save some byte and send 3 blanks once, instead of having it in every if

		if        ((buf[3] == 0x00)) {
			dbg << F("DEVICE_INFO; fw: ") << _HEX((buf+10),1) << F(", type: ") << _HEX((buf+11),2) << F(", serial: ") << _HEX((buf+13),10) << '\n';
			dbg << F("              , class: ") << _HEXB(buf[23]) << F(", pCnlA: ") << _HEXB(buf[24]) << F(", pCnlB: ") << _HEXB(buf[25]) << F(", na: ") << _HEXB(buf[26]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x01)) {
			dbg << F("CONFIG_PEER_ADD; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnlA: ") << _HEXB(buf[15]) << F(", pCnlB: ") << _HEXB(buf[16]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x02)) {
			dbg << F("CONFIG_PEER_REMOVE; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnlA: ") << _HEXB(buf[15]) << F(", pCnlB: ") << _HEXB(buf[16]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x03)) {
			dbg << F("CONFIG_PEER_LIST_REQ; cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x04)) {
			dbg << F("CONFIG_PARAM_REQ; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnl: ") << _HEXB(buf[15]) << F(", lst: ") << _HEXB(buf[16]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x05)) {
			dbg << F("CONFIG_START; cnl: ") << _HEXB(buf[10]) << F(", peer: ") << _HEX((buf+12),3) << F(", pCnl: ") << _HEXB(buf[15]) << F(", lst: ") << _HEXB(buf[16]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x06)) {
			dbg << F("CONFIG_END; cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x08)) {
			dbg << F("CONFIG_WRITE_INDEX; cnl: ") << _HEXB(buf[10]) << F(", data: ") << _HEX((buf+12),(buf[0]-11));

		} else if ((buf[3] == 0x01) && (buf[11] == 0x09)) {
			dbg << F("CONFIG_SERIAL_REQ");

		} else if ((buf[3] == 0x01) && (buf[11] == 0x0A)) {
			dbg << F("PAIR_SERIAL, serial: ") << _HEX((buf+12),10);

		} else if ((buf[3] == 0x01) && (buf[11] == 0x0E)) {
			dbg << F("CONFIG_STATUS_REQUEST, cnl: ") << _HEXB(buf[10]);

		} else if ((buf[3] == 0x02) && (buf[10] == 0x00)) {
			if (buf[0] == 0x0A) dbg << F("ACK");
			else dbg << F("ACK; data: ") << _HEX((buf+11),buf[0]-10);

		} else if ((buf[3] == 0x02) && (buf[10] == 0x01)) {
			dbg << F("ACK_STATUS; cnl: ") << _HEXB(buf[11]) << F(", status: ") << _HEXB(buf[12]) << F(", down/up/loBat: ") << _HEXB(buf[13]);
			if (buf[0] > 13) dbg << F(", rssi: ") << _HEXB(buf[14]);

		} else if ((buf[3] == 0x02) && (buf[10] == 0x02)) {
			dbg << F("ACK2");

		} else if ((buf[3] == 0x02) && (buf[10] == 0x04)) {
			dbg << F("ACK_PROC; para1: ") << _HEX((buf+11),2) << F(", para2: ") << _HEX((buf+13),2) << F(", para3: ") << _HEX((buf+15),2) << F(", para4: ") << _HEXB(buf[17]);

		} else if ((buf[3] == 0x02) && (buf[10] == 0x80)) {
			dbg << F("NACK");

		} else if ((buf[3] == 0x02) && (buf[10] == 0x84)) {
			dbg << F("NACK_TARGET_INVALID");

		} else if ((buf[3] == 0x03)) {
			dbg << F("AES_REPLY; data: ") << _HEX((buf+10),buf[0]-9);

		} else if ((buf[3] == 0x04) && (buf[10] == 0x01)) {
			dbg << F("TO_HMLAN:SEND_AES_CODE; cnl: ") << _HEXB(buf[11]);

		} else if ((buf[3] == 0x04)) {
			dbg << F("TO_ACTOR:SEND_AES_CODE; code: ") << _HEXB(buf[11]);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x00)) {
			dbg << F("INFO_SERIAL; serial: ") << _HEX((buf+11),10);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x01)) {
			dbg << F("INFO_PEER_LIST; peer1: ") << _HEX((buf+11),4);
			if (buf[0] >= 19) dbg << F(", peer2: ") << _HEX((buf+15),4);
			if (buf[0] >= 23) dbg << F(", peer3: ") << _HEX((buf+19),4);
			if (buf[0] >= 27) dbg << F(", peer4: ") << _HEX((buf+23),4);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x02)) {
			dbg << F("INFO_PARAM_RESPONSE_PAIRS; data: ") << _HEX((buf+11),buf[0]-10);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x03)) {
			dbg << F("INFO_PARAM_RESPONSE_SEQ; offset: ") << _HEXB(buf[11]) << F(", data: ") << _HEX((buf+12),buf[0]-11);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x04)) {
			dbg << F("INFO_PARAMETER_CHANGE; cnl: ") << _HEXB(buf[11]) << F(", peer: ") << _HEX((buf+12),4) << F(", pLst: ") << _HEXB(buf[16]) << F(", data: ") << _HEX((buf+17),buf[0]-16);

		} else if ((buf[3] == 0x10) && (buf[10] == 0x06)) {
			dbg << F("INFO_ACTUATOR_STATUS; cnl: ") << _HEXB(buf[11]) << F(", status: ") << _HEXB(buf[12]) << F(", na: ") << _HEXB(buf[13]);
			if (buf[0] > 13) dbg << F(", rssi: ") << _HEXB(buf[14]);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x02)) {
			dbg << F("SET; cnl: ") << _HEXB(buf[11]) << F(", value: ") << _HEXB(buf[12]) << F(", rampTime: ") << _HEX((buf+13),2) << F(", duration: ") << _HEX((buf+15),2);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x03)) {
			dbg << F("STOP_CHANGE; cnl: ") << _HEXB(buf[11]);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x04) && (buf[11] == 0x00)) {
			dbg << F("RESET");

		} else if ((buf[3] == 0x11) && (buf[10] == 0x80)) {
			dbg << F("LED; cnl: ") << _HEXB(buf[11]) << F(", color: ") << _HEXB(buf[12]);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x81) && (buf[11] == 0x00)) {
			dbg << F("LED_ALL; Led1To16: ") << _HEX((buf+12),4);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x81)) {
			dbg << F("LED; cnl: ") << _HEXB(buf[11]) << F(", time: ") << _HEXB(buf[12]) << F(", speed: ") << _HEXB(buf[13]);

		} else if ((buf[3] == 0x11) && (buf[10] == 0x82)) {
			dbg << F("SLEEPMODE; cnl: ") << _HEXB(buf[11]) << F(", mode: ") << _HEXB(buf[12]);

		} else if ((buf[3] == 0x12)) {
			dbg << F("HAVE_DATA");

		} else if ((buf[3] == 0x3E)) {
			dbg << F("SWITCH; dst: ") << _HEX((buf+10),3) << F(", na: ") << _HEXB(buf[13]) << F(", cnl: ") << _HEXB(buf[14]) << F(", counter: ") << _HEXB(buf[15]);

		} else if ((buf[3] == 0x3F)) {
			dbg << F("TIMESTAMP; na: ") << _HEX((buf+10),2) << F(", time: ") << _HEX((buf+12),2);

		} else if ((buf[3] == 0x40)) {
			dbg << F("REMOTE; button: ") << _HEXB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", counter: ") << _HEXB(buf[11]);

		} else if ((buf[3] == 0x41)) {
			dbg << F("SENSOR_EVENT; button: ") <<_HEXB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", value: ") << _HEXB(buf[11]) << F(", next: ") << _HEXB(buf[12]);

		} else if ((buf[3] == 0x53)) {
			dbg << F("SENSOR_DATA; cmd: ") << _HEXB(buf[10]) << F(", fld1: ") << _HEXB(buf[11]) << F(", val1: ") << _HEX((buf+12),2) << F(", fld2: ") << _HEXB(buf[14]) << F(", val2: ") << _HEX((buf+15),2) << F(", fld3: ") << _HEXB(buf[17]) << F(", val3: ") << _HEX((buf+18),2) << F(", fld4: ") << _HEXB(buf[20]) << F(", val4: ") << _HEX((buf+21),2);

		} else if ((buf[3] == 0x58)) {
			dbg << F("CLIMATE_EVENT; cmd: ") << _HEXB(buf[10]) << F(", valvePos: ") << _HEXB(buf[11]);

		} else if ((buf[3] == 0x70)) {
			dbg << F("WEATHER_EVENT; temp: ") << _HEX((buf+10),2) << F(", hum: ") << _HEXB(buf[12]);

		} else {
			dbg << F("Unknown Message, please report!");
		}
		dbg << F("\n\n");
	}
#endif

// - AES Signing related methods -------------------

#ifdef SUPPORT_AES
	void AS::sendSignRequest(void) {
		// description --------------------------------------------------------
		//                reID      toID      SigningRequest Challange
		// l> 11 24 80 02 1F B7 4A  63 19 63  04             XX XX XX XX XX XX 00

		sn.mBdy.mLen = 0x11;
		sn.mBdy.mCnt = rv.mBdy.mCnt;
		sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
		sn.mBdy.mTyp = AS_MESSAGE_TYPE_RESPONSE;
		memcpy(sn.mBdy.reID, HMID, 3);
		memcpy(sn.mBdy.toID, rv.mBdy.reID, 3);
		sn.mBdy.by10 = AS_RESPONSE_TYPE_AES_CHALLANGE;								// AES Challenge

		uint8_t pBuf[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};				// we need 7 bytes
		getRandomBytes(pBuf, 6);													// but only 6 bytes becomes random data
		memcpy(sn.buf+11, pBuf, 7);													// the 7th byte must be 0x00

		makeTmpKey(pBuf);															// here we make the temporarly key with the challange and the old known hm key

		dbg << F(">>> signingRequestData  : ") << _HEX(sn.buf+10, 7) << F(" <<<") << "\n";
		sn.active = 1;																			// fire the message
	}

	void AS::makeTmpKey(uint8_t *challenge) {
		memcpy(this->tempHmKey, HMKEY, 16);

		for (uint8_t i = 0; i < 6; i++) {
			this->tempHmKey[i] = HMKEY[i] ^ challenge[i];
		}
		aes128_init(this->tempHmKey, &ctx);											// generating the round keys from the 128 bit key
	}

	void AS::payloadEncrypt(uint8_t *encPayload, uint8_t *msgToEnc) {
		uint8_t iv[16] = {															// initial vector
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		uint32_t mill = millis();
		encPayload[0] = 0;
		encPayload[1] = 0;
		encPayload[2] = (mill & 0xFF000000) >> 24;									// first 6 bytes of payload probably is a counter
		encPayload[3] = (mill & 0x00FF0000) >> 16;									// we store current millis in bytes 2-5 of payload
		encPayload[4] = (mill & 0x0000FF00) >> 8;
		encPayload[5] = (mill & 0x000000FF);

		memcpy(encPayload+6, msgToEnc+1, ((msgToEnc[0] < 10) ? 9 : 10) );			// build payload to encrypt
		memcpy(iv, msgToEnc+11, msgToEnc[0]-10);									// build initial vector

		aes128_enc(encPayload, &ctx);												// encrypt payload width tmpKey first time
		for (uint8_t i = 0; i < 16; i++)	encPayload[i] ^= iv[i];					// xor encrypted payload with iv
		aes128_enc(encPayload, &ctx);												// encrypt payload width tmpKey again
	}

	uint8_t AS::checkPayloadDecrypt (uint8_t *data, uint8_t *msgOriginal) {
		uint8_t authAck[4];
		uint8_t iv[16] = {															// initial vector
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		memcpy(iv, msgOriginal+11, msgOriginal[0]-10);								// build initial vector
	//	dbg << F(">>> key   : ") << _HEX(HMKEY, 16) << F(" <<<") << "\n";
	//	dbg << F(">>> keyTmp: ") << _HEX(tempHmKey, 16) << F(" <<<") << "\n";
	//	dbg << F(">>> iv    : ") << _HEX(iv, 16) << F(" <<<") << "\n";

		aes128_dec(data, &ctx);														// decrypt payload width tmpKey first time
//		dbg << F(">>> plDec : ") << _HEX(data, 16) << F(" <<<") << "\n";

		for (uint8_t i = 0; i < 16; i++) data[i] ^= iv[i];							// xor encrypted payload with iv
//		dbg << F(">>> plDec^: ") << _HEX(data, 16) << F(" <<<") << "\n";

		memcpy(authAck, data, 4);													// build auth ACK
//		dbg << F(">>> ack   : ") << _HEX(authAck, 4) << F(" <<<") << "\n";

		aes128_dec(data, &ctx);														// decrypt payload width tmpKey again

//		dbg << F(">>> plD^D: ") << _HEX(data, 6) << " | "<< _HEX(data+6, 10) << F(" <<<") << "\n";

//		dbg << F("Check AES Response ...\n");
		dbg << F(">>> compare: ") << _HEX(data, 16) << " | "<< _HEX(msgOriginal, 11) << "\n";

		// memcmp returns 0 if compare true
		 if (!memcmp(data+6, msgOriginal+1, 10)) {									// compare bytes 7-17 of decrypted data with bytes 2-12 of msgOriginal
			 sendAckAES(authAck);													// send AES-Ack
			 return 1;

		 } else {
			 return 0;
		 }
	}

	void AS::sendSignResponse(void) {
		sn.mBdy.mLen = 0x19;
		sn.mBdy.mCnt = rv.mBdy.mLen;
		sn.mBdy.mTyp = AS_MESSAGE_TYPE_RESPONSE_AES;
		memcpy(sn.mBdy.reID,HMID,3);
		memcpy(sn.mBdy.toID,rv.mBdy.reID,3);

		uint8_t challenge[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};				// challenge
		memcpy(challenge, rv.mBdy.pyLd, 6);											// get challenge
		this->makeTmpKey(challenge);

		uint8_t payload[16];
	//	this->payloadEncrypt(payload, sn.msgPartToSign);

		sn.mBdy.by10 = payload[0];
		memcpy(sn.buf+10, payload, 16);
		sn.active = 1;																// fire the message
	}

	/**
	 * get number of random bytes
	 */
	void AS::getRandomBytes(uint8_t *buffer, uint8_t length) {
		uint8_t i = 0;
		srand(this->randomSeed ^ uint16_t (millis() & 0xFFFF));
		for (i =0; i < length; i++) {
			buffer[i] = rand() % 0xFF;
		}
	}

	void AS::initRandomSeed() {
		uint16_t *p = (uint16_t*) (RAMEND+1);
		extern uint16_t __heap_start;
		while (p >= &__heap_start + 1) {
			this->randomSeed ^= * (--p);
		}
	}
#endif

// - some helpers ----------------------------------
// public:		//---------------------------------------------------------------------------------------------------------

/**
 * @brief Query if the timer has expired
 *
 * @return 0 if timer is still running, 1 if not.
 *         If the timer was never set(), return value is 1
 */
uint8_t  waitTimer::done(void) {
	if (!armed) return 1;																	// not armed, so nothing to do
	if ( (getMillis() - startTime) < checkTime ) return 0;									// not ready yet

	checkTime = armed = 0;																	// if we are here, timeout was happened, next loop status 1 will indicated
	return 1;
}

/**
 * @brief Start the timer
 *
 * @param ms Time until timer is done() (unit: ms)
 */
void     waitTimer::set(uint32_t ms) {
	armed = ms ? 1 : 0;
	if (armed) {
		startTime = getMillis();
		checkTime = ms;
	}
}

/**
 * @brief Query the remaing time until the timer is done
 *
 * @return Time until timer is done() (unit: ms)
 */
uint32_t waitTimer::remain(void) {
	if (!armed) return 0;
	return (checkTime - (getMillis() - startTime));
}

uint32_t byteTimeCvt(uint8_t tTime) {
	const uint16_t c[8] = {1,10,50,100,600,3000,6000,36000};
	return (uint32_t)(tTime & 0x1f) * c[tTime >> 5]*100;
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
