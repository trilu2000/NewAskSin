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
	process_config_list_answer_slice();														// check if something has to be send slice wise
	if (!snd_msg.active) return;															// nothing to do

	/*  return while no ACK received and timer is running */
	if ((!snd_msg.timer.done()) && (snd_msg.retr_cnt != 0xff)) return;						

	/* can only happen while an ack was received and AS:processMessage had send the retr_cnt to 0xff */
	if (snd_msg.retr_cnt == 0xff) {
		snd_msg.clear();																	// nothing to do any more
		led.set(ack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while
		//DBG(SN, F("ACK detected...\n") );
		return;
	}

	/* check for first time and prepare the send */
	if (!snd_msg.retr_cnt) {

		/* check if we should send an internal message */
		if (isEqual(snd_msg.mBody.RCV_ID, dev_ident.HMID, 3)) {
			memcpy(rcv_msg.buf, snd_msg.buf, snd_msg.buf[0] + 1);							// copy send buffer to received buffer
			DBG(SN, F("<i ...\n"));															// some debug, message is shown in the received string
			rcv.poll();																		// get intent and so on...
			snd_msg.clear();																// nothing to do any more for send, msg will processed in the receive loop
			return;																			// and return...
		}

		/* internal messages doesn't matter anymore*/
		memcpy(snd_msg.mBody.SND_ID, dev_ident.HMID, 3);									// we always send the message in our name
		snd_msg.mBody.FLAG.RPTEN = 1;														// every message need this flag
		snd_msg.temp_MSG_CNT = snd_msg.mBody.MSG_CNT;										// copy the message count to identify the ACK
		if (isEmpty(snd_msg.mBody.RCV_ID,3)) snd_msg.mBody.FLAG.BIDI = 0;					// broadcast, no ack required

		if (!snd_msg.temp_max_retr)
			snd_msg.temp_max_retr = (snd_msg.mBody.FLAG.BIDI) ? snd_msg.max_retr : 1;		// send once while not requesting an ACK

		/* Copy the complete message to msgToSign. We need them for later AES signing.
		*  We need copy the message to position after 5 of the buffer.
		*  The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
		*  will fill with 6 bytes random data later.	*/
		memcpy(&snd_msg.prev_buf[5], snd_msg.buf, (snd_msg.buf[0] > 26) ? 27 : snd_msg.buf[0] + 1);
	}
	
	/* check the retr count if there is something to send, while message timer was checked earlier */
	if (snd_msg.retr_cnt < snd_msg.temp_max_retr)  {										// not all sends done and timing is OK
		uint8_t tBurst = snd_msg.mBody.FLAG.BURST;											// get burst flag, while string will get encoded
		cc.sndData(snd_msg.buf, tBurst);													// send to communication module
		snd_msg.retr_cnt++;																	// remember that we had send the message

		if (snd_msg.mBody.FLAG.BIDI) snd_msg.timer.set(snd_msg.max_time);					// timeout is only needed while an ACK is requested
		led.set(send);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("<- "), _HEX(snd_msg.buf, snd_msg.buf[0] + 1), ' ', _TIME, '\n');			// some debug

	} else {
		/* if we are here, message was send one or multiple times and the timeout was raised if an ack where required */
		/* seems, nobody had got our message, other wise we had received an ACK */
		snd_msg.clear();
		if (!snd_msg.mBody.FLAG.BIDI) return;												// everything fine, ACK was not required

		snd_msg.timeout = 1;																// set the time out only while an ACK or answer was requested

		led.set(noack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("  timed out"), ' ', _TIME, '\n');										// some debug
	}

	
}

