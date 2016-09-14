/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin send function --------------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _RV_H
#define _RV_H

#include "HAL.h"										// hardware abstraction layer
#include "message_union.h"


class RV  {
	friend class AS;

public:		//---------------------------------------------------------------------------------------------------------
	#define rcvStrLen    rcv.msg.mBody.MSG_LEN+1						// natural len of received bytes
	#define rcvHasData   rcv.buf[0]?1:0									// check if something is in the received buffer
	#define rcvAckReq    rcv.msg.mBody.FLAG.BIDI						// check if an ACK is requested

	u_Message msg;														// union for all different type of message structs
	uint8_t *buf = (uint8_t*)&msg;										// map the union to a local byte buffer

	uint8_t lastMSG_CNT;												// remember the last message counter to check if it is a repeated message

	uint8_t PEER_CNL;
	uint8_t PEER_ID[4];

	uint8_t prevBuf[27];												// store the last receive message to verify with AES signed data.
	uint8_t prevBufUsed;												// flag to store prevBuf is used

	RV();																// constructor
	void    poll(void);													// poll function
	uint8_t getIntend(void);											// checks the received string if addresses are known
};
extern RV rcv;															// declaration of receive class in AS.cpp

#endif 
