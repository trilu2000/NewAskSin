//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin registrar functions --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include "Registrar.h"
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------
void	RG::regUserModuleInAS(uint8_t cnl, uint8_t lst, myDelegate delegate, uint8_t *mainList, uint8_t *peerList) {
	s_modTable *pModTbl = &modTbl[cnl];													// pointer to the respective line in the module table

	pModTbl->cnl = cnl;
	pModTbl->lst = lst;
	pModTbl->mDlgt = delegate;
	pModTbl->lstCnl = mainList;
	pModTbl->lstPeer = peerList;

	hm.ee.getList(cnl, 1, 0, pModTbl->lstCnl);											// load list1 in the respective buffer
	
	pModTbl->mDlgt(1,0,6,NULL,0);														// inform the module of the change
}

void	RG::poll(void) {

	for (uint8_t i = 0; i <= cnl_max; i++) {											// poll through the module table
		s_modTable *pModTbl = &modTbl[i];												// pointer to the respective line in the module table
		if (pModTbl->cnl) pModTbl->mDlgt(0,0,0,NULL,0);
	}
}