void SN::process_config_list_answer_slice(void) {
	if (!config_list_answer_slice.active) return;											// nothing to send, return
	if (snd_msg.active) return;																// we have something to do, but send_msg is busy

	s_config_list_answer_slice *cl = &config_list_answer_slice;								// short hand
	uint8_t payload_len;

	if (cl->type == LIST_ANSWER::PEER_LIST) {						// INFO_PEER_LIST
		dbg << "cur:" << cl->cur_slc << ", max:" << cl->max_slc << '\n';
		payload_len = cl->peer->get_slice(cl->cur_slc, snd_msg.buf + 11);					// get the slice and the amount of bytes
		snd_msg.mBody.MSG_CNT = snd_msg.MSG_CNT++;											// set the message counter
		snd_msg.set_msg(MSG_TYPE::INFO_PEER_LIST, MAID, 1, payload_len + 10);				// set message type, RCV_ID, SND_ID and set it active
		cl->cur_slc++;																		// increase slice counter
		//dbg << "peer slc: " << _HEX(snd_msg.buf, snd_msg.buf[0] + 1) << '\n';

	} else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_PAIRS) {		// INFO_PARAM_RESPONSE_PAIRS
		dbg << "cur:" << cl->cur_slc << ", max:" << cl->max_slc << '\n';
		if ((cl->cur_slc + 1) < cl->max_slc) {												// within message processing, get the content													
			payload_len = cl->list->get_slice_pairs(cl->peer_idx, cl->cur_slc, snd_msg.buf + 11);// get the slice and the amount of bytes
		} else {																			// last slice, terminating message
			payload_len = 2;																// reflect it in the payload_len
			memset(snd_msg.buf + 11, 0, payload_len);										// write terminating zeros
		}
		snd_msg.mBody.MSG_CNT = snd_msg.MSG_CNT++;											// set the message counter
		snd_msg.set_msg(MSG_TYPE::INFO_PARAM_RESPONSE_PAIRS, MAID, 1, payload_len + 10);	// set message type, RCV_ID, SND_ID and set it active
		cl->cur_slc++;																		// increase slice counter
		//dbg << "reg2 slc: " << _HEX(snd_msg.buf, snd_msg.buf[0] + 1) << '\n';				

	} else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_SEQ) {		// INFO_PARAM_RESPONSE_SEQ
		// not implemented yet
	}

	if (cl->cur_slc >= cl->max_slc) {														// if everything is send, we could stop the struct
		cl->active = 0;
		cl->cur_slc = 0;
	}
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
/*void SN::prep_msg( MSG_REASON::E reason, MSG_INTENT::E intent, MSG_TYPE::E type, uint8_t len = 0xff, uint8_t max_retr = 3 ) {
	// todo: max_retr could be taken from respective channel module
	uint8_t *type_arr = new uint8_t[4];

	// intent relevant things
	if ( (intent == MSG_INTENT::MASTER) && (isEmpty(MAID,3)) )								// check if we have a valid master
		intent = MSG_INTENT::BROADCAST;														// otherwise set the message to broadcast


	if (intent == MSG_INTENT::BROADCAST) {
		snd_msg.max_retr = 1;																// nobody to answer, ack not required
		snd_msg.mBody.FLAG.BIDI = 0;														// no ack required while broadcast
		memset(snd_msg.mBody.RCV_ID, 0, 3);												// set receiver to 00 00 00

	} else if (intent == MSG_INTENT::INTERN) {
		snd_msg.max_retr = 1;																// internal message, no answer required
		snd_msg.mBody.FLAG.BIDI = 0;														// no ack required while internal
		memcpy(snd_msg.mBody.RCV_ID, dev_ident.HMID, 3);									// copy in the HMID as receiver

	} else if (intent == MSG_INTENT::MASTER) {
		// max_retr is set by default
		snd_msg.mBody.FLAG.BIDI = 1;														// send to master, ack required
		memcpy(snd_msg.mBody.RCV_ID, MAID, 3);												// copy in the central hm id as receiver

	} else if (intent == MSG_INTENT::PEER) {												// it is only to answer messages, so should be fine to copy address from the received string
		// max_retr is set by default
		snd_msg.mBody.FLAG.BIDI = 1;														// send to known peer, ack required
		memcpy(snd_msg.mBody.RCV_ID, rcv_msg.mBody.SND_ID, 3);							// copy in the received peer id as receiver

	}

	//dbg << "i:" << intent << ", t:" << type << ", d:" << _HEX(type_arr, 4) << '\n';

	// if it is an ACK type message, we do not need a BIDI
	if (snd_msg.mBody.MSG_TYP == 0x02) snd_msg.mBody.FLAG.BIDI = 0;

	// general flags
	if      (reason == MSG_REASON::ANSWER) snd_msg.mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;	// copy counter from received string
	else if (reason == MSG_REASON::INITIAL) snd_msg.mBody.MSG_CNT = snd_msg.MSG_CNT++;		// copy counter from send function and increase
	snd_msg.mBody.FLAG.RPTEN = 1;															// set as standard
	snd_msg.max_time = 300; // todo: link to maintenance channel module
	snd_msg.active = 1;
}*/

/*void SN::prep_peer_msg() {
// handle internal messages as 

}*/

