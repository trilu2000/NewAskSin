//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin registrar functions --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------


#ifndef _RG_H
#define _RG_H

#include "HAL.h"


class RG {
  public:		//---------------------------------------------------------------------------------------------------------
	struct s_modTable {
		uint8_t cnl;																		// channel where the module is registered to
		uint8_t stat;																		// status of the module, important for actuators
		uint8_t cng;																		// is module in change?, actuators only
		uint8_t msgCnt;																		// channel message counter
	};

  protected:	//---------------------------------------------------------------------------------------------------------
	class AS *pHM;							// pointer to main class for function calls

  private:		//---------------------------------------------------------------------------------------------------------


  public:		//---------------------------------------------------------------------------------------------------------
	RG();
	void    init(AS *ptrMain);
	void    poll(void);

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------


};
extern RG::s_modTable modTbl[];																// initial register.h

#endif
