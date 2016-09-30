/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin send function --------------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "AS.h"
#include "Send.h"


SN snd;																						// declare send module, defined in send.h


SN::SN()  {
	DBG_START(SN, F("SN.\n") );																// ...some debug
}

void SN::poll(void) {
	#define maxRetries    3
	#define maxTime       300

	// set right amount of retries
	if (!snd_msg.max_retr) {																// first time run, check message type and set retries
		if (sndAckReq) {
			snd_msg.max_retr = maxRetries;													// if BIDI is set, we have three retries
		} else {
			snd_msg.max_retr = 1;
		}
	}
	
	//dbg << "x:" << this->retrCnt << " y:" << this->maxRetr << " t:" << sndTmr.done() << '\n';
	
	// send something while timer is not busy with waiting for an answer and max tries are not done 
	if ((snd_msg.retr_cnt < snd_msg.max_retr) && (snd_msg.timer.done() )) {					// not all sends done and timing is OK

		// some sanity
		snd_msg.mBody->FLAG.RPTEN = 1;														// every message need this flag
		//if (pHM->cFlag.active) this->mBdy.mFlg.CFG = pHM->cFlag.active;					// set the respective flag while we are in config mode
		snd_msg.timeout = 0;																// not timed out because just started
		snd_msg.prev_MSG_CNT = snd_msg.mBody->MSG_CNT;										// copy the message count to identify the ACK
		snd_msg.retr_cnt++;																	// increase counter while send out

		// check if we should send an internal message
		if (!memcmp( snd_msg.mBody->RCV_ID, dev_ident.HMID, 3)) {
			memcpy(rcv_msg.buf, snd_msg.buf, snd_msg.buf[0]+1);								// copy send buffer to received buffer
			snd_msg.retr_cnt = 0xFF;														// ACK not required, because internal
			rcv_msg.hasdata = 1;															// signalize that something is to progress

			DBG(SN, F("<i ") );

		} else {																			// send it external
			uint8_t tBurst = snd_msg.mBody->FLAG.BURST;										// get burst flag, while string will get encoded

			/*
			 * Copy the complete message to msgToSign. We need them for later AES signing.
			 * We need copy the message to position after 5 of the buffer.
			 * The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
			 * will fill with 6 bytes random data later.
			 */
			memcpy( snd_msg.prev_buf + 5, snd_msg.buf, (snd_msg.buf[0] > 26) ? 27 : snd_msg.buf[0]+1);

			cc.sndData(snd_msg.buf, tBurst);												// send to communication module
			
			if (sndAckReq) {
				snd_msg.timer.set(maxTime);													// set the time out for the message
			}
			
			DBG(SN, F("<- ") );
		}
		
		if (!led.active) {
			led.set(send);																	// fire the status led
		}
		
		DBG(SN, _HEX( snd_msg.buf, snd_msg.buf[0]+1), ' ', _TIME, '\n' );


	} else if ((snd_msg.retr_cnt >= snd_msg.max_retr) && (snd_msg.timer.done() )) {			// max retries achieved, but seems to have no answer
		snd_msg.clear();
		snd_msg.timeout = 1;																// set the time out only while an ACK or answer was requested

		if (!snd_msg.mBody->FLAG.BIDI) {
			return;
		}
		
		pom.stayAwake(100);
		led.set(noack);
		
		DBG(SN, F("  timed out"), ' ', _TIME, '\n' );
	}

	if (snd_msg.retr_cnt == 0xFF) {															// answer was received, clean up the structure
//		dbg << F(">>> clear timer") << _TIME << "\n";

		snd_msg.clear();
		pom.stayAwake(100);
		if (!led.active) led.set(ack);														// fire the status led
	}
}

void SN::cleanUp(void) {
	//snd_msg.buf[0] = 0;
}



