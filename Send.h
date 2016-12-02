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



class SN  {
	#define sndStrLen	 snd_msg.mBody->MSG_LEN+1							// amount of bytes in the send buffer
	#define sndAckReq    snd_msg.mBody->FLAG.BIDI							// check if an ACK is requested

	struct s_0x01 {
		uint8_t PEER_NEEDS_BURST : 1;  // 0x01.0, s:1   d: false  
		uint8_t                  : 6;  // 0x01.1, s:6   d:   
		uint8_t EXPECT_AES       : 1;  // 0x01.7, s:1   d: false  
	}; // 1 byte

public:		//---------------------------------------------------------------------------------------------------------

	SN() {}																	// constructor
	void poll(void);														// poll function, process if something is to send

	inline void process_config_list_answer_slice(void);						// slice wise send function for list requests
	inline void prepare_message(void);										// prepare the message to send with the information in snd_msg struct

};

extern s_config_list_answer_slice config_list_answer_slice;

/*
* @brief Struct to hold the buffer for any string to send with some flags for further processing
*/
extern s_send snd_msg;

extern SN snd;																// send class is declared in AS.cpp

#endif 
