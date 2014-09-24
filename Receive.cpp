//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin receive function -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define RV_DBG
#define RV_DBG_EX
#include "Receive.h"
#include "AS.h"


// private:		//---------------------------------------------------------------------------------------------------------
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

	pHM->recvMessage();

	this->mBdy.mLen = 0;
}
