//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define AS_DBG
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------
AS::AS() {
}
void AS::init(void) {
	#ifdef AS_DBG																			// only if cc debug is set
	dbgStart();																				// serial setup
	dbg << F("AS.\n");																		// ...and some information
	#endif
	
	ee.init();																				// eeprom init
	cc.init();																				// init the rf module

	sn.init(this);
	rv.init(this);

	initMillis();																			// start the millis counter

	// everything is setuped, enable RF functionality
	_enableGDO0Int;																			// enable interrupt to get a signal while receiving data
}

// - poll functions --------------------------------
void AS::poll(void) {

	// check if something received
	if (ccGetGDO0()) {																		// check if something was received
		cc.rcvData(rv.buf);																	// copy the data into the receiver module
		if (rv.hasData) decode(rv.buf);														// decode the string
	}

	if (rv.hasData) rv.poll();																// check if there is something in the received buffer
	if (sn.active) sn.poll();

	if (sList.active) sendSliceList();														// poll the slice list send function
	if (peMsg.active) sendPeerMsg();														// poll the peer message sender
	
	// check if we could go to standby
	
	// some sanity poll routines
	
}

void AS::sendSliceList(void) {
	if (sn.active) return;																	// check if send function has a free slot, otherwise return

	uint8_t cnt;

	if        (sList.peer) {			// INFO_PEER_LIST
		cnt = ee.getPeerListSlc(sList.cnl,sList.curSlc,sn.buf+11);							// get the slice and the amount of bytes
		sendINFO_PEER_LIST(cnt);															// create the body		
		sList.curSlc++;																		// increase slice counter
		//dbg << "peer slc: " << pHex(sn.buf,sn.buf[0]+1) << '\n';							// write to send buffer

	} else if (sList.reg2) {			// INFO_PARAM_RESPONSE_PAIRS
		cnt = ee.getRegListSlc(sList.cnl,sList.lst,sList.idx,sList.curSlc,sn.buf+11);		// get the slice and the amount of bytes
		sendINFO_PARAM_RESPONSE_PAIRS(cnt);	
		sList.curSlc++;																		// increase slice counter
		//dbg << "reg2 slc: " << pHex(sn.buf,sn.buf[0]+1) << '\n';							// write to send buffer
		
	} else if (sList.reg3) {			// INFO_PARAM_RESPONSE_SEQ

	}

	if (sList.curSlc == sList.totSlc) {														// if everything is send, we could empty the struct
		memset((void*)&sList,0,10);															// by memset
		//dbg << "end: " << sList.active << sList.peer << sList.reg2 << sList.reg3 << '\n';
	}
}
void AS::sendPeerMsg(void) {
	#define maxRetries    3

	if (sn.active) return;																	// check if send function has a free slot, otherwise return
	
	// first run, prepare amount of slots
	if (!peMsg.maxIdx) peMsg.maxIdx = ee.getPeerSlots(peMsg.cnl);							// get amount of messages of peer channel

	// all slots of channel processed, start next round or end processing
	if (peMsg.curIdx >= peMsg.maxIdx) {														// check if all peer slots are done
		peMsg.rnd++;																		// increase the round counter
		
		if ((peMsg.rnd >= maxRetries) || (isEmty(peMsg.slt,8))) {							// all rounds done or all peers reached
			//dbg << "through\n";
			sn.msgCnt++;																	// increase the send message counter
			memset((void*)&peMsg, 0, sizeof(s_peMsg));										// clean out and return
			
		} else {																			// start next round
			//dbg << "next round\n";
			peMsg.curIdx = 0;

		}
		return;

	} else if ((peMsg.curIdx) && (!sn.timeOut)) {											// peer index is >0, first round done and no timeout
		peMsg.slt[(peMsg.curIdx-1) >> 3] &=  ~(1<<((peMsg.curIdx-1) & 0x07));				// clear bit, because message got an ACK		

	}
	
	// set respective bit to check if ACK was received
	if (!peMsg.rnd) peMsg.slt[peMsg.curIdx >> 3] |= (1<<(peMsg.curIdx & 0x07));				// set bit in slt table										// clear bit in slt and increase counter


	// exit while bit is not set
	if (!peMsg.slt[peMsg.curIdx >> 3] & (1<<(peMsg.curIdx & 0x07))) {
		peMsg.curIdx++;																		// increase counter for next time
		return;
	}

	uint8_t tPeer[4];																		// get the respective peer
	ee.getPeerByIdx(peMsg.cnl,peMsg.curIdx,tPeer);
		
	if (isEmty(tPeer,4)) {																	// if peer is 0, set done bit in slt and skip
		peMsg.slt[peMsg.curIdx >> 3] &=  ~(1<<(peMsg.curIdx & 0x07));						// remember empty peer in slt table										// clear bit in slt and increase counter
		peMsg.curIdx++;																		// increase counter for next time
		return;																				// wait for next round
	}

	// if we are here, there is something to send
	//dbg << "cnl:" << peMsg.cnl << " cIdx:" << peMsg.curIdx << " mIdx:" << peMsg.maxIdx << " slt:" << pHex(peMsg.slt,8) << '\n';
	
	// description --------------------------------------------------------
	//    len  cnt  flg  typ  reID      toID      pl
	// l> 0B   0A   A4   40   23 70 EC  1E 7A AD  02 01
	sn.mBdy.mLen = peMsg.lenPL +9;															// set message len
	sn.mBdy.mCnt = sn.msgCnt;																// set message counter
	//sn.mBdy.mFlg.RPTEN = 1; 
	sn.mBdy.mFlg.CFG = 1; sn.mBdy.mFlg.BIDI = peMsg.bidi;		// message flag
	
	// rework needed - burst should be send only once
	peMsg.burst = peMsg.burst;
	
	sn.mBdy.mTyp = peMsg.mTyp;																// message type
	//uint8_t t1[] = {0x23,0x70,0xD8};
	//memcpy(sn.mBdy.reID, t1, 3);															// sender id
	memcpy(sn.mBdy.reID, HMID, 3);															// sender id
	memcpy(sn.mBdy.toID, tPeer, 3);															// receiver id
	memcpy(sn.buf+10, peMsg.pL, peMsg.lenPL);												// payload
	
	sn.maxRetr = 1;																			// send only one time
	sn.active = 1;																			// make send active
	
	if (!sn.mBdy.mFlg.BIDI) peMsg.slt[peMsg.curIdx >> 3] &=  ~(1<<(peMsg.curIdx & 0x07));	// clear bit, because it is a message without need to be repeated
	peMsg.curIdx++;																			// increase counter for next time
}

	
// - receive functions -----------------------------
void AS::recvDEVICE_INFO(void) {
	// description --------------------------------------------------------
	// 
	// 
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}

