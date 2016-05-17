//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define LD_DBG
#include "StatusLed.h"
#include "AS.h"

waitTimer ledTmr;																				// config timer functionality

// public:		//---------------------------------------------------------------------------------------------------------

// private:		//---------------------------------------------------------------------------------------------------------
LD::LD() {} 

/**
* @brief Init function for the led class. Normally called directly from AskSin class.
* In this case the init is only the handover of a pointer to the main class, some debug message if enabled 
* and the trigger for the hardware initialization.
* @param   *ptrMain   The pointer to the main class 
*/
void    LD::init(AS *ptrMain) {
	
	#ifdef LD_DBG																				// only if ee debug is set
	dbgStart();																					// serial setup
	dbg << F("LD.\n");																			// ...and some information
	#endif

	pHM = ptrMain;																				// store the pointer to the main class
	initLeds();																					// init the hardware, defined in hardware.h
}

/**
* @brief Here we are choosing and start the blink pattern. Processing is done via the poll function within 
* this class. Blinkpattern defined in HAL.H and declaration is done in HAL_extern.h
* @param   stat    pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long, nothing
*/
void    LD::set(ledStat stat) {
	#ifdef LD_DBG
	dbg << "stat: " << stat << '\n';
	#endif

	ledRed(0);																					// new program starts, so switch leds off
	ledGrn(0);

	blinkPtr = &blPat[stat];																	// set the pattern within the pointer, processing is done in the poll function
	active = 1;																					// make module active
	lCnt = 0;																					// set start position
	dCnt = 1;

	if (blinkPtr->len == 0) stat = nothing;														// some sanity on blink pointer
	
	if (stat == nothing) {
		ledTmr.set(0);																			// timer done
		active = 0;																				// nothing to do any more
	}
}

/**
* @brief By calling this function there is only a short red blink on the LED. Independent from
* pattern processing within the poll function.
*/
void    LD::blinkRed(void) {
	ledRed(0);																					// switch led off
	_delay_ms(20);																				// wait
	ledRed(1);																					// switch led on
	_delay_ms(20);																				// wait
	ledRed(0);																					// switch led off
}

/**
* @brief Poll function has to be called regulary to process blink patterns. Done by the AS main class
* by default.
*/
void	LD::poll(void) {
	if (!active) return;																		// still waiting to do something
	if (!ledTmr.done()) return;																	// active but timer not done
	
	// if we are here we have something to do, set the led, timer and counter
	ledTmr.set(blinkPtr->pat[lCnt]*10);															// set the timer for next check up

	if ((blinkPtr->led0) && (blinkPtr->pat[lCnt])) {
		ledRed((lCnt % 2)^1);																	// set the led
		#ifdef LD_DBG
		dbg << "lCnt:" << lCnt << " led0: " << ((lCnt % 2)^1) << '\n';
		#endif
	}
	
	if ((blinkPtr->led1) && (blinkPtr->pat[lCnt])) {
		ledGrn((lCnt % 2)^1);
		#ifdef LD_DBG
		dbg << "lCnt:" << lCnt  << " led1: " << ((lCnt % 2)^1) << '\n';
		#endif
	}
	lCnt++;																						// increase the pointer for the blink string

	// check if position pointer runs out of string
	if (lCnt >= blinkPtr->len) {																// we are through the pattern 
		if (blinkPtr->dur == 0) {																// we are in an endless loop
			lCnt = 0;
			#ifdef LD_DBG
			dbg << "lCnt 0\n";
			#endif

		} else if (dCnt < blinkPtr->dur) {														// duration is not done
			lCnt = 0;
			dCnt++;
			#ifdef LD_DBG
			dbg << "lCnt 0, dCnt++\n";
			#endif

		} else {
			set(nothing);

		}
	}
}
