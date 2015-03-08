//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin hardware definition ----------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -------------------------------------------------------------------------------------------------------------------

#include "hardware.h"
#include <HAL_extern.h>

void    initWakeupPin(void) {
	#if defined(wakeupDDR)
	pinInput(wakeupDDR, wakeupPIN);											// set pin as input
	setPinHigh(wakeupPRT, wakeupPIN);										// enable internal pull up
	#endif
}
uint8_t checkWakeupPin(void) {
	// to enable the USB port for upload, configure PE2 as input and check if it is 0, this will avoid sleep mode and enable program upload via serial
	#if defined(wakeupDDR)
	if (getPin(wakeupPNR, wakeupPIN)) return 1;								// return pin is active
	#endif

	return 0;																// normal operation
}
//- -----------------------------------------------------------------------------------------------------------------------