/* 
* @brief Make broadcast, pair, peer and internal messages ready to send
* Set the right flags for the send poll function. Following flags are set: active, max_retr and timeout
* Timeout and max_retr could come from the register set of the cmMaintenance class. 
*
* Further functions are - set the message counter in the send string accordingly. On an answer we copy from the 
* received messgae, if we are initiating the message it is taken from send class counter.
*
* Sender ID is copied in the string, length of send string is set
* Msg flags, like CONFIG, BIDI, and ACK are set. 
*
*/
void SN::prep_msg( MSG_REASON::E reason, MSG_INTENT::E intent, MSG_TYPE::E type, uint8_t len = 0xff, uint8_t max_retr = 3 ) {
	// todo: max_retr could be taken from respective channel module
	uint8_t *type_arr = new uint8_t[4];

	// intent relevant things
	if ( (intent == MSG_INTENT::MASTER) && (isEmpty(MAID,3)) )								// check if we have a valid master
		intent = MSG_INTENT::BROADCAST;														// otherwise set the message to broadcast

	memset(&snd_msg.mBody->FLAG, 0, 1);														// clear the message flag while set later on

	if (intent == MSG_INTENT::BROADCAST) {
		snd_msg.max_retr = 1;																// nobody to answer, ack not required
		snd_msg.mBody->FLAG.BIDI = 0;														// no ack required while broadcast
		memset(snd_msg.mBody->RCV_ID, 0, 3);												// set receiver to 00 00 00

	} else if (intent == MSG_INTENT::INTERN) {
		snd_msg.max_retr = 1;																// internal message, no answer required
		snd_msg.mBody->FLAG.BIDI = 0;														// no ack required while internal
		memcpy(snd_msg.mBody->RCV_ID, dev_ident.HMID, 3);									// copy in the HMID as receiver

	} else if (intent == MSG_INTENT::MASTER) {
		// max_retr is set by default
		snd_msg.mBody->FLAG.BIDI = 1;														// send to master, ack required
		memcpy(snd_msg.mBody->RCV_ID, MAID, 3);												// copy in the central hm id as receiver

	} else if (intent == MSG_INTENT::PEER) {												// it is only to answer messages, so should be fine to copy address from the received string
		// max_retr is set by default
		snd_msg.mBody->FLAG.BIDI = 1;														// send to known peer, ack required
		memcpy(snd_msg.mBody->RCV_ID, rcv_msg.mBody->SND_ID, 3);							// copy in the received peer id as receiver

	}

	// message type spefic things
	#define BIG_ENDIAN ((1 >> 1 == 0) ? 0 : 1)
	#if BIG_ENDIAN
		type_arr[0] = type;
		type_arr[1] = type >> 8;
		type_arr[2] = type >> 16;
		type_arr[3] = type >> 24;
	#else
		type_arr[0] = type >> 24;
		type_arr[1] = type >> 16;
		type_arr[2] = type >> 8;
		type_arr[3] = type;
	#endif
	snd_msg.mBody->MSG_TYP = type_arr[1];
	if (type_arr[2] != 0xff) snd_msg.mBody->BY10 = type_arr[2];
	if (type_arr[3] != 0xff) snd_msg.mBody->BY11 = type_arr[3];
	//dbg << "i:" << intent << ", t:" << type << ", d:" << _HEX(type_arr, 4) << '\n';

	// if it is an ACK type message, we do not need a BIDI
	if (snd_msg.mBody->MSG_TYP == 0x02) snd_msg.mBody->FLAG.BIDI = 0;

	// general flags
	if      (reason == MSG_REASON::ANSWER) snd_msg.mBody->MSG_CNT = rcv_msg.mBody->MSG_CNT;	// copy counter from received string
	else if (reason == MSG_REASON::INITIAL) snd_msg.mBody->MSG_CNT = snd_msg.MSG_CNT++;		// copy counter from send function and increase
	snd_msg.mBody->FLAG.RPTEN = 1;															// set as standard
	snd_msg.max_time = 300; // todo: link to maintenance channel module
	snd_msg.active = 1;
}

/*void SN::prep_peer_msg() {
// handle internal messages as 

}*/

