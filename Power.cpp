//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin power management function --------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define PW_DBG
#include "Power.h"
#include "AS.h"

// private:		//---------------------------------------------------------------------------------------------------------
waitTimer pwrTmr;																			// power timer functionality

PW::PW() {
} 
void PW::init(void) {
	#ifdef PW_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("PW.\n");																		// ...and some information
	#endif

}
void PW::poll(void) {
}
