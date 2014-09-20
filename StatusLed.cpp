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

// some led macros, to avoid errors we set the led to NULL while not available
#ifdef led0_off()
	#define led0(x) (x?led0_on():led0_off())
#else
	#define led0(x) (NULL)
#endif

#ifdef led1_off()
	#define led1(x) (x?led1_on():led1_off())
#else
	#define led1(x) (NULL)
#endif


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

	led0(0);																				// new program starts, so switch leds off
	led1(0);

	if      (stat == pairing)  blinkPtr = &sPairing;
	else if (stat == pair_suc) blinkPtr = &sPair_suc;
	else if (stat == pair_err) blinkPtr = &sPair_err;
	else if (stat == send)     blinkPtr = &sSend;
	else if (stat == ack)      blinkPtr = &sAck;
	else if (stat == bat_low)  blinkPtr = &sBattLow;
	else if (stat == defect)   blinkPtr = &sDefect;
	else if (stat == welcome)  blinkPtr = &sWelcome;

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
void	LD::poll(void) {
	if (!active) return;																	// still waiting to do something
	if (!ledTmr.done()) return;																// active but timer not done
	
	// if we are here we have something to do, set the led, timer and counter
	ledTmr.set(blinkPtr->pat[lCnt]*10);														// set the timer for next check up
	if ((blinkPtr->led0) && (blinkPtr->pat[lCnt])) {
		led0((lCnt % 2)^1);																	// set the led
		//dbg << "lCnt:" << lCnt << " led0: " << ((lCnt % 2)^1) << '\n';
	}
	
	if ((blinkPtr->led1) && (blinkPtr->pat[lCnt])) {
		led1((lCnt % 2)^1);
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
