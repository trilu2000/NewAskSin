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
	dbgStart();																				// serial setup
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

	// check which type of message was received
	if         (this->mBdy.mTyp == 0x00) {									// DEVICE_INFO
		pHM->recvDEVICE_INFO();												//   00

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x01)) {	// CONFIG_PEER_ADD
		pHM->recvCONFIG_PEER_ADD();											//   01          01

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x02)) {	// CONFIG_PEER_REMOVE
		pHM->recvCONFIG_PEER_REMOVE();										//   01          02

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x03)) {	// CONFIG_PEER_LIST_REQ
		pHM->recvCONFIG_PEER_LIST_REQ();									//   01          03

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x04)) {	// CONFIG_PARAM_REQ
		pHM->recvCONFIG_PARAM_REQ();										//   01          04

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x04)) {	// CONFIG_START
		pHM->recvCONFIG_START();											//   01          05

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x06)) {	// CONFIG_END
		pHM->recvCONFIG_END();												//   01          06

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x08)) {	// CONFIG_WRITE_INDEX
		pHM->recvCONFIG_WRITE_INDEX();										//   01          08

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x09)) {	// CONFIG_SERIAL_REQ
		pHM->recvCONFIG_SERIAL_REQ();										//   01          09

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x0A)) {	// PAIR_SERIAL
		pHM->recvPAIR_SERIAL();												//   01          0A

	} else if ((this->mBdy.mTyp == 0x01) && (this->mBdy.by11 == 0x0E)) {	// CONFIG_STATUS_REQUEST
		pHM->recvCONFIG_STATUS_REQUEST();									//   01          0E
	

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x00)) {	// ACK
		pHM->recvACK();														//   02    00

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x01)) {	// ACK_STATUS
		pHM->recvACK_STATUS();												//   02    01

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x02)) {	// ACK2
		pHM->recvACK2();													//   02    02

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x04)) {	// ACK_PROC
		pHM->recvACK_PROC();												//   02    04

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x80)) {	// NACK
		pHM->recvNACK();													//   02    80

	} else if ((this->mBdy.mTyp == 0x02) && (this->mBdy.by10 == 0x84)) {	// NACK_TARGET_INVALID
		pHM->recvNACK_TARGET_INVALID();										//   02    84


	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x02)) {	// SET
		pHM->recvSET();														//   11    02

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x03)) {	// STOP_CHANGE
		pHM->recvSTOP_CHANGE();												//   11    03

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x01) && (this->mBdy.by11 == 0x00)) {	// RESET
		pHM->recvRESET();													//   11    04    00

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x80)) {	// LED
		pHM->recvLED();														//   11    80

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x81) && (this->mBdy.by11 == 0x00)) {	// LEDALL
		pHM->recvLEDALL();													//   11    81    00

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x81)) {	// LEVEL
		pHM->recvLEVEL();													//   11    81

	} else if ((this->mBdy.mTyp == 0x11) && (this->mBdy.by10 == 0x82)) {	// SLEEPMODE
		pHM->recvSLEEPMODE();												//   11    82


	} else if  (this->mBdy.mTyp == 0x12) {									// HAVE_DATA
		pHM->recvHAVE_DATA();												//   12

	} else if  (this->mBdy.mTyp == 0x3E) {									// SWITCH
		pHM->recvSWITCH();													//   3E

	} else if  (this->mBdy.mTyp == 0x3F) {									// TIMESTAMP
		pHM->recvTIMESTAMP();												//   3F

	} else if  (this->mBdy.mTyp == 0x40) {									// REMOTE
		pHM->recvREMOTE();													//   40

	} else if  (this->mBdy.mTyp == 0x41) {									// SENSOR_EVENT
		pHM->recvSENSOR_EVENT();											//   41

	} else if  (this->mBdy.mTyp == 0x53) {									// SENSOR_DATA
		pHM->recvSENSOR_DATA();												//   53

	} else if  (this->mBdy.mTyp == 0x58) {									// CLIMATE_EVENT
		pHM->recvCLIMATE_EVENT();											//   58

	} else if  (this->mBdy.mTyp == 0x70) {									// WEATHER_EVENT
		pHM->recvWEATHER_EVENT();											//   70

	}

	this->mBdy.mLen = 0;
}
