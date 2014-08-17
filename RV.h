//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin RF receiver functions ------------------------------------------------------------------------------------------
//- 
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _RV_H
#define _RV_H

#include "HAL.h"

class RV {
  public:		//---------------------------------------------------------------------------------------------------------
	#define MaxDataLen          60																// maximum length of received bytes
	uint8_t buf[MaxDataLen];																	// buffer for received string
	uint8_t *HMID;																				// pointer to HMID, 3 byte
	uint8_t *MAID;																				// pointer to Master ID, 3 byte
	
	enum msgTypes {info,pair,peer,bcast};
	uint8_t forUs;
	
	#define hasData             buf[0]?1:0														// check if something is in the buffer
	#define reID                buf+4															// message comes from
	#define toID                buf+7															// addressed to us
	#define len                 buf[0]+1
	#define msgTyp              buf[3]
	#define by10                buf[10]
	#define by11                buf[11]
	#define reCnt				buf[1]
	
	#define ackRq				buf[2] & (1<<5)													// check if an ACK is requested
		
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	RV();
	void    decode(void);																		// decodes the message  
	
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

};

#endif 
