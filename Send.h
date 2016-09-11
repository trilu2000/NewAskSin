//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin send function --------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _SN_H
#define _SN_H

#include "HAL.h"
#include "message_union.h"							// definition of receive buffer union


class SN {
	friend class AS;
  
  private:		//---------------------------------------------------------------------------------------------------------

	uint8_t retrCnt;						// variable to count how often a message was already send
	uint8_t maxRetr;						// how often a message has to be send until ACK
	uint8_t lastMsgCnt;						// store of message counter, needed to identify ACK


  protected:	//---------------------------------------------------------------------------------------------------------
  public:		//---------------------------------------------------------------------------------------------------------

	uint8_t msgCnt;							// message counter for standard sends, while not answering something

	uint8_t active;							// is send module active, 1 indicates yes
	uint8_t timeOut;						// was last message a timeout
	uint8_t msgToSign[32];					// store the last sent message for calculating AES signing response. Maximal we need the bytes 1 - 27 of the original message

  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

	SN();
	void poll(void);
	void cleanUp(void);
};

#endif 
