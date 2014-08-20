//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin RF send structures ---------------------------------------------------------------------------------------------
//- 
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _SN_H
#define _SN_H

#include "HAL.h"

	#define MaxDataLen   60						// maximum length of received bytes

	/*struct s_mFlg {
		uint8_t WKUP     :1;					// 0x01: send initially to keep the device awake
		uint8_t WKMEUP   :1;					// 0x02: awake - hurry up to send messages
		uint8_t CFG      :1;					// 0x04: Device in Config mode
		uint8_t	         :1;
		uint8_t Burst    :1;					// 0x10: set if burst is required by device
		uint8_t BIDI     :1;					// 0x20: response is expected
		uint8_t RPTED    :1;					// 0x40: repeated (repeater operation)
		uint8_t RPTEN    :1;					// 0x80: set in every message. Meaning?
	};
	
	struct s_msgBody {
		uint8_t       mLen;
		uint8_t       rCnt;
		struct s_mFlg mFlg;
		uint8_t       mTyp;
		uint8_t       reID[3];
		uint8_t       toID[3];
		uint8_t       by10;
		uint8_t       by11;
		uint8_t       pyLd[MaxDataLen-12];
	};*/

	#define sndHasData   sndBuf[0]?1:0			// check if something is in the buffer
	#define sndLen       sndBuf[0]+1
	#define reqACK       snd.mFlg.BIDI			// check if an ACK is requested


#endif 
