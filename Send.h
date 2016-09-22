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
#include "AS_typedefs.h"


class SN {
	#define sndStrLen	 snd.msg.mBody.MSG_LEN+1							// amount of bytes in the send buffer
	#define sndAckReq    snd.msg.mBody.FLAG.BIDI							// check if an ACK is requested

public:		//---------------------------------------------------------------------------------------------------------

	u_Message msg;															// union for all different type of message structs
	uint8_t   *buf = (uint8_t*)&msg;										// map the union to a local byte buffer
	uint8_t   *prev_buf = new uint8_t[32];									// store the last sent message for calculating AES signing response, bytes 1 - 27 of the original message needed

	s_send_flags flag;
	uint8_t      msg_cnt;													// message counter for initial sends
	uint8_t      prev_msg_cnt;												// store for message counter, needed to identify ACK

	SN();																	// constructor
	void poll(void);														// poll function, process if something is to send
	void cleanUp(void);														// clenup for the send function

	void prep_nonpeer_msg(MSG_REASON::E reason, MSG_INTENT::E intent, MSG_TYPE::E type, uint8_t len, uint8_t max_retr);

};

extern SN snd;																// send class is declared in AS.cpp

#endif 
