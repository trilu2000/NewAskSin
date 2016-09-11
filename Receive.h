/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin receive function -----------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _RV_H
#define _RV_H

#include "HAL.h"									// hardware abstraction layer
#include "message_union.h"							// definition of receive buffer union

class RV  {
	friend class AS;

	public:		//---------------------------------------------------------------------------------------------------------
		uint8_t prevBuf[27];						// store the last receive message to verify with AES signed data.
		uint8_t prevBufUsed;						// flag to store prevBuf is used

		RV();										// constructor
		void    poll(void);							// poll function
		uint8_t getIntend(void);					// checks the received string if addresses are known
};
extern uint8_t last_MSG_CNT;						// remember the last message counter to check if it is a repeated message


#endif 
