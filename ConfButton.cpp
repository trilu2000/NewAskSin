//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin config key function --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define CB_DBG
#include "ConfButton.h"
#include "AS.h"

// scenario 01
// short press = nothing
// long press  = pair
// double long = reset
//
// scenario 02
// short press = toggle channel one
// long press  = pair
// double long = reset
//
// if 0x18 localResDis available, take care of it and enable or disable double long


// public:		//---------------------------------------------------------------------------------------------------------
CB::CB() {
} 

void CB::init(AS *ptrMain) {
	#ifdef CB_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("CB.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void CB::poll(void) {

}
