/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin receive function -----------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "Receive.h"
#include "AS.h"


RV rcv;																						// declare receive module, defined in receive.h


RV::RV() {
	DBG(RV, F("RV.\n") );																	// ...and some information
}

void RV::poll(void) {

	// checks a received string for validity and intent
	if (rcv_msg.mBody.MSG_LEN < 9) {														// check if the string has all mandatory bytes, if not
		DBG(RV, F("  too short...\n"));
		rcv_msg.clear();																	// clear receive buffer
		return;
	}

	// check for a repeated string which was already processed
	if ((rcv_msg.mBody.FLAG.RPTED) && (rcv_msg.prev_MSG_CNT == rcv_msg.mBody.MSG_CNT)) {	// check if message was already received
		DBG(RV, F("  repeated ...\n"));
		rcv_msg.clear();																	// clear receive buffer
		return;
	}
	rcv_msg.prev_MSG_CNT = rcv_msg.mBody.MSG_CNT;											// remember for next time

	// get the intend of the message
	getIntend();																			// no params neccassary while everything is in the recv struct

	// filter out the footprint of MAX! devices
	// b> 0F 04 86 10 38 EB 06 00 00 00 0A 24 B8 0C 00 40  (1963077)
	if ((rcv_msg.mBody.MSG_LEN == 0x0f) && (*(uint8_t*)&rcv_msg.mBody.FLAG == 0x86) && (rcv_msg.intent == MSG_INTENT::BROADCAST)) {
		rcv_msg.clear();
		return;
	}

	DBG(RV, (char)rcv_msg.intent, F("> "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), ' ', _TIME, '\n');

	// process only messages from master, peer or internal
	if ((rcv_msg.intent != MSG_INTENT::MASTER) && (rcv_msg.intent != MSG_INTENT::PEER) && (rcv_msg.intent != MSG_INTENT::INTERN) && (rcv_msg.intent != MSG_INTENT::NOT_PAIRED)) {
		rcv_msg.clear();																	// nothing to do any more
		return;
	}

	rcv_msg.hasdata = 1;																	// signalize that something is to do
}


void RV::getIntend() {
	// prepare the peer search
	memcpy(rcv_msg.peer, rcv_msg.mBody.SND_ID, 3);											// peer has 4 byte and the last byte indicates the channel but also lowbat and long message, therefore we copy it together in a seperate byte array
	uint8_t buf10 = rcv_msg.buf[10];														// get the channel byte seperate
	if (rcv_msg.use_prev_buf) buf10 = rcv_msg.prev_buf[10];									// if AES is active, we must get buf[10] from prevBuf[10]
	rcv_msg.peer[3] = buf10 & 0x3f;															// mask out long and battery low


	// it could come as a broadcast message - report it only while loging is enabled
	if (isEmpty(rcv_msg.mBody.RCV_ID, 3))													// broadcast message
		rcv_msg.intent = MSG_INTENT::BROADCAST;			

	// it could be addressed to a different device - report it only while loging is enabled
	// memcmp gives 0 if string matches, any other value while no match
	else if ( !isEqual(rcv_msg.mBody.RCV_ID, dev_ident.HMID, 3) ) 							// not for us, only show as log message
		rcv_msg.intent = MSG_INTENT::LOGGING;

	// because of the previous check, message is for us, check against master
	else if ( isEqual(rcv_msg.mBody.SND_ID, MAID, 3) )										// coming from master
		rcv_msg.intent = MSG_INTENT::MASTER;

	// message is for us, but not from master, maybe it is a peer message
	else if (rcv_msg.cnl = is_peer_valid(rcv_msg.peer))										// check if the peer is known and remember the channel
		rcv_msg.intent = MSG_INTENT::PEER;

	// message is for us, but not from pair or peer, check if we were the sender and flag it as internal
	else if ( isEqual(rcv_msg.mBody.SND_ID, dev_ident.HMID, 3) )							// we were the sender, internal message
		rcv_msg.intent = MSG_INTENT::INTERN;

	// message is for us, but not from pair or peer or internal - check if we miss the master id because we are not paired
	else if ( isEmpty(MAID, 3))																// for us, but pair is empty
		rcv_msg.intent = MSG_INTENT::NOT_PAIRED;

	else																					// should never happens
		rcv_msg.intent = MSG_INTENT::ERROR;
}

