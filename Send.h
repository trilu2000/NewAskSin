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

	/*
	* @brief Helper struct for all send function relevant variables
	* structs holds following information:
	* active       - is send module active, 1 indicates yes
	* timeout      - was last message a timeout
	* prev_msg_cnt - store for message counter, needed to identify ACK
	* retr_cnt     - variable to count how often a message was already send
	* max_retr     - how often a message has to be send until ACK
	* timer        - config mode timeout
	*/
	s_send_flags flag;	
	uint8_t      msg_cnt;													// message counter for initial sends
	uint8_t      prev_msg_cnt;												// store for message counter, needed to identify ACK

	SN();																	// constructor
	void poll(void);														// poll function, process if something is to send
	void cleanUp(void);														// clenup for the send function


};

extern SN snd;																// send class is declared in AS.cpp

#endif 
