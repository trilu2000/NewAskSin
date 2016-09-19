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


RV rcv;																						// declare receive module, defined in receive.h


RV::RV() {
	DBG( F("RV.\n") );																		// ...and some information
}

void RV::poll(void) {

	// checks a received string for validity and intent
	if (msg.mBody.MSG_LEN < 9) {															// check if the string has all mandatory bytes, if not
		DBG( F("  too short...\n") );
		msg.mBody.MSG_LEN = 0;															// clear receive buffer
		return;
	}

	// check for a repeated string which was already processed
	if ((msg.mBody.FLAG.RPTED) && (lastMSG_CNT == msg.mBody.MSG_CNT)) {				// check if message was already received
		DBG( F("  repeated ...\n") );
		msg.mBody.MSG_LEN = 0;															// clear receive buffer
		return;
	}
	lastMSG_CNT = msg.mBody.MSG_CNT;													// remember for next time

	// get the intend of the message
	uint8_t bIntend = getIntend();															// no params neccassary while everything is in the rcv union

	DBG( (char)bIntend, F("> "), _HEX( buf, rcvStrLen), ' ', _TIME, '\n' );

	#ifdef RV_DBG_EX																		// only if extended AS debug is set
	hm.explainMessage(this->buf);
	#endif

	// process only messages from master, peer or internal
	if ((bIntend != 'm') && (bIntend != 'p') && (bIntend != 'i') && (bIntend != 'x')) {
		msg.mBody.MSG_LEN = 0;															// clear receive buffer
		PEER_CNL = 0;
		return;																				// nothing to do any more
	}

	hm.processMessage();																	// process the message which is in the received buffer
	msg.mBody.MSG_LEN = 0;																// everything done, set length byte to 0
	PEER_CNL = 0;
}

uint8_t RV::getIntend() {

	// it could come as a broadcast message - report it only while loging is enabled
	if ( isEmpty(msg.mBody.RCV_ID, 3)) return 'b';										// broadcast message

	// it could be addressed to a different device - report it only while loging is enabled
	// memcmp gives 0 if string matches, any other value while no match
	if ( !isEqual(msg.mBody.RCV_ID, HMID, 3) ) return 'l';								// not for us, only show as log message

	// because of the previous check, message is for us, check against master
	if ( isEqual(msg.mBody.SND_ID, MAID, 3) ) return 'm';									// coming from master

	// message is for us, but not from master, maybe it is a peer message
	memcpy( PEER_ID, msg.mBody.SND_ID, 3 );											// peer has 4 byte and the last byte indicates the channel but also lowbat and long message, therefore we copy it together in a seperate byte array
	uint8_t buf10 = buf[10];															// get the channel byte seperate
	if (prevBufUsed) buf10 = prevBuf[10];													// if AES is active, we must get buf[10] from prevBuf[10]
	PEER_ID[3] = buf10 & 0x3f;															// mask out long and battery low
	PEER_CNL = ee_peer.isPeerValid(PEER_ID);
	if (PEER_CNL ) return 'p';															// check if it is coming from a peer

	// message is for us, but not from pair or peer, check if we were the sender and flag it as internal
	if ( isEqual(msg.mBody.SND_ID, HMID, 3) ) return 'i';									// we were the sender, internal message

	// message is for us, but not from pair or peer or internal - check if we miss the master id because we are not paired
	if ( isEmpty(MAID, 3)) return 'x';
	return 'u';																				// should never happens
}

