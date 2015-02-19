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

waitTimer ledTmr;																			// config timer functionality

// public:		//---------------------------------------------------------------------------------------------------------

// private:		//---------------------------------------------------------------------------------------------------------
LD::LD() {
} 
void    LD::init(uint8_t leds, AS *ptrMain) {
	
	#ifdef LD_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("LD.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
	bLeds = leds;
}
void    LD::set(ledStat stat) {
	if (!bLeds) return;																		// no led available, skip...
	//dbg << "stat: " << stat << '\n';

	ledRed(0);																				// new program starts, so switch leds off
	ledGrn(0);

	uint8_t leds = bLeds-1;
	if      (stat == pairing)  blinkPtr = &sPairing[leds];
	else if (stat == pair_suc) blinkPtr = &sPair_suc[leds];
	else if (stat == pair_err) blinkPtr = &sPair_err[leds];
	else if (stat == send)     blinkPtr = &sSend[leds];
	else if (stat == ack)      blinkPtr = &sAck[leds];
	else if (stat == noack)    blinkPtr = &sNoack[leds];
	else if (stat == bat_low)  blinkPtr = &sBattLow[leds];
	else if (stat == defect)   blinkPtr = &sDefect[leds];
	else if (stat == welcome)  blinkPtr = &sWelcome[leds];
	else if (stat == key_long)  blinkPtr = &sKeyLong[leds];

	active = 1;																				// make module active
	lCnt = 0;																				// set start position
	dCnt = 1;

	// some sanity on blink pointer
	if (blinkPtr->len == 0) stat = nothing;
	
	if (stat == nothing) {
		ledTmr.set(0);																		// timer done
		active = 0;																			// nothing to do any more
		return;																				// jump out
	}
}
void    LD::blinkRed(void) {
	if (!bLeds) return;																		// no led available, skip...

	ledRed(0);																				// switch led off
	_delay_ms(20);																			// wait
	ledRed(1);																				// switch led on
	_delay_ms(20);																			// wait
	ledRed(0);																				// switch led off
}
void	LD::poll(void) {
	if (!active) return;																	// still waiting to do something
	if (!ledTmr.done()) return;																// active but timer not done
	
	// if we are here we have something to do, set the led, timer and counter
	ledTmr.set(blinkPtr->pat[lCnt]*10);														// set the timer for next check up
	if ((blinkPtr->led0) && (blinkPtr->pat[lCnt])) {
		ledRed((lCnt % 2)^1);																	// set the led
		//dbg << "lCnt:" << lCnt << " led0: " << ((lCnt % 2)^1) << '\n';
	}
	
	if ((blinkPtr->led1) && (blinkPtr->pat[lCnt])) {
		ledGrn((lCnt % 2)^1);
		//dbg << "lCnt:" << lCnt  << " led1: " << ((lCnt % 2)^1) << '\n';
	}
	lCnt++;																					// increase the pointer for the blink string

	// check if position pointer runs out of string
	if (lCnt >= blinkPtr->len) {															// we are through the pattern 
		if (blinkPtr->dur == 0) {															// we are in an endless loop
			lCnt = 0;
			//dbg << "lCnt 0\n";
		} else if (dCnt < blinkPtr->dur) {													// duration is not done
			lCnt = 0;
			dCnt++;
			//dbg << "lCnt 0, dCnt++\n";
		} else {
			set(nothing);

		}
	}
}
