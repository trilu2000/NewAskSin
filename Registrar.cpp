//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin registrar functions --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define RG_DBG
#include "Registrar.h"
#include "AS.h"

// public:		//---------------------------------------------------------------------------------------------------------
void	RG::regInAS(uint8_t cnl, uint8_t lst, s_mod_dlgt delegate, uint8_t *mainList, uint8_t *peerList) {
	modTbl[cnl-1].cnl = cnl;
	modTbl[cnl-1].lst = lst;
	modTbl[cnl-1].mDlgt = delegate;
	modTbl[cnl-1].lstCnl = mainList;
	modTbl[cnl-1].lstPeer = peerList;

	pHM->ee.getList(cnl,1,0,modTbl[cnl-1].lstCnl);											// load list1 in the respective buffer
	modTbl[cnl-1].mDlgt(0x01, 0, 0x06, NULL, 0);											// inform the module of the change
}

// private:		//---------------------------------------------------------------------------------------------------------
RG::RG() {
} 
void    RG::init(AS *ptrMain) {
	
	#ifdef RG_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("RG.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void	RG::poll(void) {
	// poll through the module table
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {
		if (modTbl[i].cnl) modTbl[i].mDlgt(0,0,0,NULL,0);
	}
}