void AS::recvCONFIG_PEER_ADD(void) {		
	// description --------------------------------------------------------
	//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
	// l> 10 55 A0 01 63 19 63 01 02 04 01   01  1F A6 5C  06         05
	// do something with the information ----------------------------------

	// first call remPeer to avoid doubles
	uint8_t ret = ee.addPeer(rv.mBdy.by10,rv.buf+12);										// send to addPeer function
		
	// let module registrations know of the change

	if ((ret) && (rv.ackRq)) sendACK();														// send appropriate answer
	else if (rv.ackRq) sendNACK();
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_PEER_REMOVE(void) {
	// description --------------------------------------------------------
	//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
	// l> 10 55 A0 01 63 19 63 01 02 04 01   02  1F A6 5C  06         05
	// do something with the information ----------------------------------
		
	uint8_t ret = ee.remPeer(rv.mBdy.by10,rv.buf+12);										// call the remPeer function
	if (rv.ackRq) sendACK();																// send appropriate answer
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_PEER_LIST_REQ(void) {
	// description --------------------------------------------------------
	//                                  Cnl
	// l> 0B 05 A0 01 63 19 63 01 02 04 01  03
	// do something with the information ----------------------------------
		
	sList.totSlc = ee.countPeerSlc(rv.mBdy.by10);											// how many slices are need
	sList.mCnt = rv.mBdy.mLen;																// remember the message count
	memcpy(sList.toID,rv.mBdy.reID,3);
	sList.cnl = rv.mBdy.by10;																// send input to the send peer function
	sList.peer = 1;																			// set the type of answer
	sList.active = 1;																		// start the send function
	// answer will send from sendsList(void)
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_PARAM_REQ(void) {
	// description --------------------------------------------------------
	//                                  Cnl    PeerID    PeerCnl  ParmLst
	// l> 10 04 A0 01 63 19 63 01 02 04 01  04 00 00 00  00       01
	// do something with the information ----------------------------------

	sList.idx = ee.getIdxByPeer(rv.mBdy.by10, rv.buf+12);									// fill struct
	sList.totSlc = ee.countRegListSlc(rv.mBdy.by10, rv.buf[16]);							// how many slices are need
	sList.mCnt = rv.mBdy.mLen;																// remember the message count
	memcpy(sList.toID,rv.mBdy.reID,3);
	sList.cnl = rv.mBdy.by10;																// send input to the send peer function
	sList.lst = rv.buf[16];																	// send input to the send peer function
	sList.reg2 = 1;																			// set the type of answer
		
	if ((sList.idx != 0xff) && (sList.totSlc > 0)) sList.active = 1;						// only send register content if something is to send															// start the send function
	else memset((void*)&sList,0,10);														// otherwise empty variable
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_START(void) {
	// description --------------------------------------------------------
	//                                  Cnl    PeerID    PeerCnl  ParmLst
	// l> 10 01 A0 01 63 19 63 01 02 04 00  05 00 00 00  00       00
	// do something with the information ----------------------------------

	cFlag.idx = ee.getIdxByPeer(rv.mBdy.by10, rv.buf+12);									// fill structure to remember where to write
	cFlag.cnl = rv.mBdy.by10;
	cFlag.lst = rv.buf[16];
	if (cFlag.idx != 0xff) {
		cFlag.active = 1;																	// set active if there is no error on index
		// set message id flag to config in send module
	}
		
	if (rv.ackRq) sendACK();																// send appropriate answer
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_END(void) {
	// description --------------------------------------------------------
	//                                  Cnl
	// l> 0B 01 A0 01 63 19 63 01 02 04 00  06
	// do something with the information ----------------------------------

	cFlag.active = 0;																		// set inactive
	if ((cFlag.cnl == 0) && (cFlag.idx == 0)) ee.getMasterID();
	// remove message id flag to config in send module
		
	if (rv.ackRq) sendACK();																// send appropriate answer
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_WRITE_INDEX(void) {
	// description --------------------------------------------------------
	//                                  Cnl    Data
	// l> 13 02 A0 01 63 19 63 01 02 04 00  08 02 01 0A 63 0B 19 0C 63
	// do something with the information ----------------------------------

	if ((cFlag.active) && (cFlag.cnl == rv.mBdy.by10)) {									// check if we are in config mode and if the channel fit
		ee.setListArray(cFlag.cnl, cFlag.lst, cFlag.idx, rv.buf[0]+1-11, rv.buf+12);		// write the string to eeprom
	}
	if (rv.ackRq) sendACK();																// send appropriate answer
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_SERIAL_REQ(void) {
	// description --------------------------------------------------------
	//                                  
	// l> 0B 77 A0 01 63 19 63 01 02 04 00 09
	// do something with the information ----------------------------------
	sendINFO_SERIAL();																		// jump to create the answer
	// --------------------------------------------------------------------
}
void AS::recvPAIR_SERIAL(void) {
	// description --------------------------------------------------------
	//                                         serial
	// b> 15 93 B4 01 63 19 63 00 00 00 01 0A  4B 45 51 30 32 33 37 33 39 36
	// do something with the information ----------------------------------
	if (cmpAry(rv.buf+12,HMSR,10)) sendDEVICE_INFO();										// compare serial and send device info
	// --------------------------------------------------------------------
}
void AS::recvCONFIG_STATUS_REQUEST(void) {
	// description --------------------------------------------------------
	//  
	// b> 
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//CHANNEL => "0,2", } },
}

void AS::recvACK(void) {
	// description --------------------------------------------------------
	// 
	// l> 0A 05 80 02 63 19 63 01 02 04 00
	// do something with the information ----------------------------------
		
	if ((sn.active) && (rv.mBdy.mLen == sn.lastMsgCnt)) sn.retrCnt = 0xff;					// was an ACK to an active message, message counter is similar - set retrCnt to 255 
	//dbg << "act:" << sn.active << " rC:" << rv.mBdy.mLen << " sC:" << sn.lastMsgCnt << " cntr:" << sn.retrCnt << '\n';
	// --------------------------------------------------------------------
}
void AS::recvACK_STATUS(void) {
	// description --------------------------------------------------------
	// <- 0B 08 B4 40 23 70 D8 1F B7 4A 02 08
	//                                      cnl stat DUL RSSI
	// l> 0E 08 80 02 1F B7 4A 23 70 D8 01  01  C8   80  27
	// do something with the information ----------------------------------
	// DUL = UP 10, DOWN 20, LOWBAT 80
	
	if ((sn.active) && (rv.mBdy.mLen == sn.lastMsgCnt)) sn.retrCnt = 0xff;					// was an ACK to an active message, message counter is similar - set retrCnt to 255 
	// --------------------------------------------------------------------
}
void AS::recvACK2(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvACK_PROC(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//Para1          => "02,4",
	//Para2          => "06,4",
	//Para3          => "10,4",
	//Para4          => "14,2",}}, # remote?
}
void AS::recvNACK(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// for test
	static uint8_t x2[2];
	x2[0] = 0x02;
	x2[1] += 1;
	sendREMOTE(1,1,x2);

	// --------------------------------------------------------------------
}
void AS::recvNACK_TARGET_INVALID(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}

void AS::recvSET(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvSTOP_CHANGE(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvRESET(void) {
	// description --------------------------------------------------------
	//
	// l> 0B 1C B0 11 63 19 63 1F B7 4A 04 00 (234116)
	// l> 0E 1C 80 02 1F B7 4A 63 19 63 01 01 00 80 14 (234243)
	// do something with the information ----------------------------------

	ee.clearPeers();
	ee.clearRegs();
	ee.getMasterID();
	sendACK_STATUS(); // or sendACK() depending on device
	// --------------------------------------------------------------------
}
void AS::recvLED(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvLEDALL(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvLEVEL(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvSLEEPMODE(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}

void AS::recvHAVE_DATA(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
}
void AS::recvSWITCH(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//DST      => "00,6",
	//UNKNOWN  => "06,2",
	//CHANNEL  => "08,2",
	//COUNTER  => "10,2", } },
}
void AS::recvTIMESTAMP(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//UNKNOWN  => "00,4",
	//TIME     => "04,2", } },
}
void AS::recvREMOTE(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
	//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
	//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
	//COUNTER  => "02,2", } },
}
void AS::recvSENSOR_EVENT(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
	//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
	//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
	//NBR      => '02,2,$val=(hex($val))',
	//VALUE    => '04,2,$val=(hex($val))',} },
}
void AS::recvSENSOR_DATA(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
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
void AS::recvCLIMATE_EVENT(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
}
void AS::recvWEATHER_EVENT(void) {
	// description --------------------------------------------------------
	//
	// b>
	// do something with the information ----------------------------------

	// --------------------------------------------------------------------
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
}


// - send functions --------------------------------
void AS::sendDEVICE_INFO(void) {
	// description --------------------------------------------------------
	//                 reID      toID      fw  type   serial                         class  pCnlA  pCnlB  unknown
	// l> 1A 94 84 00  1F B7 4A  01 02 04  15  00 6C  4B 45 51 30 32 33 37 33 39 36  10     41     01     00
	// do something with the information ----------------------------------

	uint8_t xCnt;
	if ((rv.mBdy.mTyp == 0x01) && (rv.mBdy.by11 == 0x0A)) xCnt = rv.mBdy.mLen;														// send counter - is it an answer or a initial message
	else xCnt = sn.msgCnt++;
	
	sn.mBdy.mLen = 0x1a;
	sn.mBdy.mCnt = xCnt;
	sn.mBdy.mFlg.CFG = 1;
	sn.mBdy.mTyp = 0x00;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,MAID,3);

	memcpy_P(sn.buf+10,devDef.devIdnt,3);
	memcpy(sn.buf+13,HMSR,10);
	memcpy_P(sn.buf+23,devDef.devIdnt+3,4);
	sn.active = 1;																			// fire the message
}
void AS::sendACK(void) {
	// description --------------------------------------------------------
	//                reID      toID      ACK      
	// l> 0A 24 80 02 1F B7 4A  63 19 63  00
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 0x0a;
	sn.mBdy.mCnt = rv.mBdy.mLen;
	sn.mBdy.mTyp = 0x02;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = 0x00;
	sn.active = 1;																			// fire the message
}
void AS::sendACK_STATUS(void) {
	//"02;p01=01"   => { txt => "ACK_STATUS",  params => {
	//CHANNEL        => "02,2",
	//STATUS         => "04,2",
	//DOWN           => '06,02,$val=(hex($val)&0x20)?1:0',
	//UP             => '06,02,$val=(hex($val)&0x10)?1:0',
	//LOWBAT         => '06,02,$val=(hex($val)&0x80)?1:0',
	//RSSI           => '08,02,$val=(-1)*(hex($val))', }},
}
void AS::sendNACK(void) {
	// description --------------------------------------------------------
	//                reID      toID      NACK
	// l> 0A 24 80 02 1F B7 4A  63 19 63  80
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 0x0a;
	sn.mBdy.mCnt = rv.mBdy.mLen;
	sn.mBdy.mTyp = 0x02;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = 0x80;
	sn.active = 1;																			// fire the message
}
void AS::sendNACK_TARGET_INVALID(void) {
	// description --------------------------------------------------------
	//                reID      toID      ACK
	// l> 0A 24 80 02 1F B7 4A  63 19 63  84
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 0x0a;
	sn.mBdy.mCnt = rv.mBdy.mLen;
	sn.mBdy.mTyp = 0x02;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = 0x84;
	sn.active = 1;																			// fire the message
}
void AS::sendINFO_SERIAL(void) {
	// description --------------------------------------------------------
	// l> 0B 77 A0 01 63 19 63 1E 7A AD 00 09
	//                reID      toID     by10  serial
	// l> 14 77 80 10 1E 7A AD  63 19 63 00    4A 45 51 30 37 33 31 39 30 35
	// do something with the information ----------------------------------

	sn.mBdy.mLen = 0x14;
	sn.mBdy.mCnt = rv.mBdy.mLen;
	sn.mBdy.mTyp = 0x10;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,rv.mBdy.reID,3);
	sn.mBdy.by10 = 0x00;
	memcpy(sn.buf+11,HMSR,10);
	sn.active = 1;																			// fire the message
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
	sn.mBdy.mCnt = sList.mCnt++;
	sn.mBdy.mFlg.BIDI = 1;
	sn.mBdy.mTyp = 0x10;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,sList.toID,3);
	sn.mBdy.by10 = 0x01; //sList.cnl;
	//dbg << "x: " << pHex(sn.buf, sn.mBdy.mLen+1) << '\n';
	sn.active = 1;																			// fire the message
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

	sn.mBdy.mLen = 10+len;
	sn.mBdy.mCnt = sList.mCnt++;
	sn.mBdy.mFlg.BIDI = 1;
	sn.mBdy.mTyp = 0x10;
	memcpy(sn.mBdy.reID,HMID,3);
	memcpy(sn.mBdy.toID,sList.toID,3);
	sn.mBdy.by10 = 0x02; //sList.cnl;
	sn.active = 1;																			// fire the message
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


}
void AS::sendINFO_PARAMETER_CHANGE(void) {
	//"10;p01=04"   => { txt => "INFO_PARAMETER_CHANGE", params => {
	//CHANNEL => "2,2",
	//PEER    => '4,8,$val=CUL_HM_id2Name($val)',
	//PARAM_LIST => "12,2",
	//DATA => '14,,$val =~ s/(..)(..)/ $1:$2/g', } },
}
void AS::sendINFO_ACTUATOR_STATUS(void) {
	//"10;p01=06"   => { txt => "INFO_ACTUATOR_STATUS", params => {
	//CHANNEL => "2,2",
	//STATUS  => '4,2',
	//UNKNOWN => "6,2",
	//RSSI    => '08,02,$val=(-1)*(hex($val))' } },
}
void AS::sendINFO_TEMP(void) {
	//"10;p01=0A"   => { txt => "INFO_TEMP", params => {
	//SET     => '2,4,$val=(hex($val)>>10)&0x3F',
	//ACT     => '2,4,$val=hex($val)&0x3FF',
	//ERR     => "6,2",
	//VALVE   => "6,2",
	//MODE    => "6,2" } },
}
void AS::sendHAVE_DATA(void) {
	//"12"          => { txt => "HAVE_DATA"},
}
void AS::sendSWITCH(void) {
	//"3E"          => { txt => "SWITCH"      , params => {
	//DST      => "00,6",
	//UNKNOWN  => "06,2",
	//CHANNEL  => "08,2",
	//COUNTER  => "10,2", } },
}
void AS::sendTimeStamp(void) {
	//"3F"          => { txt => "TimeStamp"   , params => {
	//UNKNOWN  => "00,4",
	//TIME     => "04,2", } },
}
void AS::sendREMOTE(uint8_t cnl, uint8_t burst, uint8_t *pL) {
	// description --------------------------------------------------------
	//                 reID      toID      BLL Cnt
	// l> 0B 0A A4 40  23 70 EC  1E 7A AD  02  01
	// l> 0F 0A 80 02 1E 7A AD 23 70 EC 01 01 14 10 32 A4
	// l> 0B 0B B0 40 23 70 EC 1F B7 4A 02 01
	// l> 0E 0B 80 02 1F B7 4A 23 70 EC 01 01 C8 80 21
	// l> 0F 0B A4 10 1E 7A AD 63 19 63 06 01 C8 00 80 C8
	// l> 0A 0B 80 02 63 19 63 1E 7A AD 00
	// BUTTON = bit 0 - 5
	// LONG   = bit 6
	// LOWBAT = bit 7

	peMsg.pL = pL;
	peMsg.lenPL = 2;
	peMsg.cnl = cnl;
	peMsg.burst = burst;
	peMsg.bidi = 1; // depends on BLL, long didn't need ack
	peMsg.mTyp = 0x40;
	peMsg.active = 1;

}
void AS::sendSensor_event(uint8_t cnl, uint8_t burst, uint8_t *pL) {
	// description --------------------------------------------------------
	//                 reID      toID      BLL  Cnt  Val
	// l> 0C 0A A4 41  23 70 EC  1E 7A AD  02   01   200
	//"41"          => { txt => "Sensor_event", params => {
	// BUTTON = bit 0 - 5
	// LONG   = bit 6
	// LOWBAT = bit 7
}
void AS::sendSensorData(void) {
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
	//"58"          => { txt => "ClimateEvent", params => {
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
}
void AS::sendSetTeamTemp(void) {
	//"59"          => { txt => "setTeamTemp" , params => {
	//CMD      => "00,2",
	//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
	//mode     => '02,2,$val=(hex($val) & 0x3)',} },
}
void AS::sendWeatherEvent(void) {
	//"70"          => { txt => "WeatherEvent", params => {
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
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
void AS::explainMessage(uint8_t *buf) {
	dbg << F("   ");																		// save some byte and send 3 blanks once, instead of having it in every if
	
	if        ((buf[3] == 0x00)) {
		dbg << F("DEVICE_INFO; fw: ") << pHex((buf+10),1) << F(", type: ") << pHex((buf+11),2) << F(", serial: ") << pHex((buf+13),10) << '\n';
		dbg << F("              , class: ") << pHexB(buf[23]) << F(", pCnlA: ") << pHexB(buf[24]) << F(", pCnlB: ") << pHexB(buf[25]) << F(", na: ") << pHexB(buf[26]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x01)) {
		dbg << F("CONFIG_PEER_ADD; cnl: ") << pHexB(buf[10]) << F(", peer: ") << pHex((buf+12),3) << F(", pCnlA: ") << pHexB(buf[15]) << F(", pCnlB: ") << pHexB(buf[16]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x02)) {
		dbg << F("CONFIG_PEER_REMOVE; cnl: ") << pHexB(buf[10]) << F(", peer: ") << pHex((buf+12),3) << F(", pCnlA: ") << pHexB(buf[15]) << F(", pCnlB: ") << pHexB(buf[16]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x03)) {
		dbg << F("CONFIG_PEER_LIST_REQ; cnl: ") << pHexB(buf[10]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x04)) {
		dbg << F("CONFIG_PARAM_REQ; cnl: ") << pHexB(buf[10]) << F(", peer: ") << pHex((buf+12),3) << F(", pCnl: ") << pHexB(buf[15]) << F(", lst: ") << pHexB(buf[16]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x05)) {
		dbg << F("CONFIG_START; cnl: ") << pHexB(buf[10]) << F(", peer: ") << pHex((buf+12),3) << F(", pCnl: ") << pHexB(buf[15]) << F(", lst: ") << pHexB(buf[16]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x06)) {
		dbg << F("CONFIG_END; cnl: ") << pHexB(buf[10]);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x08)) {
		dbg << F("CONFIG_WRITE_INDEX; cnl: ") << pHexB(buf[10]) << F(", data: ") << pHex((buf+12),(buf[0]-11));

	} else if ((buf[3] == 0x01) && (buf[11] == 0x09)) {
		dbg << F("CONFIG_SERIAL_REQ");
		
	} else if ((buf[3] == 0x01) && (buf[11] == 0x0A)) {
		dbg << F("PAIR_SERIAL, serial: ") << pHex((buf+12),10);

	} else if ((buf[3] == 0x01) && (buf[11] == 0x0E)) {
		dbg << F("CONFIG_STATUS_REQUEST, cnl: ") << pHexB(buf[10]);

	} else if ((buf[3] == 0x02) && (buf[11] == 0x00)) {
		if (buf[0] == 0x0A) dbg << F("ACK");
		else dbg << F("ACK; data: ") << pHex((buf+11),buf[0]-10);

	} else if ((buf[3] == 0x02) && (buf[11] == 0x01)) {
		dbg << F("ACK_STATUS; cnl: ") << pHexB(buf[11]) << F(", status: ") << pHexB(buf[12]) << F(", down/up/loBat: ") << pHexB(buf[13]);
		if (buf[0] > 13) dbg << F(", rssi: ") << pHexB(buf[14]);

	} else if ((buf[3] == 0x02) && (buf[11] == 0x02)) {
		dbg << F("ACK2");
		
	} else if ((buf[3] == 0x02) && (buf[11] == 0x04)) {
		dbg << F("ACK_PROC; para1: ") << pHex((buf+11),2) << F(", para2: ") << pHex((buf+13),2) << F(", para3: ") << pHex((buf+15),2) << F(", para4: ") << pHexB(buf[17]);

	} else if ((buf[3] == 0x02) && (buf[11] == 0x80)) {
		dbg << F("NACK");

	} else if ((buf[3] == 0x02) && (buf[11] == 0x84)) {
		dbg << F("NACK_TARGET_INVALID");
		
	} else if ((buf[3] == 0x03)) {
		dbg << F("AES_REPLY; data: ") << pHex((buf+10),buf[0]-9);
		
	} else if ((buf[3] == 0x04) && (buf[10] == 0x01)) {
		dbg << F("TOpHMLAN:SEND_AES_CODE; cnl: ") << pHexB(buf[11]);

	} else if ((buf[3] == 0x04)) {
		dbg << F("TO_ACTOR:SEND_AES_CODE; code: ") << pHexB(buf[11]);
		
	} else if ((buf[3] == 0x10) && (buf[10] == 0x00)) {
		dbg << F("INFO_SERIAL; serial: ") << pHex((buf+11),10);

	} else if ((buf[3] == 0x10) && (buf[10] == 0x01)) {
		dbg << F("INFO_PEER_LIST; peer1: ") << pHex((buf+11),4);
		if (buf[0] >= 19) dbg << F(", peer2: ") << pHex((buf+15),4);
		if (buf[0] >= 23) dbg << F(", peer3: ") << pHex((buf+19),4);
		if (buf[0] >= 27) dbg << F(", peer4: ") << pHex((buf+23),4);

	} else if ((buf[3] == 0x10) && (buf[10] == 0x02)) {
		dbg << F("INFO_PARAM_RESPONSE_PAIRS; data: ") << pHex((buf+11),buf[0]-10);

	} else if ((buf[3] == 0x10) && (buf[10] == 0x03)) {
		dbg << F("INFO_PARAM_RESPONSE_SEQ; offset: ") << pHexB(buf[11]) << F(", data: ") << pHex((buf+12),buf[0]-11);

	} else if ((buf[3] == 0x10) && (buf[10] == 0x04)) {
		dbg << F("INFO_PARAMETER_CHANGE; cnl: ") << pHexB(buf[11]) << F(", peer: ") << pHex((buf+12),4) << F(", pLst: ") << pHexB(buf[16]) << F(", data: ") << pHex((buf+17),buf[0]-16);

	} else if ((buf[3] == 0x10) && (buf[10] == 0x06)) {
		dbg << F("INFO_ACTUATOR_STATUS; cnl: ") << pHexB(buf[11]) << F(", status: ") << pHexB(buf[12]) << F(", na: ") << pHexB(buf[13]);
		if (buf[0] > 13) dbg << F(", rssi: ") << pHexB(buf[14]);
		
	} else if ((buf[3] == 0x11) && (buf[10] == 0x02)) {
		dbg << F("SET; cnl: ") << pHexB(buf[11]) << F(", value: ") << pHexB(buf[12]) << F(", rampTime: ") << pHex((buf+13),2) << F(", duration: ") << pHex((buf+15),2);

	} else if ((buf[3] == 0x11) && (buf[10] == 0x03)) {
		dbg << F("STOP_CHANGE; cnl: ") << pHexB(buf[11]);

	} else if ((buf[3] == 0x11) && (buf[10] == 0x04) && (buf[11] == 0x00)) {
		dbg << F("RESET");

	} else if ((buf[3] == 0x11) && (buf[10] == 0x80)) {
		dbg << F("LED; cnl: ") << pHexB(buf[11]) << F(", color: ") << pHexB(buf[12]);

	} else if ((buf[3] == 0x11) && (buf[10] == 0x81) && (buf[11] == 0x00)) {
		dbg << F("LED_ALL; Led1To16: ") << pHex((buf+12),4);
		
	} else if ((buf[3] == 0x11) && (buf[10] == 0x81)) {
		dbg << F("LED; cnl: ") << pHexB(buf[11]) << F(", time: ") << pHexB(buf[12]) << F(", speed: ") << pHexB(buf[13]);
		
	} else if ((buf[3] == 0x11) && (buf[10] == 0x82)) {
		dbg << F("SLEEPMODE; cnl: ") << pHexB(buf[11]) << F(", mode: ") << pHexB(buf[12]);
		
	} else if ((buf[3] == 0x12)) {
		dbg << F("HAVE_DATA");
		
	} else if ((buf[3] == 0x3E)) {
		dbg << F("SWITCH; dst: ") << pHex((buf+10),3) << F(", na: ") << pHexB(buf[13]) << F(", cnl: ") << pHexB(buf[14]) << F(", counter: ") << pHexB(buf[15]);
		
	} else if ((buf[3] == 0x3F)) {
		dbg << F("TIMESTAMP; na: ") << pHex((buf+10),2) << F(", time: ") << pHex((buf+12),2);
		
	} else if ((buf[3] == 0x40)) {
		dbg << F("REMOTE; button: ") << pHexB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", counter: ") << pHexB(buf[11]);
		
	} else if ((buf[3] == 0x41)) {
		dbg << F("SENSOR_EVENT; button: ") <<pHexB((buf[10] & 0x3F)) << F(", long: ") << (buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (buf[10] & 0x80 ? 1:0) << F(", value: ") << pHexB(buf[11]) << F(", next: ") << pHexB(buf[12]);
		
	} else if ((buf[3] == 0x53)) {
		dbg << F("SENSOR_DATA; cmd: ") << pHexB(buf[10]) << F(", fld1: ") << pHexB(buf[11]) << F(", val1: ") << pHex((buf+12),2) << F(", fld2: ") << pHexB(buf[14]) << F(", val2: ") << pHex((buf+15),2) << F(", fld3: ") << pHexB(buf[17]) << F(", val3: ") << pHex((buf+18),2) << F(", fld4: ") << pHexB(buf[20]) << F(", val4: ") << pHex((buf+21),2);
		
	} else if ((buf[3] == 0x58)) {
		dbg << F("CLIMATE_EVENT; cmd: ") << pHexB(buf[10]) << F(", valvePos: ") << pHexB(buf[11]);
		
	} else if ((buf[3] == 0x70)) {
		dbg << F("WEATHER_EVENT; temp: ") << pHex((buf+10),2) << F(", hum: ") << pHexB(buf[12]);

	} else {
		dbg << F("Unknown Message, please report!");
	}
	dbg << F("\n\n");
}

// - some helpers ----------------------------------

AS hm;


// public:		//---------------------------------------------------------------------------------------------------------
uint8_t  waitTimer::done(void) {
	// todo - check if nexTime is near overflow and we have some delay, so getMillis() goes over 0
	// to get the correct timer result
	if (!armed) return 1;
	if ( nexTime > getMillis() ) return 0;
	armed = 0;
	return 1;
}
void     waitTimer::set(uint32_t ms) {
	armed = ms?1:0;
	if (armed) nexTime = getMillis() + ms -1;
}

