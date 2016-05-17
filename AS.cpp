//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de>, <dirk@hfma.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

/*
 * Comment out to disable AES support
 */
 #define SUPPORT_AES

/*
 * On device reset the watchdog hart reset the entire device.
 * Comment out to disable this.
 */
#define WDT_RESET_ON_RESET

//#define AS_DBG
//#define RV_DBG_EX
//#define AES_DBG

#include "AS.h"
#include <avr/wdt.h>

#ifdef SUPPORT_AES
	#include "aes.h"

	aes128_ctx_t ctx; 																			// the context where the round keys are stored
#endif

waitTimer cnfTmr;																				// config timer functionality
waitTimer pairTmr;																				// pair timer functionality

// public:		//---------------------------------------------------------------------------------------------------------
AS::AS() {
}

/**
 * @brief Initialize the AskSin Module
 */
void AS::init(void) {
	#ifdef AS_DBG																				// only if cc debug is set
		dbgStart();																				// serial setup
		dbg << F("AS.\n");																		// ...and some information
	#endif

	//initLeds();																					// initialize the leds
	initConfKey();																				// initialize the port for getting config key interrupts

	ee.init();																					// eeprom init
	cc.init();																					// init the rf module

	memcpy_P(HMID, HMSerialData+0, 3);															// set HMID from pgmspace
	memcpy_P(HMSR, HMSerialData+3, 10);															// set HMSerial from pgmspace

	sn.init(this);																				// send module
	rv.init(this);																				// receive module
	rg.init(this);																				// module registrar
	ld.init(this);																				// led class initialization
	confButton.init(this);																		// config button
	pw.init(this);																				// power management
	bt.init(this);																				// battery check
	
	initMillis();																				// start the millis counter

	initRandomSeed();

	// everything is setuped, enable RF functionality
}

/**
 * @brief Cyclic poll all related functions
 */
void AS::poll(void) {
	if (ccGetGDO0()) {																			// check if something was received
		cc.rcvData(rv.buf);																		// copy the data into the receiver module
		if (rv.hasData) decode(rv.buf);															// decode the string
	}

	// handle send and receive buffer
	if (rv.hasData) rv.poll();																	// check if there is something in the received buffer
	if (sn.active) sn.poll();																	// check if there is something to send

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
	if (cFlag.active) {																			// check only if we are still in config mode
		if (cnfTmr.done()) cFlag.active = 0;													// when timer is done, set config flag to inactive
	}

	// time out the pairing timer
	if (pairActive) { 
		if (pairTmr.done()) {
			pairActive = 0;
			isEmpty(MAID, 3)? ld.set(pair_err) : ld.set(pair_suc);	
		}
	}

	// regular polls
	rg.poll();																					// poll the channel module handler
	confButton.poll();																			// poll the config button
	ld.poll();																					// poll the led's
	bt.poll();																					// poll the battery check
		
	// check if we could go to standby
	pw.poll();																					// poll the power management
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
	if ((rv.mBdy.mTyp == AS_MESSAGE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_PAIR_SERIAL)) {
		msgCount = rv.mBdy.mLen;																// send counter - is it an answer or a initial message
	} else {
		msgCount = sn.msgCnt++;
	}

	sn.mBdy.mLen = 0x1A;
	sn.mBdy.mFlg.CFG = 1;
	sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3)) ? 0 : 1;

	memcpy_P(sn.buf+10, devDef.devIdnt, 3);
	memcpy(sn.buf+13, HMSR, 10);
	memcpy_P(sn.buf+23, devDef.devIdnt+3, 4);

	prepareToSend(msgCount, AS_MESSAGE_DEVINFO, MAID);

	pairActive = 1;																				// set pairing flag
	pairTmr.set(20000);																			// set pairing time
	ld.set(pairing);																			// and visualize the status
}

/**
 * @brief Check if ACK required and send ACK or NACK
 */
