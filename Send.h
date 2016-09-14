/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin send function --------------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _SN_H
#define _SN_H


#include "HAL.h"
#include "message_union.h"


class SN {
	#define sndStrLen	 snd.msg.mBody.MSG_LEN+1							// amount of bytes in the send buffer
	#define sndAckReq    snd.msg.mBody.FLAG.BIDI							// check if an ACK is requested

public:		//---------------------------------------------------------------------------------------------------------
	u_Message msg;															// union for all different type of message structs
	uint8_t   *buf = (uint8_t*)&msg;										// map the union to a local byte buffer
	uint8_t   *prev_buf = new uint8_t[32];									// store the last sent message for calculating AES signing response, bytes 1 - 27 of the original message needed

	uint8_t   MSG_CNT;														// message counter for standard sends, while not answering something
	uint8_t   prevMSG_CNT;													// store for message counter, needed to identify ACK

	uint8_t   retrCnt;														// variable to count how often a message was already send
	uint8_t   maxRetr;														// how often a message has to be send until ACK


	uint8_t   active;														// is send module active, 1 indicates yes
	uint8_t   timeOut;														// was last message a timeout

	SN();																	// constructor
	void poll(void);														// poll function, process if something is to send
	void cleanUp(void);														// todo
};

extern SN snd;																// send class is declared in AS.cpp

#endif 
