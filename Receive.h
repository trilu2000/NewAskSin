//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin receive function -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------


#ifndef _RV_H
#define _RV_H

#include "HAL.h"
#define MaxDataLen   60						// maximum length of received bytes

class RV {
	friend class AS;
  
  protected:	//---------------------------------------------------------------------------------------------------------
	struct s_mFlg {
	  uint8_t WKUP     :1;					// 0x01: send initially to keep the device awake
	  uint8_t WKMEUP   :1;					// 0x02: awake - hurry up to send messages
	  uint8_t CFG      :1;					// 0x04: Device in Config mode
	  uint8_t	       :1;
	  uint8_t BURST    :1;					// 0x10: set if burst is required by device
	  uint8_t BIDI     :1;					// 0x20: response is expected
	  uint8_t RPTED    :1;					// 0x40: repeated (repeater operation)
	  uint8_t RPTEN    :1;					// 0x80: set in every message. Meaning?
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

  public:		//---------------------------------------------------------------------------------------------------------
	struct s_msgBody mBdy;					// structure for easier message creation
	uint8_t peerId[4];						// hold for messages >= 3E the peerID with channel
	uint8_t *buf;							// cast to byte array

	#define hasData		buf[0]?1:0			// check if something is in the buffer

  private:		//---------------------------------------------------------------------------------------------------------
	#define bufLen      buf[0]+1
	#define ackRq       mBdy.mFlg.BIDI		// check if an ACK is requested

	class AS *pHM;							// pointer to main class for function calls

  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	RV();
	void    init(AS *ptrMain);
	void    poll(void);

};

#endif 