void AS::checkSendACK(uint8_t ackOk) {
	if (rv.ackRq) {
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
	if (rv.mBdy.mFlg.BIDI) {																	// prevent answer for requests from a user class on repeated key press
		sn.mBdy.mLen = 0x0A;
		sn.mBdy.mFlg.CFG = 0;
		sn.mBdy.mFlg.BIDI = 0;
		sn.mBdy.by10 = 0x00;
		prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
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
	sn.mBdy.mLen = 0x0A;
	sn.mBdy.by10 = AS_RESPONSE_NACK;
	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
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
		sn.mBdy.mLen = 0x0E;
		sn.mBdy.mFlg.BIDI = 0;
		sn.mBdy.by10 = AS_RESPONSE_ACK;
		sn.mBdy.by11 = data[0];
		memcpy(sn.mBdy.pyLd, data+1, 3);

		prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
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
	if (rv.mBdy.mFlg.BIDI) {																	// prevent answer for requests from a user class on repeated key press
		sn.mBdy.mLen      = 0x0E;
		sn.mBdy.mFlg.BIDI = 0;
		sn.mBdy.by10      = 0x01;
		sn.mBdy.by11      = channel;
		sn.mBdy.pyLd[0]   = state;
		sn.mBdy.pyLd[1]   = action | (bt.getStatus() << 7);
		sn.mBdy.pyLd[2]   = cc.rssi;
		prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
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
	sn.mBdy.mLen = 0x0A;
	sn.mBdy.by10 = AS_RESPONSE_NACK_TARGET_INVALID;
	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
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
	sn.mBdy.mLen = 0x0E;
	uint8_t cnt = sn.msgCnt++;

	if ((rv.mBdy.mTyp == AS_MESSAGE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_STATUS_REQUEST)) {
		cnt = rv.mBdy.mCnt;
	}

	sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
	sn.mBdy.by10      = AS_INFO_ACTUATOR_STATUS;
	sn.mBdy.by11      = channel;
	sn.mBdy.pyLd[0]   = state;
	sn.mBdy.pyLd[1]   = flag; // | (bt.getStatus() << 7);
	sn.mBdy.pyLd[2]   = cc.rssi;

	prepareToSend(cnt, AS_MESSAGE_INFO, MAID);
}

void AS::sendINFO_POWER_EVENT(uint8_t *data) {
	sn.mBdy.mLen = 15; // 15, 16 works somehow but 12 + 6 = 18
	uint8_t cnt;

	if ((rv.mBdy.mTyp == AS_MESSAGE_CONFIG) && (rv.mBdy.by11 == AS_CONFIG_STATUS_REQUEST)) {
		cnt = rv.mBdy.mCnt;
	} else {
		cnt = sn.msgCnt++;
	}
	#ifdef AS_DBG
		Serial << F("sendINFO_POWER_EVENT cnt: ");
		Serial.print(cnt, DEC);
	#endif

	//char* myBytes = reinterpret_cast<char*>(sn.mBdy.pyLd);


	sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3))?0:1;
	//sn.mBdy.by10      = AS_MESSAGE_POWER_EVENT_CYCLIC;

	// set payload
	sn.mBdy.by10      = data[0];
	sn.mBdy.by11      = data[1]; // first byte of payload
	for (uint8_t i = 2; i < 6; i++){
		//dbg << "AS::sendINFO_POWER_EVENT BYTES: ("<< i <<" " << _HEXB(myBytes[i]) << " = " <<_HEXB(sn.mBdy.pyLd[i]) << "\n";
		sn.mBdy.pyLd[i-2] = data[i];
	}
	//sn.mBdy.pyLd[0]   = state;
	//sn.mBdy.pyLd[1]   = flag; // | (bt.getStatus() << 7);
	//sn.mBdy.pyLd[2]   = cc.rssi;
	#ifdef AS_DBG
		Serial << F(" BIDI: ") << sn.mBdy.mFlg.BIDI << "\n";
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

	if (sn.active) return;																		// check if send function has a free slot, otherwise return

	if        (stcSlice.peer) {			// INFO_PEER_LIST
		cnt = ee.getPeerListSlc(stcSlice.cnl, stcSlice.curSlc, sn.buf+11);						// get the slice and the amount of bytes
		sendINFO_PEER_LIST(cnt);																// create the body
		stcSlice.curSlc++;																		// increase slice counter
		//dbg << "peer slc: " << _HEX(sn.buf,sn.buf[0]+1) << '\n';								// write to send buffer

	} else if (stcSlice.reg2) {			// INFO_PARAM_RESPONSE_PAIRS
		cnt = ee.getRegListSlc(stcSlice.cnl, stcSlice.lst, stcSlice.idx, stcSlice.curSlc, sn.buf+11); // get the slice and the amount of bytes
		//dbg << "cnt: " << cnt << '\n';
		sendINFO_PARAM_RESPONSE_PAIRS(cnt);
		stcSlice.curSlc++;																		// increase slice counter
		//dbg << "reg2 slc: " << _HEX(sn.buf,sn.buf[0]+1) << '\n';								// write to send buffer
		
	} else if (stcSlice.reg3) {																	// INFO_PARAM_RESPONSE_SEQ

	}

	if (stcSlice.curSlc == stcSlice.totSlc) {													// if everything is send, we could empty the struct
		memset((void*)&stcSlice, 0, 10);														// by memset
		//dbg << "end: " << stcSlice.active << stcSlice.peer << stcSlice.reg2 << stcSlice.reg3 << '\n';
	}
}

inline void AS::sendPeerMsg(void) {
	uint8_t retries_max;

	retries_max = (stcPeer.bidi) ? 3 : 1;
	
	if (sn.active) return;																		// check if send function has a free slot, otherwise return
	
	// first run, prepare amount of slots
	if (!stcPeer.idx_max) {
		stcPeer.idx_max = ee.getPeerSlots(stcPeer.channel);										// get amount of messages of peer channel
	
		if (stcPeer.idx_max == ee.countFreeSlots(stcPeer.channel) ) {							// check if at least one peer exist in db, otherwise send to master and stop function
			preparePeerMessage(MAID, retries_max);
			sn.msgCnt++;																		// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));										// clean out and return
			return;
		}
	}
	
	// all slots of channel processed, start next round or end processing
	if (stcPeer.idx_cur >= stcPeer.idx_max) {													// check if all peer slots are done
		stcPeer.retries++;																		// increase the round counter
		
		if ((stcPeer.retries >= retries_max) || (isEmpty(stcPeer.slot,8))) {					// all rounds done or all peers reached
			//dbg << "through\n";
			sn.msgCnt++;																		// increase the send message counter
			memset((void*)&stcPeer, 0, sizeof(s_stcPeer));										// clean out and return
			
		} else {																				// start next round
			//dbg << "next round\n";
			stcPeer.idx_cur = 0;

		}
		return;

	} else if ((stcPeer.idx_cur) && (!sn.timeOut)) {											// peer index is >0, first round done and no timeout
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

	uint8_t tmp_peer[4];																		// get the respective peer address
	ee.getPeerByIdx(stcPeer.channel, stcPeer.idx_cur, tmp_peer);
	
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
	l4_0x01.ui = ee.getRegAddr(stcPeer.channel, 4, stcPeer.idx_cur, 0x01);
	// fillLvlUpThr    =>{a=>  4.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level upper threshold"},
	// fillLvlLoThr    =>{a=>  5.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level lower threshold"},
	//dbg << F("s_l4_0x01=") << _HEXB(l4_0x01.ui) << F("\n");
	//l4_0x01.ui = 0;		// disable burst - hardcoded
	
	preparePeerMessage(tmp_peer, 1);
	
	if (!sn.mBdy.mFlg.BIDI) {
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

	sn.mBdy.mLen       = stcPeer.len_payload + 9;												// set message length
	sn.mBdy.mFlg.CFG   = 1;
	sn.mBdy.mFlg.BIDI  = stcPeer.bidi;															// message flag
	sn.mBdy.mFlg.BURST = l4_0x01.s.peerNeedsBurst;
	
	prepareToSend(sn.msgCnt, stcPeer.msg_type, xPeer);

	if (sn.mBdy.mTyp == 0x41) {
		sn.mBdy.by10 = stcPeer.channel;
		sn.mBdy.by10 |= (bt.getStatus() << 7);													// battery bit
		memcpy(sn.buf+11, stcPeer.ptr_payload, stcPeer.len_payload);							// payload
		sn.mBdy.mLen++;
	} else {
		memcpy(sn.buf+10, stcPeer.ptr_payload, stcPeer.len_payload);							// payload
	}
	sn.maxRetr = retries;																		// send only one time
}

/**
 * @brief Receive handler: Process received messages
 */
void AS::processMessage(void) {
	uint8_t by10 = rv.mBdy.by10 - 1;

	// check which type of message was received
	if        (rv.mBdy.mTyp == AS_MESSAGE_DEVINFO) {
		//TODO: do something with the information

	} else if (rv.mBdy.mTyp == AS_MESSAGE_CONFIG) {

		if (rv.mBdy.by11        == AS_CONFIG_PEER_LIST_REQ) {
			processMessageConfigPeerListReq();

		} else if (rv.mBdy.by11 == AS_CONFIG_PARAM_REQ) {
			processMessageConfigParamReq();

		} else if (rv.mBdy.by11 == AS_CONFIG_SERIAL_REQ) {
			processMessageConfigSerialReq();

		} else if (rv.mBdy.by11 == AS_CONFIG_PAIR_SERIAL) {
			processMessageConfigPairSerial();

		} else if (rv.mBdy.by11 == AS_CONFIG_STATUS_REQUEST) {
			processMessageConfigStatusRequest(by10);

		} else {
			processMessageConfigAESProtected();
		}

	} else if (rv.mBdy.mTyp == AS_MESSAGE_RESPONSE) {
		/*
		 * This is an response (ACK) to an active message.
		 * In exception of AS_RESPONSE_AES_CHALLANGE message, we set retrCnt to 0xFF
		 */
		if ((sn.active) && (rv.mBdy.mCnt == sn.lastMsgCnt) && (rv.mBdy.by10 != AS_RESPONSE_AES_CHALLANGE)) {
			sn.retrCnt = 0xFF;
		}

		if (rv.mBdy.by10 == AS_RESPONSE_ACK) {
			/*
			 * Message description:
			 *             Sender__ Receiver ACK
			 * 0A 05 80 02 63 19 63 01 02 04 00
			 */
			// nothing to do yet

		} else if (rv.mBdy.by10 == AS_RESPONSE_ACK_STATUS) {
			/*
			 * Message description:
			 *             Sender__ Receiver ACK Channel State Action RSSI
			 * 0E 08 80 02 1F B7 4A 23 70 D8 01  01      C8    80     27
			 *
			 * Action: Down=0x20, UP=0x10, LowBat=&0x80
			 */
			// nothing to do yet

		} else if (rv.mBdy.by10 == AS_RESPONSE_ACK2) {
			// nothing to do yet

		#ifdef SUPPORT_AES
			} else if (rv.mBdy.by10 == AS_RESPONSE_AES_CHALLANGE) {
				processMessageResponseAES_Challenge();

				memcpy(sn.buf+10, sn.msgToSign, 16);
				prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_RESPONSE_AES, rv.mBdy.reID);
		#endif

		} else if (rv.mBdy.by10 == AS_RESPONSE_NACK) {
			// nothing to do yet

		} else if (rv.mBdy.by10 == AS_RESPONSE_NACK_TARGET_INVALID) {
			// nothing to do yet

		}

	#ifdef SUPPORT_AES
		} else if ((rv.mBdy.mTyp == AS_MESSAGE_RESPONSE_AES)) {
			/*
			 * Message description:
			 *             Sender__ Receiver AES-Response-Data
			 * 0E 08 80 02 1F B7 4A 23 70 D8 6E 55 89 7F 12 6E 63 55 15 FF 54 07 69 B3 D8 A5
			 */
			sn.cleanUp();																		// cleanup send module data;

			uint8_t iv[16];																		// 16 bytes initial vector
			memset(iv, 0x00, 16);																// fill IV with 0x00;
			memcpy(iv, rv.prevBuf+11, rv.prevBuf[0]-10);
			aes128_dec(rv.buf+10, &ctx);														// decrypt payload with temporarily key first time

			for (uint8_t i = 0; i < 16; i++) rv.buf[i+10] ^= iv[i];								// xor encrypted payload with iv

			uint8_t authAck[4];
			authAck[0] = rv.buf[10];
			authAck[1] = rv.buf[11];
			authAck[2] = rv.buf[12];
			authAck[3] = rv.buf[13];

			aes128_dec(rv.buf+10, &ctx);														// decrypt payload with temporarily key again

			/**
			 * Compare decrypted message with original message
			 */
			#ifdef AES_DBG
				dbg << F(">>> compare: ") << _HEX(rv.buf+10, 16) << " | "<< _HEX(rv.prevBuf, 11) << '\n';
			#endif

			// memcmp returns 0 if compare true
			 if (!memcmp(rv.buf+16, rv.prevBuf+1, 10)) {										// compare bytes 7-17 of decrypted data with bytes 2-12 of msgOriginal
				#ifdef AES_DBG
					dbg << F("Signature check OK\n");
				#endif

				sendAckAES(authAck);															// send AES-Ack

				if (keyPartIndex == AS_STATUS_KEYCHANGE_INACTIVE) {
					memcpy(rv.buf, rv.prevBuf, rv.prevBuf[0]+1);								// restore the last received message for processing from saved buffer
					rv.prevBufUsed = 0;

					if (rv.mBdy.mTyp == AS_MESSAGE_CONFIG) {
						processMessageConfig();

					} else if (rv.mBdy.mTyp == AS_MESSAGE_ACTION) {
						processMessageAction11();

					} else if (rv.mBdy.mTyp >= AS_MESSAGE_SWITCH_EVENT) {
						uint8_t pIdx;
						uint8_t cnl = getChannelFromPeerDB(&pIdx);
						if (cnl > 0) {
							processMessageAction3E(cnl, pIdx);
						}
					}

				} else if (keyPartIndex == AS_STATUS_KEYCHANGE_ACTIVE2) {
					setEEPromBlock(15, 16, newHmKey);											// store HMKEY
					getEEPromBlock(15, 16, HMKEY);
					setEEPromBlock(14, 1, newHmKeyIndex);										// store used key index
					hmKeyIndex[0] = newHmKeyIndex[0];
					#ifdef AES_DBG
						dbg << F("newHmKey: ") << _HEX(newHmKey, 16) << F(" ID: ") << _HEXB(hmKeyIndex[0]) << '\n';
					#endif

					keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;
				}

			 } else {
				#ifdef AES_DBG
					 dbg << F("Signature check FAIL\n");
				#endif

				// ToDo: Check if needed.
				sendNACK();
			}

		} else if ((rv.mBdy.mTyp == AS_MESSAGE_KEY_EXCHANGE)) {									// AES Key Exchange
			processMessageKeyExchange();

	#endif

	} else if (rv.mBdy.mTyp == AS_MESSAGE_ACTION) {												// action message
		#ifdef SUPPORT_AES

			uint8_t aesActiveForReset = 0;
			if (rv.mBdy.by10 == AS_ACTION_RESET && rv.mBdy.by11 == 0x00) {						// device reset requested
				aesActiveForReset = checkAnyChannelForAES();									// check if AES activated for any channel			}
			}

			// check if AES for the current channel active or aesActiveForReset @see above
			if (ee.getRegAddr(rv.mBdy.by11, 1, 0, AS_REG_L1_AES_ACTIVE) == 1 || aesActiveForReset == 1) {
				sendSignRequest(1);

			} else {
		#endif

				processMessageAction11();
				if (rv.ackRq || resetStatus == AS_RESET) {
					if (ee.getRegListIdx(1,3) == 0xFF || resetStatus == AS_RESET) {
						sendACK();
					} else {
						uint8_t channel = rv.mBdy.by11;
						if (rv.mBdy.by10 == AS_ACTION_RESET && rv.mBdy.by11 == 0x00) {
							channel = 1;
						}
						sendACK_STATUS(channel, 0, 0);
					}
				}

		#ifdef SUPPORT_AES
			}
		#endif

	} else if  (rv.mBdy.mTyp == AS_MESSAGE_HAVE_DATA) {											// HAVE_DATA
		// TODO: Make ready

	} else if  (rv.mBdy.mTyp >= AS_MESSAGE_SWITCH_EVENT) {
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

		//dbg << "cnl: " << cnl << " pIdx: " << pIdx << " mTyp: " << _HEXB(rv.mBdy.mTyp) << " by10: " << _HEXB(rv.mBdy.by10)  << " by11: " << _HEXB(rv.mBdy.by11) << " data: " << _HEX((rv.buf+10),(rv.mBdy.mLen-9)) << '\n'; _delay_ms(100);

		if (cnl > 0) {
			#ifdef SUPPORT_AES
				// check if AES for the current channel active
				if (ee.getRegAddr(cnl, 1, 0, AS_REG_L1_AES_ACTIVE) == 1) {
					sendSignRequest(1);

				} else {
			#endif

					processMessageAction3E(cnl, pIdx);

			#ifdef SUPPORT_AES
				}
			#endif
		}

	}

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
	if ((rv.mBdy.mTyp == AS_MESSAGE_SWITCH_EVENT) && (rv.mBdy.mLen == 0x0F)) {
		tmp = rv.buf[13];																		// save byte13, because we will replace it
		rv.buf[13] = rv.buf[14];																// copy the channel byte to the peer
		cnl = ee.isPeerValid(rv.buf+10);														// check with the right part of the string
		if (cnl) {
			*pIdx = ee.getIdxByPeer(cnl, rv.buf+10);											// get the index of the respective peer in the channel store
		}
		rv.buf[13] = tmp;																		// get it back

	} else {
		cnl = ee.isPeerValid(rv.peerId);
		if (cnl) {
			*pIdx = ee.getIdxByPeer(cnl, rv.peerId);											// get the index of the respective peer in the channel store
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
		for (i = 1; i <= devDef.cnlNbr; i++) {													// check if AES activated for any channel
			if (ee.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
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
		memcpy(rv.prevBuf, rv.buf, rv.buf[0]+1);												// remember this message
//		rv.prevBufUsed = 1;																		// ToDo: check if we need this here

		aes128_init(HMKEY, &ctx);																// load HMKEY
		aes128_dec(rv.buf+10, &ctx);															// decrypt payload width HMKEY first time

		#ifdef AES_DBG
			dbg << F("decrypted buf: ") << _HEX(rv.buf+10, 16) << '\n';
		#endif

		if (rv.buf[10] == 0x01) {																// the decrypted data must start with 0x01
			keyPartIndex = (rv.buf[11] & 1) ? AS_STATUS_KEYCHANGE_ACTIVE2 : AS_STATUS_KEYCHANGE_ACTIVE1;
			if (keyPartIndex == AS_STATUS_KEYCHANGE_ACTIVE1) {
				newHmKeyIndex[0] = rv.buf[11];
			}

			memcpy(newHmKey + keyPartIndex, rv.buf+12, 8);

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

		sn.cleanUp();																			// cleanup send module data;
		initPseudoRandomNumberGenerator();

		uint8_t challenge[6];
		memcpy(challenge, rv.buf+11, 6);														// get challenge

		makeTmpKey(challenge);																	// Build the temporarily key from challenge

		// Prepare the payload for encryption.
		uint8_t msgLen = sn.msgToSign[5];														// the message length stored at byte 5
		for (i = 0; i < 32; i++) {
			if (i < 6) {
				sn.msgToSign[i] = (uint8_t)rand();												// fill the first 6 bytes with random data
			} else if (i > msgLen + 5 ) {
				sn.msgToSign[i] = 0x00;															// the unused message bytes padded with 0x00
			}
		}

		aes128_enc(sn.msgToSign, &ctx);															// encrypt the message first time
		for (i = 0; i < 16; i++) {
			sn.msgToSign[i] ^= sn.msgToSign[i+16];												// xor encrypted payload with IV (the bytes 11-27)
		}

		aes128_enc(sn.msgToSign, &ctx);															// encrypt payload again
		sn.mBdy.mLen = 0x19;
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
	if (modTbl[by10].cnl) {
		modTbl[by10].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.mBdy.pyLd, rv.mBdy.mLen-11);
	} else {
		sendINFO_ACTUATOR_STATUS(rv.mBdy.by10, 0, 0);
	}
}

/*
 * @brief Process message CONFIG_STATUS_REQUEST.
 *
 * Message description:
 *             Sender__ Receiver       SerialNumber
 * 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
 */
inline void AS::processMessageConfigPairSerial(void) {
	if (!memcmp(rv.buf+12, HMSR, 10)) {															// compare serial and send device info
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
 *             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
 * 10 04 A0 01 63 19 63 01 02 04 01  04     00 00 00 00          01
 */
inline void AS::processMessageConfigParamReq(void) {
	if ((rv.buf[16] == 0x03) || (rv.buf[16] == 0x04)) {											// only list 3 and list 4 needs an peer id and idx
		stcSlice.idx = ee.getIdxByPeer(rv.mBdy.by10, rv.buf+12);								// get peer index
	} else {
		stcSlice.idx = 0;																		// otherwise peer index is 0
	}

	stcSlice.totSlc = ee.countRegListSlc(rv.mBdy.by10, rv.buf[16]);								// how many slices are need
	stcSlice.mCnt = rv.mBdy.mCnt;																// remember the message count
	memcpy(stcSlice.toID, rv.mBdy.reID, 3);
	stcSlice.cnl = rv.mBdy.by10;																// send input to the send peer function
	stcSlice.lst = rv.buf[16];																	// send input to the send peer function
	stcSlice.reg2 = 1;																			// set the type of answer

	#ifdef AS_DBG
		dbg << "cnl: " << rv.mBdy.by10 << " s: " << stcSlice.idx << '\n';
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
	stcSlice.totSlc = ee.countPeerSlc(rv.mBdy.by10);											// how many slices are need
	stcSlice.mCnt = rv.mBdy.mCnt;																// remember the message count
	memcpy(stcSlice.toID, rv.mBdy.reID, 3);
	stcSlice.cnl = rv.mBdy.by10;																// send input to the send peer function
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

	if (rv.mBdy.by11 == AS_CONFIG_PEER_ADD) {													// CONFIG_PEER_ADD
		ackOk = configPeerAdd(rv.mBdy.by10 -1);

	} else if (rv.mBdy.by11 == AS_CONFIG_PEER_REMOVE) {											// CONFIG_PEER_REMOVE
		ackOk = configPeerRemove();

	} else if (rv.mBdy.by11 == AS_CONFIG_START) {												// CONFIG_START
		configStart();

	} else if (rv.mBdy.by11 == AS_CONFIG_END) {													// CONFIG_END
		configEnd ();

	} else if (rv.mBdy.by11 == AS_CONFIG_WRITE_INDEX) {											// CONFIG_WRITE_INDEX
		configWriteIndex();

	}

	return ackOk;
}

/**
 * @brief Process CONFIG_PEER_REMOVE messages
 *
 * Message description:
 *             Sender__ Receiver Byte10    Channel Peer-ID_ PeerChannelA  PeerChannelB
 * 0C 0A A4 01 23 70 EC 1E 7A AD 01        01      1F A6 5C 06            05
 */
inline uint8_t AS::configPeerAdd(uint8_t by10) {
	ee.remPeer(rv.mBdy.by10, rv.buf+12);														// first call remPeer to avoid doubles
	uint8_t ackOk = ee.addPeer(rv.mBdy.by10, rv.buf+12);										// send to addPeer function

	// let module registrations know of the change
	if ((ackOk) && (modTbl[by10].cnl)) {
		modTbl[by10].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+15, 4);
	}

	return ackOk;
}

/**
 * @brief Process CONFIG_PEER_REMOVE messages
 *
 * Message description:
 *             Sender__ Receiver    Channel Peer-ID_ PeerChannelA  PeerChannelB
 * 0C 0A A4 01 23 70 EC 1E 7A AD 02 01      1F A6 5C 06            05
 */
inline uint8_t AS::configPeerRemove() {
	return ee.remPeer(rv.mBdy.by10,rv.buf+12);													// call the remPeer function
}

/**
 * @brief Process CONFIG_START messages
 *
 * Message description:
 *             Sender__ Receiver    Channel PeerID__ PeerChannel ParmList
 * 10 04 A0 01 63 19 63 01 02 04 01 05      00 00 00 00          00
 */
inline void AS::configStart() {
	cFlag.channel = rv.mBdy.by10;																// fill structure to remember where to write
	cFlag.list = rv.buf[16];
	if ((cFlag.list == 3) || (cFlag.list == 4)) {
		cFlag.idx_peer = ee.getIdxByPeer(rv.mBdy.by10, rv.buf + 12);
	} else {
		cFlag.idx_peer = 0;
	}

	if (cFlag.idx_peer != 0xFF) {
		cFlag.active = 1;																		// set active if there is no error on index
		cnfTmr.set(20000);																		// set timeout time, will be checked in poll function
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
	uint8_t cnl1 = cFlag.channel - 1;

	cFlag.active = 0;																			// set inactive
	if ((cFlag.channel == 0) && (cFlag.idx_peer == 0)) {
		ee.getMasterID();
	}
	// remove message id flag to config in send module

	if ((cFlag.channel > 0) && (modTbl[cnl1].cnl)) {
		/*
		 * Check if a new list1 was written and reload.
		 * No need for reload list3/4 because they will be loaded on an peer event.
		 */
		if (cFlag.list == 1) {
			ee.getList(cFlag.channel, 1, cFlag.idx_peer, modTbl[cnl1].lstCnl); 					// load list1 in the respective buffer
		}
		modTbl[cnl1].mDlgt(0x01, 0, 0x06, NULL, 0);												// inform the module of the change
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
	if ((cFlag.active) && (cFlag.channel == rv.mBdy.by10)) {									// check if we are in config mode and if the channel fit
		ee.setListArray(cFlag.channel, cFlag.list, cFlag.idx_peer, rv.buf[0]-11, rv.buf+12);			// write the string to EEprom

		if ((cFlag.channel == 0) && (cFlag.list == 0)) {										// check if we got somewhere in the string a 0x0a, as indicator for a new masterid
			uint8_t maIdFlag = 0;
			for (uint8_t i = 0; i < (rv.buf[0]+1-12); i+=2) {
				if (rv.buf[12+i] == 0x0A) maIdFlag = 1;
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
}

/**
 * @brief Process all action (11) messages
 */
void AS::processMessageAction11() {
	if (rv.mBdy.by10 == AS_ACTION_RESET && rv.mBdy.by11 == 0x00) {								// RESET
		/*
		 * Message description:
		 *             Sender__ Receiver
		 * 0B 1C B0 11 63 19 63 1F B7 4A 04 00
		 */
		resetStatus = AS_RESET_CLEAR_EEPROM;													// schedule a device reset with clear eeprom

	} else if (rv.mBdy.by10 == AS_ACTION_ENTER_BOOTLOADER) {									// We should enter the Bootloader
		dbg << "AS_ACTION_ENTER_BOOTLOADER\n";
		/*
		 * Message description:
		 *             Sender__ Receiver
		 * 0B 1C B0 11 63 19 63 1F B7 4A CA
		 */
		resetStatus = AS_RESET;																	// schedule a device reset without eeprom
		rv.ackRq = 1;

	} else {
		/*
		 * All other action types like STOP_CHANGE, LED, LEDALL, LEVEL, SLEEPMODE and do on
		 *
		 * Message description:
		 *             Sender__ Receiver type actionType channel data
		 * 0E 5E B0 11 63 19 63 1F B7 4A 02   01         01      C8 00 00 00 00
		 */
		if (modTbl[rv.mBdy.by11-1].cnl) {
			modTbl[rv.mBdy.by11-1].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+12, rv.mBdy.mLen-11);
		}
	}
}

/**
 * @brief Process all action (3E, 3F, 40, 41, ...) messages
 */
void AS::processMessageAction3E(uint8_t cnl, uint8_t pIdx) {
	// check if a module is registered and send the information, otherwise report an empty status
	if (modTbl[cnl-1].cnl) {

		//dbg << "pIdx:" << pIdx << ", cnl:" << cnl << '\n';
		ee.getList(cnl, modTbl[cnl-1].lst, pIdx, modTbl[cnl-1].lstPeer);						// get list3 or list4 loaded into the user module

		// call the user module
		modTbl[cnl-1].mDlgt(rv.mBdy.mTyp, rv.mBdy.by10, rv.mBdy.by11, rv.buf+10, rv.mBdy.mLen-9);

	} else {
		sendACK();

	}
}

/**
 * @brief Reset the Device
 *        Set all register to default 0x00, reset HMKEY, reset device via watchdog,
 *        and so on.
 */
void AS::deviceReset(uint8_t clearEeprom) {
	if (clearEeprom == AS_RESET_CLEAR_EEPROM) {
		ee.clearPeers();
		ee.clearRegs();
		ee.getMasterID();

		#ifdef SUPPORT_AES
			ee.initHMKEY();
		#endif
	}

	#ifdef WDT_RESET_ON_RESET
		wdt_enable(WDTO_15MS);																	// configure the watchdog so the reset sould trigger in 15ms
	#else
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
	sn.mBdy.mLen = 0x14;
	sn.mBdy.by10 = AS_INFO_SERIAL;
	memcpy(sn.buf+11, HMSR, 10);
	prepareToSend(rv.mBdy.mLen, AS_MESSAGE_INFO, rv.mBdy.reID);
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
	sn.mBdy.mLen = length + 10;
	sn.mBdy.mFlg.BIDI = 1;
	sn.mBdy.by10 = AS_INFO_PEER_LIST;															//stcSlice.cnl;
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
	sn.mBdy.mLen = length + 10;
	sn.mBdy.mFlg.BIDI = 1;
	sn.mBdy.by10 = (length < 3) ? AS_INFO_PARAM_RESPONSE_SEQ : AS_INFO_PARAM_RESPONSE_PAIRS;
	prepareToSend(stcSlice.mCnt++, AS_MESSAGE_INFO, stcSlice.toID);
}

/**
 * @brief Set message type, sender and receiver address
 *        and set sn.active, so the message should send next time
 *
 * @param mCounter the message counter
 * @param mType    the message type
 * @param addrTo   pointer to receiver address
 */
void AS::prepareToSend(uint8_t mCounter, uint8_t mType, uint8_t *receiverAddr) {
	uint8_t i;

	sn.mBdy.mCnt = mCounter;
	sn.mBdy.mTyp = mType;
	sn.mBdy.reID[0] = HMID[0];
	sn.mBdy.reID[1] = HMID[1];
	sn.mBdy.reID[2] = HMID[2];
	memcpy(sn.mBdy.toID, receiverAddr, 3);

	sn.active = 1;																				// remember to fire the message
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
void AS::decode(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i = 2; i < buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xDC) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}

/**
 * @brief Encode the outgoing messages
 *        Note: this is no encryption!
 *
 * @param buf   pointer to buffer
 */
void AS::encode(uint8_t *buf) {
	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i = 2; i < buf[0]; i++) {
		prev = (prev + 0xDC) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}

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
			memcpy(rv.prevBuf, rv.buf, rv.buf[0]+1);											// remember the message from buffer
			rv.prevBufUsed = 1;
		}

		sn.mBdy.mLen      = 0x11;
		sn.mBdy.mFlg.BIDI = (isEmpty(MAID,3)) ? 0 : 1;
		sn.mBdy.by10      = AS_RESPONSE_AES_CHALLANGE;											// AES Challenge

		initPseudoRandomNumberGenerator();

		uint8_t i = 0;
		for (i = 0; i < 6; i++) {																// random bytes to the payload
			sn.buf[11 + i] = (uint8_t)rand();
		}
		sn.buf[17] = hmKeyIndex[0];																// the 7th byte is the key index

		/*
		 * Here we make a temporarily key with the challenge and the HMKEY.
		 * We need this for later signature verification.
		 */
		makeTmpKey(sn.buf+11);

		#ifdef AES_DBG
			dbg << F(">>> signingRequestData  : ") << _HEX(sn.buf+10, 7) << F(" <<<") << '\n';
		#endif

		prepareToSend(rv.mBdy.mCnt, AS_MESSAGE_RESPONSE, rv.mBdy.reID);
	}

	/**
	 * @brief Make a temporarily key for encrypting the sign response.
	 *        The temporarily key was built by XORing the key with the challenge
	 *
	 * @param challenge   pointer to the challenge
	 */
	void AS::makeTmpKey(uint8_t *challenge) {
		for (uint8_t i = 0; i < 16; i++) {
			this->tempHmKey[i] = (i<6) ? (HMKEY[i] ^ challenge[i]) : HMKEY[i];
		}

		aes128_init(this->tempHmKey, &ctx);														// generating the round keys from the 128 bit key
	}

#endif


// - some helpers ----------------------------------
// public:		//---------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize the random number generator
 */
void AS::initPseudoRandomNumberGenerator() {
	srand(this->randomSeed ^ uint16_t (millis() & 0xFFFF));
}

/**
 * @brief Initialize the pseudo random number generator
 *        Take all bytes from uninitialized RAM and xor together
 */
inline void AS::initRandomSeed() {
	uint16_t *p = (uint16_t*) (RAMEND + 1);
	extern uint16_t __heap_start;
	while (p >= &__heap_start + 1) {
		this->randomSeed ^= * (--p);
	}

	initPseudoRandomNumberGenerator();
}

/**
 * @brief Query if the timer has expired
 *
 * @return 0 if timer is still running, 1 if not.
 *         If the timer was never set(), return value is 1
 */
uint8_t  waitTimer::done(void) {
	if (!armed) return 1;																		// not armed, so nothing to do
	if ( (getMillis() - startTime) < checkTime ) return 0;										// not ready yet

	checkTime = armed = 0;																		// if we are here, timeout was happened, next loop status 1 will indicated
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
	return (uint32_t)(tTime & 0x1F) * c[tTime >> 5]*100;
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
