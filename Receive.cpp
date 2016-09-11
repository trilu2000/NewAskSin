/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin receive function -----------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef RV_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif


#include "Receive.h"
#include "AS.h"

uint8_t last_MSG_CNT;																		// remember the last message counter to check if it is a repeated message

RV::RV() {
	DBG( F("RV.\n") );																		// ...and some information
}

void RV::poll(void) {

	if (!rcv_hasData) return;																// something to do?

	// checks a received string for validity and intent
	if (rcv.msgBody.MSG_LEN < 9) {															// check if the string has all mandatory bytes, if not
		DBG( F("  too short...\n") );
		rcv.msgBody.MSG_LEN = 0;															// clear receive buffer
		return;
	}

	// check for a repeated string which was already processed
	if ((rcv.msgBody.FLAG.RPTED) && (last_MSG_CNT == rcv.msgBody.MSG_CNT)) {				// check if message was already received
		DBG( F("  repeated ...\n") );
		rcv.msgBody.MSG_LEN = 0;															// clear receive buffer
		return;
	}
	last_MSG_CNT = rcv.msgBody.MSG_CNT;														// remember for next time

	// get the intend of the message
	uint8_t bIntend = getIntend();															// no params neccassary while everything is in the rcv union

	DBG( (char)bIntend, F("> "), _HEX(rcv.buf, rcv_bufLen), ' ', _TIME, '\n' );

	#ifdef RV_DBG_EX																		// only if extended AS debug is set
	hm.explainMessage(this->buf);
	#endif

	// process only messages from master, peer or internal
	if ((bIntend != 'm') && (bIntend != 'p') && (bIntend != 'i') && (bIntend != 'x')) {
		rcv.msgBody.MSG_LEN = 0;															// clear receive buffer
		rcv_PEER_CNL = 0;
		return;																				// nothing to do any more
	}

	hm.processMessage();																	// process the message which is in the received buffer
	rcv.msgBody.MSG_LEN = 0;																// everything done, set length byte to 0
	rcv_PEER_CNL = 0;
}

uint8_t RV::getIntend() {

	// it could come as a broadcast message - report it only while loging is enabled
	if ( isEmpty(rcv.msgBody.RCV_ID, 3)) return 'b';										// broadcast message

	// it could be addressed to a different device - report it only while loging is enabled
	// memcmp gives 0 if string matches, any other value while no match
	if ( !isEqual(rcv.msgBody.RCV_ID, HMID, 3) ) return 'l';								// not for us, only show as log message

	// because of the previous check, message is for us, check against master
	if ( isEqual(rcv.msgBody.SND_ID, MAID, 3) ) return 'm';									// coming from master

	// message is for us, but not from master, maybe it is a peer message
	memcpy( rcv_PEER_ID, rcv.msgBody.SND_ID, 3 );											// peer has 4 byte and the last byte indicates the channel but also lowbat and long message, therefore we copy it together in a seperate byte array
	uint8_t buf10 = rcv.buf[10];															// get the channel byte seperate
	if (prevBufUsed) buf10 = prevBuf[10];													// if AES is active, we must get buf[10] from prevBuf[10]
	rcv_PEER_ID[3] = buf10 & 0x3f;															// mask out long and battery low
	rcv_PEER_CNL = hm.ee.isPeerValid(rcv_PEER_ID);
	if (rcv_PEER_CNL ) return 'p';															// check if it is coming from a peer

	// message is for us, but not from pair or peer, check if we were the sender and flag it as internal
	if ( isEqual(rcv.msgBody.SND_ID, HMID, 3) ) return 'i';									// we were the sender, internal message

	// message is for us, but not from pair or peer or internal - check if we miss the master id because we are not paired
	if ( isEmpty(MAID, 3)) return 'x';
	return 'u';																				// should never happens
}

