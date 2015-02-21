//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin battery status functions ---------------------------------------------------------------------------------------
//- with a lot of support from dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _BT_H
#define _BT_H

#include "HAL.h"


class BT {
	friend class AS;

  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	
	class AS *pHM;								// pointer to main class for function calls

	uint8_t  checkTenthVolt;					// holds the proof point
	uint8_t  measureTenthVolt;					// variable to hold last measured value
	uint8_t  bState        :1;					// holds status bit
	uint8_t  bMode         :2;					// mode variable
	uint32_t bDuration;							// duration for the next check
	
  public:		//---------------------------------------------------------------------------------------------------------
	BT();
	void set(uint8_t tenthVolt, uint32_t duration);
		
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	void    init(AS *ptrMain);
	void    poll(void);
	uint8_t getStatus(void);
};


#endif
