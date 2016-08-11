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
	uint8_t mod_cnl = cnl - 1;
	modTbl[mod_cnl].cnl = cnl;
	modTbl[mod_cnl].lst = lst;
	modTbl[mod_cnl].mDlgt = delegate;
	modTbl[mod_cnl].lstCnl = mainList;
	modTbl[mod_cnl].lstPeer = peerList;

	hm.ee.getList(cnl, 1, 0, mainList);														// load list1 in the respective buffer
	
	//hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len)
	modTbl[mod_cnl].mDlgt(0x01, 0, 0x06, NULL, 0);											// inform the module of the change
}

void	RG::poll(void) {
	// poll through the module table
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {
		if (modTbl[i].cnl) modTbl[i].mDlgt(0,0,0,NULL,0);
	}
}


