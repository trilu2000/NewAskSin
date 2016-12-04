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


void SN::poll(void) {
	s_snd_msg *sm = &snd_msg;																// short hand to snd_msg struct

	process_config_list_answer_slice();														// check if something has to be send slice wise
	if (sm->active == MSG_ACTIVE::NONE) return;												// nothing to do

	/* can only happen while an ack was received and AS:processMessage had send the retr_cnt to 0xff */
	if (sm->retr_cnt == 0xff) {
		sm->clear();																		// nothing to do any more

		led.set(ack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while
		return;
	}

	/*  return while no ACK received and timer is running */
	if (!sm->timer.done()) return;


	/* check for first time and prepare the send */
	if (!sm->retr_cnt) {

		/* check based on active flag if it is a message which needs to be prepared or only processed */
		if (sm->active >= MSG_ACTIVE::ANSWER) {
			sm->mBody.FLAG.RPTEN = 1;														// every message need this flag
			sm->mBody.FLAG.BIDI = 0;														// ACK required, default no?

			memcpy(sm->mBody.SND_ID, dev_ident.HMID, 3);									// we always send the message in our name

			sm->mBody.MSG_TYP = BY03(sm->type);												// msg type
			if (BY10(sm->type) != 0xff) sm->mBody.BY10 = BY10(sm->type);					// byte 10
			if (BY11(sm->type) != 0xff) sm->mBody.BY11 = BY11(sm->type);					// byte 11
			if (MLEN(sm->type) != 0xff) sm->mBody.MSG_LEN = MLEN(sm->type);					// msg len
		}

		/* now more in detail in regards to the active flag */
		if (sm->active == MSG_ACTIVE::ANSWER) {
			/* answer means - msg_cnt and rcv_id from received string, no bidi needed, but bidi is per default off */
			memcpy(sm->mBody.RCV_ID, rcv_msg.mBody.SND_ID, 3);
			sm->mBody.MSG_CNT = rcv_msg.mBody.MSG_CNT;

		} else if (sm->active == MSG_ACTIVE::PAIR) {
			/* pair means - msg_cnt from snd_msg struct, rcv_id is master_id, bidi needed */
			memcpy(sm->mBody.RCV_ID, dev_operate.MAID, 3);
			sm->mBody.MSG_CNT = sm->MSG_CNT;
			sm->MSG_CNT++;
			sm->mBody.FLAG.BIDI = 1;														// ACK required, will be detected later if not paired 

		} else if (sm->active == MSG_ACTIVE::PEER_BIDI) {
			sm->mBody.FLAG.BIDI = 1;														// ACK required, will be detected later if not paired 
		}


		/* an internal message which is only to forward while already prepared,
		* other options are internal but need to be prepared, external message are differs to whom they have to be send and if they
		* are initial send or as an answer to a received message. all necassary information are in the send struct */
		if (isEqual(sm->mBody.RCV_ID, dev_ident.HMID, 3)) {
			memcpy(rcv_msg.buf, sm->buf, sm->buf[0] + 1);									// copy send buffer to received buffer
			DBG(SN, F("<i ...\n"));															// some debug, message is shown in the received string
			rcv.poll();																		// get intent and so on...
			sm->clear();																	// nothing to do any more for send, msg will processed in the receive loop
			return;																			// and return...
		}

		/* internal messages doesn't matter anymore*/
		sm->temp_MSG_CNT = sm->mBody.MSG_CNT;												// copy the message count to identify the ACK
		if (isEmpty(sm->mBody.RCV_ID, 3)) sm->mBody.FLAG.BIDI = 0;							// broadcast, no ack required

		if (!sm->temp_max_retr)
			sm->temp_max_retr = (sm->mBody.FLAG.BIDI) ? sm->max_retr : 1;					// send once while not requesting an ACK

		/* Copy the complete message to msgToSign. We need them for later AES signing.
		*  We need copy the message to position after 5 of the buffer.
		*  The bytes 0-5 remain free. These 5 bytes and the first byte of the copied message
		*  will fill with 6 bytes random data later.	*/
		memcpy(&sm->prev_buf[5], sm->buf, (sm->buf[0] > 26) ? 27 : sm->buf[0] + 1);
	}

	
	/* check the retr count if there is something to send, while message timer was checked earlier */
	if (sm->retr_cnt < sm->temp_max_retr)  {												// not all sends done and timing is OK
		uint8_t tBurst = sm->mBody.FLAG.BURST;												// get burst flag, while string will get encoded
		cc.sndData(sm->buf, tBurst);														// send to communication module
		sm->retr_cnt++;																		// remember that we had send the message

		if (sm->mBody.FLAG.BIDI) sm->timer.set(sm->max_time);								// timeout is only needed while an ACK is requested
		led.set(send);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("<- "), _HEX(sm->buf, sm->buf[0] + 1), ' ', _TIME, '\n');					// some debug

	} else {
		/* if we are here, message was send one or multiple times and the timeout was raised if an ack where required */
		/* seems, nobody had got our message, other wise we had received an ACK */
		sm->clear();																	// clear the struct, while nothing to do any more

		if (!sm->mBody.FLAG.BIDI) return;													// everything fine, ACK was not required

		sm->timeout = 1;																	// set the time out only while an ACK or answer was requested
		led.set(noack);																		// fire the status led
		pom.stayAwake(100);																	// and stay awake for a short while

		DBG(SN, F("  timed out "), _TIME, '\n');											// some debug
	}

	
}

