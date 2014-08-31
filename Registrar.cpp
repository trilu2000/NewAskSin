//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin registrar functions --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define RG_DBG
#include "Registrar.h"

// public:		//---------------------------------------------------------------------------------------------------------
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
}
