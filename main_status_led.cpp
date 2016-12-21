//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#include "00_debug-flag.h"
#include "main_status_led.h"
#include "wait_timer.h"



																								
LED::LED(uint8_t number_leds) {
	if (number_leds == 0) ptr_pat = NULL;
	else if (number_leds == 1) ptr_pat = led1_pat;
	else if (number_leds == 2) ptr_pat = led2_pat;
}

/**
* @brief Here we are choosing and start the blink pattern. Processing is done via the poll function within 
* this class. Blinkpattern defined in HAL.H and declaration is done in HAL_extern.h
* @param   stat    pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long, nothing
*/
void LED::set(ledStat stat) {
	if (!ptr_pat) return;																		// seems we do not have connected any leds
	
	#ifdef LD_DBG
	dbg << "stat: " << stat << '\n';
	#endif

	ledRed(0);																					// new program starts, so switch leds off
	ledGrn(0);

	if (stat == nothing) {																		// nothing, switch inactive
		active = 0;
	} else {																					// otherwise copy pattern from progmem and set some counter
		memcpy_P(&blPtr, &ptr_pat[stat], 10);													// cpoy the pattern from progmem
		lCnt = 0;																				// set start position
		dCnt = 1;
		active = 1;																				// make module active
	}

	if (blPtr.len == 0) stat = nothing;															// some sanity on blink pointer
}

/**
* @brief By calling this function there is only a short red blink on the LED. Independent from
* pattern processing within the poll function.
*/
void LED::blinkRed(void) {
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
void LED::poll(void) {
	if (!ptr_pat) return;																		// seems we do not have connected any leds

	if (!active) return;																		// still waiting to do something
	if (!timer.done()) return;																	// active but timer not done
	
	// if we are here we have something to do, set the led, timer and counter
	timer.set(blPtr.pat[lCnt] * 10);															// set the timer for next check up

	if (blPtr.led0 && blPtr.pat[lCnt]) {
		ledRed((lCnt % 2)^1);																	// set the led
		#ifdef LD_DBG
		dbg << "lCnt:" << lCnt << " led0: " << ((lCnt % 2)^1) << '\n';
		#endif
	}
	
	if (blPtr.led1 && blPtr.pat[lCnt]) {
		ledGrn((lCnt % 2)^1);
		#ifdef LD_DBG
		dbg << "lCnt:" << lCnt  << " led1: " << ((lCnt % 2)^1) << '\n';
		#endif
	}
	lCnt++;																						// increase the pointer for the blink string

	// check if position pointer runs out of string
	if (lCnt >= blPtr.len) {																	// we are through the pattern 
		if (blPtr.dur == 0) {																	// we are in an endless loop
			lCnt = 0;
			#ifdef LD_DBG
			dbg << "lCnt 0\n";
			#endif

		} else if (dCnt < blPtr.dur) {															// duration is not done
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
