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


#include "AS.h"
#include "Send.h"

SN snd;																						// declare send module, defined in send.h


SN::SN() {
	DBG( F("SN.\n") );																		// ...some debug
}

void SN::poll(void) {
	#define maxRetries    3
	#define maxTime       300

	// set right amount of retries
	if (!flag.max_retr) {																	// first time run, check message type and set retries
		if (sndAckReq) {
			flag.max_retr = maxRetries;														// if BIDI is set, we have three retries
		} else {
			flag.max_retr = 1;
		}
	}
	
	//dbg << "x:" << this->retrCnt << " y:" << this->maxRetr << " t:" << sndTmr.done() << '\n';
	
	// send something while timer is not busy with waiting for an answer and max tries are not done 
	if ((flag.retr_cnt < flag.max_retr) && (flag.timer.done() )) {								// not all sends done and timing is OK

		// some sanity
		msg.mBody.FLAG.RPTEN = 1;															// every message need this flag
		//if (pHM->cFlag.active) this->mBdy.mFlg.CFG = pHM->cFlag.active;					// set the respective flag while we are in config mode
		flag.timeout = 0;																	// not timed out because just started
		prev_msg_cnt = msg.mBody.MSG_CNT;													// copy the message count to identify the ACK
		flag.retr_cnt++;																	// increase counter while send out

		// check if we should send an internal message
		if (!memcmp( msg.mBody.RCV_ID, dev_ident.HMID, 3)) {
			memcpy( rcv.buf, buf, sndStrLen );												// copy send buffer to received buffer
			flag.retr_cnt = 0xFF;															// ACK not required, because internal
						
			DBG( F("<i ") );

		} else {																			// send it external
			uint8_t tBurst = msg.mBody.FLAG.BURST;											// get burst flag, while string will get encoded

			/*
			 * Copy the complete message to msgToSign. We need them for later AES signing.
			 * We need copy the message to position after 5 of the buffer.
			 * The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
			 * will fill with 6 bytes random data later.
			 */
			memcpy( prev_buf + 5, buf, (sndStrLen > 27) ? 27 : sndStrLen);

			cc.sndData(buf, tBurst);														// send to communication module
			
			if (sndAckReq) {
				flag.timer.set(maxTime);														// set the time out for the message
			}
			
			DBG( F("<- ") );
		}
		
		if (!led.active) {
			led.set(send);																	// fire the status led
		}
		
		DBG( _HEX( buf, sndStrLen), ' ', _TIME, '\n' );


	} else if ((flag.retr_cnt >= flag.max_retr) && (flag.timer.done() )) {						// max retries achieved, but seems to have no answer
		cleanUp();
		flag.timeout = 1;																	// set the time out only while an ACK or answer was requested

		if (!sndAckReq) {
			return;
		}
		
		pom.stayAwake(100);
		led.set(noack);
		
		DBG( F("  timed out"), ' ', _TIME, '\n' );
	}

	if (flag.retr_cnt == 0xFF) {															// answer was received, clean up the structure
//		dbg << F(">>> clear timer") << _TIME << "\n";

		cleanUp();
		pom.stayAwake(100);
		if (!led.active) led.set(ack);														// fire the status led
	}
}

void SN::cleanUp(void) {
	buf[0] = 0;
	flag = {};
}



/* 
* @brief Make broadcast, pair and internal messages ready to send
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
void SN::prep_nonpeer_msg( MSG_REASON::E reason, MSG_INTENT::E intent, MSG_TYPE::E type, uint8_t len, uint8_t max_retr = 3 ) {
	// todo: max_retr could be taken from respective channel module
	uint8_t *type_arr = new uint8_t[4];

	// intent relevant things
	if ( (intent == MSG_INTENT::MASTER) && (isEmpty(MAID,3)) )								// check if we have a valid master
		intent = MSG_INTENT::BROADCAST;														// otherwise set the message to broadcast

	memset(&msg.mBody.FLAG, 0, 1);															// clear the message flag while set later on

	if (intent == MSG_INTENT::BROADCAST) {
		flag.max_retr = 1;																	// nobody to answer, ack not required
		msg.mBody.FLAG.BIDI = 0;															// no ack required while broadcast
		memset(snd.msg.mBody.RCV_ID, 0, 3);													// set receiver to 00 00 00

	} else if (intent == MSG_INTENT::INTERN) {
		flag.max_retr = 1;																	// internal message, no answer required
		msg.mBody.FLAG.BIDI = 0;															// no ack required while internal
		memcpy(snd.msg.mBody.RCV_ID, dev_ident.HMID, 3);									// copy in the HMID as receiver

	} else if (intent == MSG_INTENT::MASTER) {
		// max_retr is set by default
		msg.mBody.FLAG.BIDI = 1;															// send to master, ack required
		memcpy(snd.msg.mBody.RCV_ID, MAID, 3);												// copy in the central hm id as receiver
	
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
	msg.mBody.MSG_TYP = type_arr[1];
	if (type_arr[2] != 0xff) msg.mBody.BY10 = type_arr[2];
	if (type_arr[3] != 0xff) msg.mBody.BY11 = type_arr[3];
	//dbg << "i:" << intent << ", t:" << type << ", d:" << _HEX(type_arr, 4) << '\n';

	// general flags
	if      (reason == MSG_REASON::ANSWER) msg.mBody.MSG_CNT = rcv.msg.mBody.MSG_CNT;		// copy counter from received string
	else if (reason == MSG_REASON::ANSWER) msg.mBody.MSG_CNT = msg_cnt++;					// copy counter from send function and increase
	msg.mBody.FLAG.RPTEN = 1;																// set as standard
	flag.max_time = 300; // todo: link to maintenance channel module
	flag.active = 1;
}

/*void SN::prep_peer_msg() {
// handle internal messages as 

}*/

