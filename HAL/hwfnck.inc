#include "../../hardware.h"

#ifndef _TEST_c
	#define _TEST_c

//- pin related functions -------------------------------------------------------------------------------------------------
void    initConfKey(void) {
	// set port pin and register pin interrupt
	pinInput(confKeyDDR, confKeyPin);														// init the config key pin
	setPinHigh(confKeyPort,confKeyPin);

	initPCINT();																			// some sanity on interrupts
	regPCIE(confKeyPCIE);																	// set the pin change interrupt
	regPCINT(confKeyPCMSK,confKeyINT);														// description is in hal.h
}

#endif
