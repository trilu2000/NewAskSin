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
#include "Fastdelegate.h"

//- typedef for delegate to module function
using namespace fastdelegate;
typedef FastDelegate5<uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t> s_mod_dlgt;				// void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len)

class RG {
	friend class AS;
  
  public:		//---------------------------------------------------------------------------------------------------------
	struct s_modTable {
		uint8_t cnl;																		// channel where the module is registered to
		uint8_t lst;																		// module has a list3 or list 4
		uint8_t msgCnt;																		// channel message counter
		uint8_t *lstCnl;																	// pointer to list0/1
		uint8_t *lstPeer;																	// pointer to list3/4
		s_mod_dlgt mDlgt;																	// delegate to the module function
	};

  protected:	//---------------------------------------------------------------------------------------------------------

  private:		//---------------------------------------------------------------------------------------------------------
	class AS *pHM;							// pointer to main class for function calls

  public:		//---------------------------------------------------------------------------------------------------------
	void regInAS(uint8_t cnl, uint8_t lst, s_mod_dlgt delegate, uint8_t *mainList, uint8_t *peerList);

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	RG();
	void init(AS *ptrMain);
	void poll(void);

};
extern RG::s_modTable modTbl[];																// initial register.h

#endif
