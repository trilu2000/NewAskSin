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

PW pom;																						// declare power management, defined in Power.h
waitTimer pwrTmr;																			// power timer functionality

// private:		//---------------------------------------------------------------------------------------------------------

/**
* @brief Initialize the power module
*/
PW::PW() {
	#ifdef PW_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("PW.\n");																		// ...and some information
	#endif

	pwrMode = POWER_MODE_NO_SLEEP;															// set default
}

/**
 * @brief Set power mode
 */
void PW::setMode(uint8_t mode) {
	pwrMode = mode;

	#ifdef PW_DBG																			// only if pw debug is set
	dbg << F("PowerMode: ") << pwrMode << '\n';											// ...and some information
	#endif

	initWakeupPin();
	setSleepMode();
	stayAwake(2000);																		// startup means stay awake for next 2 seconds
}

/**
 * @brief Stay awake for specific time
 *
 * @param time in milliseconds for stay awake
 */
void PW::stayAwake(uint16_t time) {
	if (time < pwrTmr.remain()) return;														// set new timeout only if we have to add something
	pwrTmr.set(time);
}

/**
 * @brief Check against active flag of various modules
 */
void PW::poll(void) {
	
	if (pwrMode == POWER_MODE_NO_SLEEP) return;												// no power savings, there for we can exit
	if (!pwrTmr.done()) return;																// timer active, jump out
	if (checkWakeupPin()) return;															// wakeup pin active
	
	// some communication still active, jump out
	if ((snd.flag.active) || (hm.stcSlice.active) || (config_mode.active) || (pair_mode.active) || (btn.armFlg)) return;
	
	#ifdef PW_DBG																			// only if pw debug is set
	dbg << '.';																				// ...and some information
	_delay_ms(1);
	uint32_t fTme = getMillis();
	#endif


	if (pwrMode == POWER_MODE_WAKEUP_ONRADIO) {												// check communication on power mode 1

		tmpCCBurst = cc.detectBurst();
		if ((tmpCCBurst) && (!chkCCBurst)) {												// burst detected for the first time
			chkCCBurst = 1;																	// set the flag
			
			#ifdef PW_DBG																	// only if pw debug is set
			dbg << '1';																		// ...and some information
			#endif

		} else if ((tmpCCBurst) && (chkCCBurst)) {											// burst detected for the second time
			chkCCBurst = 0;																	// reset the flag
			stayAwake(500);																	// stay awake for some time to check if we receive a valid message

			#ifdef PW_DBG																	// only if pw debug is set
			dbg << '2';																		// ...and some information
			#endif

			return;																			// we don't want proceed further, jump out
			
		} else if ((!tmpCCBurst) && (chkCCBurst)) {											// secondary test was negative, reset the flag
			chkCCBurst = 0;																	// reset the flag

			#ifdef PW_DBG																	// only if pw debug is set
			dbg << '-';																		// ...and some information
			#endif			
		}
	}

	// if we are here, we could go sleep. set cc module idle, switch off led's and sleep
	cc.setIdle();																		// set communication module to idle
	led.set(nothing);																	// switch off all led's

	// start the respective watchdog timers
	cli();
	if      ((pwrMode == POWER_MODE_WAKEUP_ONRADIO) && (!chkCCBurst)) startWDG250ms();
	else if ((pwrMode == POWER_MODE_WAKEUP_ONRADIO) && (chkCCBurst))  startWDG32ms();
	else if  (pwrMode == POWER_MODE_WAKEUP_32MS)                      startWDG32ms();
//	else if  (pwrMode == POWER_MODE_WAKEUP_64MS)                      startWDG64ms();
	else if  (pwrMode == POWER_MODE_WAKEUP_250MS)                     startWDG250ms();
	else if  (pwrMode == POWER_MODE_WAKEUP_8000MS)                    startWDG8000ms();

	// todo: move sei() to setSleep() before sleep_cpu();
	sei();

	setSleep();																				// call sleep function in HAL

	/*************************
	 * Wake up at this point *
	 *************************/
	if (pwrMode != POWER_MODE_WAKEUP_EXT_INT) {
		stopWDG();																			// stop the watchdog
	}

	stayAwake(6);																			// stay awake for a very short time to get things done
	
	#ifdef PW_DBG																			// only if pw debug is set
	dbg << ':';// << (getMillis() -fTme) << '\n';												// ...and some information
	#endif
		
}
