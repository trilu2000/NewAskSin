/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin send function --------------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef SN_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif


#include "Send.h"
#include "AS.h"


waitTimer sndTmr;																			// send timer functionality


SN::SN() {
	DBG( F("SN.\n") );																		// ...some debug
}

void SN::poll(void) {
	#define maxRetries    3
	#define maxTime       300

	// set right amount of retries
	if (!this->maxRetr) {																	// first time run, check message type and set retries
		if (sndAckReq) {
			this->maxRetr = maxRetries;														// if BIDI is set, we have three retries
		} else {
			this->maxRetr = 1;
		}
	}
	
	//dbg << "x:" << this->retrCnt << " y:" << this->maxRetr << " t:" << sndTmr.done() << '\n';
	
	// send something while timer is not busy with waiting for an answer and max tries are not done 
	if ((this->retrCnt < this->maxRetr) && (sndTmr.done() )) {								// not all sends done and timing is OK

		// some sanity
		msg.mBody.FLAG.RPTEN = 1;															// every message need this flag
		//if (pHM->cFlag.active) this->mBdy.mFlg.CFG = pHM->cFlag.active;					// set the respective flag while we are in config mode
		this->timeOut = 0;																	// not timed out because just started
		prevMSG_CNT = msg.mBody.MSG_CNT;												// copy the message count to identify the ACK
		this->retrCnt++;																	// increase counter while send out

		// check if we should send an internal message
		if (!memcmp( msg.mBody.RCV_ID, HMID, 3)) {
			memcpy( rcv.buf, buf, sndStrLen );												// copy send buffer to received buffer
			this->retrCnt = 0xFF;															// ACK not required, because internal
						
			DBG( F("<i ") );

		} else {																			// send it external
			uint8_t tBurst = msg.mBody.FLAG.BURST;										// get burst flag, while string will get encoded

			/*
			 * Copy the complete message to msgToSign. We need them for later AES signing.
			 * We need copy the message to position after 5 of the buffer.
			 * The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
			 * will fill with 6 bytes random data later.
			 */
			memcpy( prev_buf + 5, buf, (sndStrLen > 27) ? 27 : sndStrLen);

			cc.sndData(buf, tBurst);													// send to communication module
			
			if (sndAckReq) {
				sndTmr.set(maxTime);														// set the time out for the message
			}
			
			DBG( F("<- ") );
		}
		
		if (!led.active) {
			led.set(send);																// fire the status led
		}
		
		DBG( _HEX( buf, sndStrLen), ' ', _TIME, '\n' );


	} else if ((this->retrCnt >= this->maxRetr) && (sndTmr.done() )) {						// max retries achieved, but seems to have no answer
		this->retrCnt = 0;
		this->maxRetr = 0;
		this->active = 0;

		if (!sndAckReq) {
			return;
		}
		
		this->timeOut = 1;																	// set the time out only while an ACK or answer was requested
		pom.stayAwake(100);
		led.set(noack);
		
		DBG( F("  timed out"), ' ', _TIME, '\n' );
	}

	if (this->retrCnt == 0xFF) {															// answer was received, clean up the structure
//		dbg << F(">>> clear timer") << _TIME << "\n";

		this->cleanUp();
		pom.stayAwake(100);
		if (!led.active) led.set(ack);														// fire the status led
	}
}

void SN::cleanUp(void) {
	buf[0] = 0;
	this->timeOut = 0;
	this->retrCnt = 0;
	this->maxRetr = 0;
	this->active = 0;
	sndTmr.set(0);
}
