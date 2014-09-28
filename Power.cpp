//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin power management function --------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define PW_DBG
#include "Power.h"
#include "AS.h"

// private:		//---------------------------------------------------------------------------------------------------------
waitTimer pwrTmr;																			// power timer functionality

PW::PW() {
} 
void PW::init(AS *ptrMain) {
	#ifdef PW_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("PW.\n");																		// ...and some information
	#endif

	pHM = ptrMain;																			// pointer to main class
	pwrMode = 0;																			// set default
}
void PW::setMode(uint8_t mode) {
	pwrMode = mode;
	initPwrMode(pwrMode);

	#ifdef PW_DBG																			// only if pw debug is set
	dbg << F("PowerMode: ") << pwrMode << '\n';												// ...and some information
	#endif

	stayAwake(2000);																		// startup means stay awake for next 20 seconds
}
void PW::stayAwake(uint16_t time) {
	if (time < pwrTmr.remain()) return;														// set new timeout only if we have to add something
	pwrTmr.set(time);
}
void PW::poll(void) {
	if (pwrMode == 0) return;
	if (!pwrTmr.done()) return;	

	#ifdef PW_DBG																			// only if pw debug is set
	dbg << '.';																				// ...and some information
	_delay_ms(10);
	#endif

	pHM->cc.setIdle();																		// set communication module to idle
	pHM->ld.set(nothing);																	// switch off all led's
	pHM->ld.blinkRed();																		// we go sleeping
	
	setSleep();																				// call sleep function in HAL

	// wake up from sleep, if we are in pwrMode 2 we have to check for a burst signal
	// if there is one, we set the pwrTmr to stay awake for some time, otherwise we fall in sleep again
	

	#ifdef PW_DBG																			// only if pw debug is set
	dbg << ':' << getMillis() << '\n';														// ...and some information
	#endif
		
}
