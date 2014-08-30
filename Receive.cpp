//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin receive function ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define RV_DBG
//#define RV_DBG_EX
#include "Receive.h"
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------
RV::RV() {
} 

void    RV::init(AS *ptrMain) {
	#ifdef RV_DBG																			// only if ee debug is set
	if (!&dbg) dbg.begin(57600);
	//dbg.begin(57600);																		// serial setup
	dbg << F("\n....\n");																	// ...and some information
	dbg << F("RV.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void	RV::poll(void) {
	static uint8_t last_rCnt;
	
	uint8_t bIntend = pHM->ee.getIntend(this->mBdy.reID,this->mBdy.toID);					// get the intend of the message

	// some debugs
	#ifdef RV_DBG																			// only if AS debug is set
	dbg << (char)bIntend << F("> ") << pHex(this->buf,this->bufLen) << ' ' << pTime << '\n';
	#endif
	
	#ifdef RV_DBG_EX																		// only if extended AS debug is set
	pHM->explainMessage(this->buf);
	#endif
	
	// filter out unknown or not for us
	if ((bIntend == 'l') || (bIntend == 'u')) {												// not for us, or sender unknown
		this->mBdy.mLen = 0;																// clear receive buffer
		return;
	}

	// filter out repeated messages
	if ((this->mBdy.mFlg.RPTED) && (last_rCnt == this->mBdy.mLen)) {						// check if message was already received
		#ifdef RV_DBG																		// only if AS debug is set
		dbg << F("  repeated message\n");
		#endif

		this->mBdy.mLen = 0;																// clear receive buffer
		return;																				// wait for next message
	}
	last_rCnt = this->mBdy.mLen;

	pHM->received();
	
/*	// check which type of message was received
	if         ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x01)) {			// CONFIG_PEER_ADD
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 01 02 04 01   01  1F A6 5C  06         05
		// do something with the information ----------------------------------

		// first call remPeer to avoid doubles
		uint8_t ret = ee.addPeer(this->mBdy.by10,this->buf+12);										// send to addPeer function
		
		// let module registrations know of the change

		if ((ret) && (this->ackRq)) sendACK();													// send appropriate answer
		else if (this->ackRq) sendNACK();
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x02)) {			// CONFIG_PEER_REMOVE
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 01 02 04 01   02  1F A6 5C  06         05
		// do something with the information ----------------------------------
		
		uint8_t ret = ee.remPeer(this->mBdy.by10,this->buf+12);										// call the remPeer function
		if (this->ackRq) sendACK();																// send appropriate answer
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x03)) {			// CONFIG_PEER_LIST_REQ
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 05 A0 01 63 19 63 01 02 04 01  03
		// do something with the information ----------------------------------
		
		sList.totSlc = ee.countPeerSlc(this->mBdy.by10);											// how many slices are need
		sList.mCnt = this->mBdy.mLen;																// remember the message count
		memcpy(sList.toID,this->mBdy.reID,3);
		sList.cnl = this->mBdy.by10;																// send input to the send peer function
		sList.peer = 1;																		// set the type of answer
		sList.active = 1;																	// start the send function
		// answer will send from sendsList(void)
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x04)) {			// CONFIG_PARAM_REQ
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 04 A0 01 63 19 63 01 02 04 01  04 00 00 00  00       01
		// do something with the information ----------------------------------

		sList.idx = ee.getIdxByPeer(this->mBdy.by10, this->buf+12);									// fill struct
		sList.totSlc = ee.countRegListSlc(this->mBdy.by10, this->buf[16]);							// how many slices are need
		sList.mCnt = this->mBdy.mLen;																// remember the message count
		memcpy(sList.toID,this->mBdy.reID,3);
		sList.cnl = this->mBdy.by10;																// send input to the send peer function
		sList.lst = this->buf[16];																// send input to the send peer function
		sList.reg2 = 1;																		// set the type of answer
		
		if ((sList.idx != 0xff) && (sList.totSlc > 0)) sList.active = 1;					// only send register content if something is to send															// start the send function
		else memset((void*)&sList,0,10);													// otherwise empty variable
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x05)) {			// CONFIG_START
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 01 A0 01 63 19 63 01 02 04 00  05 00 00 00  00       00
		// do something with the information ----------------------------------

		cFlag.idx = ee.getIdxByPeer(this->mBdy.by10, this->buf+12);									// fill structure to remember where to write
		cFlag.cnl = this->mBdy.by10;
		cFlag.lst = this->buf[16];
		if (cFlag.idx != 0xff) {
			cFlag.active = 1;																// set active if there is no error on index
			// set message id flag to config in send module
		}
		
		if (this->ackRq) sendACK();																// send appropriate answer
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x06)) {			// CONFIG_END
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 01 A0 01 63 19 63 01 02 04 00  06
		// do something with the information ----------------------------------

		cFlag.active = 0;																	// set inactive
		if ((cFlag.cnl == 0) && (cFlag.idx == 0)) ee.getMasterID();
		// remove message id flag to config in send module
		
		if (this->ackRq) sendACK();																// send appropriate answer
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x07)) {			// CONFIG_WRITE_INDEX
		// sample needed

		
		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x08)) {			// CONFIG_WRITE_INDEX
		// description --------------------------------------------------------
		//                                  Cnl    Data
		// l> 13 02 A0 01 63 19 63 01 02 04 00  08 02 01 0A 63 0B 19 0C 63
		// do something with the information ----------------------------------

		if ((cFlag.active) && (cFlag.cnl == this->mBdy.by10)) {									// check if we are in config mode and if the channel fit
			ee.setListArray(cFlag.cnl, cFlag.lst, cFlag.idx, this->buf[0]+1-11, this->buf+12);			// write the string to eeprom
		}
		// reload master id while cnl was 0 and lst was 0
		if (this->ackRq) sendACK();																// send appropriate answer
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x09)) {			// CONFIG_SERIAL_REQ
		// description --------------------------------------------------------
		//
		// l> 0B 77 A0 01 63 19 63 01 02 04 00 09
		// do something with the information ----------------------------------
		sendINFO_SERIAL();																	// jump to create the answer
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x0A)) {			// PAIR_SERIAL
		// description --------------------------------------------------------
		//
		// b> 15 93 B4 01 63 19 63 00 00 00 01 0A 4B 45 51 30 32 33 37 33 39 36
		// do something with the information ----------------------------------
		if (cmpAry(this->buf+12,HMSR,10)) sendDEVICE_INFO();									// compare serial and send device info
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x0E)) {			// CONFIG_STATUS_REQUEST
		//CHANNEL => "0,2", } },


		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x00)) {			// ACK
		// description --------------------------------------------------------
		//
		// l> 0A 05 80 02 63 19 63 01 02 04 00
		// do something with the information ----------------------------------
		
		if ((sn.active) && (this->mBdy.mLen == sn.lastMsgCnt)) sn.retrCnt = 0xff;					// was an ACK to an active message, message counter is similar - set retrCnt to 255
		//dbg << "act:" << sn.active << " rC:" << this->mBdy.mLen << " sC:" << sn.lastMsgCnt << " cntr:" << sn.retrCnt << '\n';
		// --------------------------------------------------------------------
		
		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x01)) {			// ACK_STATUS
		// description --------------------------------------------------------
		// <- 0B 08 B4 40 23 70 D8 1F B7 4A 02 08
		//                                      cnl stat DUL RSSI
		// l> 0E 08 80 02 1F B7 4A 23 70 D8 01  01  C8   80  27
		// do something with the information ----------------------------------
		// DUL = UP 10, DOWN 20, LOWBAT 80
		
		if ((sn.active) && (this->mBdy.mLen == sn.lastMsgCnt)) sn.retrCnt = 0xff;					// was an ACK to an active message, message counter is similar - set retrCnt to 255
		// --------------------------------------------------------------------

		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x02)) {			// ACK2 - smokeDetector pairing only?


		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x04)) {			// ACK-proc - connected to AES??
		//Para1          => "02,4",
		//Para2          => "06,4",
		//Para3          => "10,4",
		//Para4          => "14,2",}}, # remote?

		
		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x80)) {			// NACK
		// for test
		static uint8_t x2[2];
		x2[0] = 0x02;
		x2[1] += 1;
		sendREMOTE(1,1,x2);
		
		} else if  ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x84)) {			// NACK_TARGET_INVALID


		} else if  ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x04)) {			// RESET
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

		} else if  (this->mBdy.mTyp == 0x12) {									// HAVE_DATA

		
		} else if  (this->mBdy.mTyp == 0x3E) {									// SWITCH
		//DST      => "00,6",
		//UNKNOWN  => "06,2",
		//CHANNEL  => "08,2",
		//COUNTER  => "10,2", } },


		} else if  (this->mBdy.mTyp == 0x3F) {									// TimeStamp
		//UNKNOWN  => "00,4",
		//TIME     => "04,2", } },


		} else if  (this->mBdy.mTyp == 0x40) {									// REMOTE
		//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
		//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
		//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
		//COUNTER  => "02,2", } },


		} else if  (this->mBdy.mTyp == 0x41) {									// Sensor_event
		//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
		//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
		//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
		//NBR      => '02,2,$val=(hex($val))',
		//VALUE    => '04,2,$val=(hex($val))',} },


		} else if  (this->mBdy.mTyp == 0x53) {									// SensorData
		//CMD => "00,2",
		//Fld1=> "02,2",
		//Val1=> '04,4,$val=(hex($val))',
		//Fld2=> "08,2",
		//Val2=> '10,4,$val=(hex($val))',
		//Fld3=> "14,2",
		//Val3=> '16,4,$val=(hex($val))',
		//Fld4=> "20,2",
		//Val4=> '24,4,$val=(hex($val))'} },


		} else if  (this->mBdy.mTyp == 0x58) {									// ClimateEvent
		//CMD      => "00,2",
		//ValvePos => '02,2,$val=(hex($val))', } },
		
		
		} else if  (this->mBdy.mTyp == 0x59) {									// setTeamTemp
		//CMD      => "00,2",
		//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
		//mode     => '02,2,$val=(hex($val) & 0x3)',} },


		} else if  (this->mBdy.mTyp == 0x70) {									// WeatherEvent
		//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
		//HUM      => '04,2,$val=(hex($val))', } },


	}
*/
	this->mBdy.mLen = 0;

}