void SN::process_config_list_answer_slice(void) {
	s_snd_msg *sm = &snd_msg;																// short hand to snd_msg struct
	s_config_list_answer_slice *cl = &config_list_answer_slice;								// short hand

	if (!cl->active) return;																// nothing to send, return
	if (!cl->timer.done()) return;															// something to send but we have to wait
	if (sm->active != MSG_ACTIVE::NONE) return;												// we have something to do, but send_msg is busy

	uint8_t payload_len;

	if (cl->type == LIST_ANSWER::PEER_LIST) {
		/* process the INFO_PEER_LIST */
		payload_len = cl->peer->get_slice(cl->cur_slc, sm->buf + 11);						// get the slice and the amount of bytes
		sm->type = MSG_TYPE::INFO_PEER_LIST;												// flags are set within the snd_msg struct
		//DBG(SN, F("SN:LIST_ANSWER::PEER_LIST cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		cl->cur_slc++;																		// increase slice counter

	} else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_PAIRS) {
		/* process the INFO_PARAM_RESPONSE_PAIRS */
		if ((cl->cur_slc + 1) < cl->max_slc) {												// within message processing, get the content													
			payload_len = cl->list->get_slice_pairs(cl->peer_idx, cl->cur_slc, sm->buf + 11);// get the slice and the amount of bytes
		} else {																			// last slice, terminating message
			payload_len = 2;																// reflect it in the payload_len
			memset(sm->buf + 11, 0, payload_len);											// write terminating zeros
		}
		sm->type = MSG_TYPE::INFO_PARAM_RESPONSE_PAIRS;										// flags are set within the snd_msg struct
		//DBG(SN, F("SN:LIST_ANSWER::PARAM_RESPONSE_PAIRS cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		cl->cur_slc++;																		// increase slice counter

	} else if (cl->type == LIST_ANSWER::PARAM_RESPONSE_SEQ) {
		/* process the INFO_PARAM_RESPONSE_SEQ 
		* not implemented yet */
	}

	sm->mBody.MSG_LEN = payload_len + 10;													// set the message len accordingly
	sm->active = MSG_ACTIVE::PAIR;															// for address, counter and to make it active

	if (cl->cur_slc >= cl->max_slc) {														// if everything is send, we could stop the struct
		//DBG(SN, F("SN:LIST_ANSWER::DONE cur_slc:"), cl->cur_slc, F(", max_slc:"), cl->max_slc, F(", pay_len:"), payload_len, '\n');
		cl->active = 0;
		cl->cur_slc = 0;
	}
}


