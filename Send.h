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
#define MaxDataLen   60						// maximum length of received bytes


class SN {
	friend class AS;
  
  private:		//---------------------------------------------------------------------------------------------------------

	struct s_mFlg {
		uint8_t WKUP     :1;				// 0x01: send initially to keep the device awake
		uint8_t WKMEUP   :1;				// 0x02: awake - hurry up to send messages
		uint8_t CFG      :1;				// 0x04: Device in Config mode
		uint8_t	         :1;
		uint8_t BURST    :1;				// 0x10: set if burst is required by device
		uint8_t BIDI     :1;				// 0x20: response is expected
		uint8_t RPTED    :1;				// 0x40: repeated (repeater operation)
		uint8_t RPTEN    :1;				// 0x80: set in every message. Meaning?
	};

	struct s_msgBody {
		uint8_t       mLen;					// message length
		uint8_t       mCnt;					// counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
		struct s_mFlg mFlg;					// see structure of message flags
		uint8_t       mTyp;					// type of message
		uint8_t       reID[3];				// sender ID
		uint8_t       toID[3];				// receiver id, broadcast for 0
		uint8_t       by10;					// type of message
		uint8_t       by11;					// type of message
		uint8_t       pyLd[MaxDataLen-12];	// payload
	};


	uint8_t retrCnt;						// variable to count how often a message was already send
	uint8_t maxRetr;						// how often a message has to be send until ACK
	uint8_t lastMsgCnt;						// store of message counter, needed to identify ACK

	class AS *pHM;							// pointer to main class for function calls

  protected:	//---------------------------------------------------------------------------------------------------------
  public:		//---------------------------------------------------------------------------------------------------------
	struct s_msgBody mBdy;					// structure for easier message creation
	uint8_t *buf;							// cast to byte array

	uint8_t msgCnt;							// message counter for standard sends, while not answering something

	uint8_t active   :1;					// is send module active, 1 indicates yes
	uint8_t timeOut  :1;					// was last message a timeout

  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

	SN();
	void init(AS *ptrMain);
	void poll(void);
};

#endif 
