//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _AS_H
#define _AS_H

#include "HAL.h"
#include "EE.h"



//struct s_devPara {
//	uint8_t  maxRetr;																	// max send retries
//	uint16_t timeOut;																	// timeout for ACK sending
//	const uint8_t  *p;																	// pointer to PROGMEM serial number, etc
//	uint8_t  MAID[3];																	// master id for further use
//	uint8_t  HMID[3];																	// own HMID
//};


class AS {
  public:		//---------------------------------------------------------------------------------------------------------
	EE ee;
  protected:
  private:

  //functions
  public:
	AS();
	void init(void);
		
  protected:
  private:

};
extern AS hm;


#endif

