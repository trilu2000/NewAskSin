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
	#if defined(WAKE_UP_DDR)
		//pinInput(WAKE_UP_DDR, WAKE_UP_PIN);											// set pin as input
		//setPinHigh(WAKE_UP_PORT, WAKE_UP_PIN);										// enable internal pull up
	#endif
}
uint8_t checkWakeupPin(void) {
	// to enable the USB port for upload, configure PE2 as input and check if it is 0, this will avoid sleep mode and enable program upload via serial
	#if defined(WAKE_UP_DDR)
		//if (getPin(WAKE_UP_PNR, WAKE_UP_PIN)) return 1;								// return pin is active
	#endif

	return 0;																		// normal operation
}
