/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin registrar functions -----------------------------------------------------------------------------------------------
* - with a lot of support from Dirk at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#define SN_DBG
#include "Send.h"
#include "AS.h"

waitTimer sndTmr;																			// send timer functionality

SN::SN() {
	#ifdef SN_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("SN.\n");																		// ...and some information
	#endif
}


void SN::poll(void) {
	#define maxTime       300
	if (!active) return;																	// nothing to do

	if (retrCnt == 0xFF) {																	// answer was received, clean up the structure
		// dbg << F(">>> clear timer") << _TIME << "\n";
		cleanUp();
		hm.pw.stayAwake(100);																// add some time to stay awake
		if (!hm.ld.active) hm.ld.set(ack);													// fire the status led
	}

	if (!sndTmr.done()) return;																// send is active, but timer is still running

	// set right amount of retries
	if (reqACK) retrSnd = maxRetr;															// if BIDI is set, we have three retries
	else retrSnd = 1;

	//dbg << "x:" << this->retrCnt << " y:" << this->maxRetr << " t:" << sndTmr.done() << '\n';
	
	// send something while timer is not busy with waiting for an answer and max tries are not done 
	if ( retrCnt < retrSnd ) {																// not all sends done and timing is OK

		// some sanity
		mBdy.mFlg.RPTEN = 1;																// every message need this flag
		//if (pHM->cFlag.active) mBdy.mFlg.CFG = pHM->cFlag.active;							// set the respective flag while we are in config mode
		timeOut = 0;																		// not timed out because just started
		lastMsgCnt = mBdy.mCnt;																// copy the message count to identify the ACK
		retrCnt++;																			// increase counter while send out

		// check if we should send an internal message
		if ( !memcmp( mBdy.toID, HMID, 3) ) {
			memcpy(hm.rv.buf, buf, sndLen);													// copy send buffer to received buffer
			retrCnt = 0xFF;																	// ACK not required, because internal
						
			#ifdef SN_DBG																	// only if AS debug is set
			dbg << F("<i ");
			#endif

		} else {																			// send it external
			/*
			 * Copy the complete message to msgToSign. We need them for later AES signing.
			 * We need copy the message to position after 5 of the buffer.
			 * The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
			 * will fill with 6 bytes random data later.
			 */
			memcpy( msgToSign+5, buf, (sndLen > 27) ? 27 : sndLen );
				
			hm.encode( buf );																// encode the string
			hm.cc.sndData( buf, mBdy.mFlg.BURST );											// send to communication module
			hm.decode( buf );																// decode the string, so it is readable next time
			if (reqACK) sndTmr.set(maxTime);												// set the time out for the message

			#ifdef SN_DBG																	// only if AS debug is set
			dbg << F("<- ");
			#endif
		}
		
		if (!hm.ld.active) hm.ld.set(send);													// fire the status led
		#ifdef SN_DBG																		// only if AS debug is set
		dbg << _HEX(buf, sndLen) << ' ' << _TIME << '\n';
		#endif

	} else {																				// max retries achieved, but seems to have no answer
		cleanUp();
		if (!reqACK) return;																// ack was not requested, nothing to do anymore
	
		timeOut = 1;																		// set the time out only while an ACK or answer was requested
		hm.pw.stayAwake(100);																// add 100 ms to the wakeup timer to clean up and blink the led
		hm.ld.set(noack);
	
		#ifdef SN_DBG																		// only if AS debug is set
		dbg << F("  timed out") << ' ' << _TIME << '\n';
		#endif
	}
}

void SN::cleanUp(void) {
	timeOut = retrCnt = retrSnd = active = 0;
	sndTmr.set(0);
}
