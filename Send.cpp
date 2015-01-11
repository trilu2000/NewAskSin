//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin send function --------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define SN_DBG
#include "Send.h"
#include "AS.h"

// private:		//---------------------------------------------------------------------------------------------------------
#define sndLen       this->buf[0]+1															// amount of bytes in the send buffer
#define reqACK       this->mBdy.mFlg.BIDI													// check if an ACK is requested

waitTimer sndTmr;																			// send timer functionality

SN::SN() {
} 
void SN::init(AS *ptrMain) {
	#ifdef SN_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("SN.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
	buf = (uint8_t*)&mBdy;
}
void SN::poll(void) {
	#define maxRetries    3
	#define maxTime       300
	
	// set right amount of retries
	if (!this->maxRetr) {																	// first time run, check message type and set retries
		if (reqACK) this->maxRetr = maxRetries;												// if BIDI is set, we have three retries
		else this->maxRetr = 1;
	}
	
	//dbg << "x:" << this->retrCnt << " y:" << this->maxRetr << " t:" << sndTmr.done() << '\n';
	
	// send something while timer is not busy with waiting for an answer and max tries are not done 
	if ((this->retrCnt < this->maxRetr) && (sndTmr.done() )) {								// not all sends done and timing is OK

		// some sanity
		this->mBdy.mFlg.RPTEN = 1;															// every message need this flag
		//if (pHM->cFlag.active) this->mBdy.mFlg.CFG = pHM->cFlag.active;						// set the respective flag while we are in config mode
		this->timeOut = 0;																	// not timed out because just started
		this->lastMsgCnt = this->mBdy.mCnt;													// copy the message count to identify the ACK
		this->retrCnt++;																	// increase counter while send out

		// check if we should send an internal message
		if (compArray(this->mBdy.toID, HMID, 3)) {											// message is addressed to us
			memcpy(pHM->rv.buf, this->buf, sndLen);											// copy send buffer to received buffer
			this->retrCnt = 0xff;															// ACK not required, because internal
						
			#ifdef SN_DBG																	// only if AS debug is set
			dbg << F("<i ");
			#endif

		} else {																			// send it external
			uint8_t tBurst = this->mBdy.mFlg.BURST;											// get burst flag, while string will get encoded
			pHM->encode(this->buf);															// encode the string
			disableGDO0Int();
			pHM->cc.sndData(this->buf,tBurst);												// send to communication module
			enableGDO0Int();
			pHM->decode(this->buf);															// decode the string, so it is readable next time
			
			if (reqACK) sndTmr.set(maxTime);												// set the time out for the message
			
			#ifdef SN_DBG																	// only if AS debug is set
			dbg << F("<- ");
			#endif

		}
		
		if (!pHM->ld.active) pHM->ld.set(send);												// fire the status led
		
		#ifdef SN_DBG																		// only if AS debug is set
		dbg << _HEX(this->buf,sndLen) << ' ' << _TIME << '\n';
		#endif

	} else if ((this->retrCnt >= this->maxRetr) && (sndTmr.done() )) {						// max retries achieved, but seems to have no answer
		this->retrCnt = 0;
		this->maxRetr = 0;
		this->active = 0;
		if (!reqACK) return;
		
		this->timeOut = 1;																	// set the time out only while an ACK or answer was requested
		pHM->pw.stayAwake(100);
		pHM->ld.set(noack);
		
		#ifdef SN_DBG																		// only if AS debug is set
		dbg << F("  timed out") << ' ' << _TIME << '\n';
		#endif
	}

	if (this->retrCnt == 0xff) {															// answer was received, clean up the structure
		this->timeOut = 0;
		this->retrCnt = 0;
		this->maxRetr = 0;
		this->active = 0;
		sndTmr.set(0);
		
		pHM->pw.stayAwake(100);
		if (!pHM->ld.active) pHM->ld.set(ack);												// fire the status led
	}

	
}
