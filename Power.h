//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin power management function --------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _PW_H
#define _PW_H

#include "HAL.h"

// - power modes to implement -----------
// 0 - no power management
// 1 - wake up every 250ms, check for wakeup signal on air and stay awake accordingly, timer gets updated every 250ms
// 2 - deep sleep, wakeup every 8 seconds, not able to receive anything while sleeping, timer gets updated every 8000ms
// 3 - deep sleep, wakeup only on interrupt

class PW {
	friend class AS;
  
  private:		//---------------------------------------------------------------------------------------------------------
	uint8_t pwrMode;
	
  protected:	//---------------------------------------------------------------------------------------------------------
  public:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

	PW();
	void init(uint8_t mode);
	void poll(void);
};

#endif 
