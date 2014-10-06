//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin battery status functions ---------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define BT_DBG
#include "Battery.h"
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------

// private:		//---------------------------------------------------------------------------------------------------------
BT::BT() {
} 
void    BT::init(uint8_t mode, AS *ptrMain) {
	
	#ifdef BT_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("BT.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void	BT::poll(void) {
}
