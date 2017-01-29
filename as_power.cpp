//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin power management function --------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include "newasksin.h"

//PW pom;																						// declare power management, defined in Power.h

// private:		//---------------------------------------------------------------------------------------------------------

/**
* @brief Initialize the power module
*/
POM::POM(uint8_t power_mode) {
	pwrMode = power_mode;																// set default
}

/**
 * @brief Set power mode
 */
void POM::setMode(uint8_t mode) {
	pwrMode = mode;
	setSleepMode();
}

/**
 * @brief Stay awake for specific time
 *
 * @param time in milliseconds for stay awake
 */
void POM::stayAwake(uint16_t time) {
	if (time < timer.remain()) return;														// set new timeout only if we have to add something
	timer.set(time);
}

/**
 * @brief Check against active flag of various modules
 */
void POM::poll(void) {
	
	if (pwrMode == POWER_MODE_NO_SLEEP) return;												// no power savings, there for we can exit
	if (!timer.done()) return;																// timer active, jump out
	//if (checkWakeupPin()) return;															// wakeup pin active
	
	// some communication still active, jump out
	if ((snd_msg.active) || (list_msg.active) || (config_mode.active) || (pair_mode.active) || (cbn->button_check.armed)) return;
	
	#ifdef PW_DBG																			// only if pw debug is set
	dbg << '.';																				// ...and some information
	_delay_ms(1);
	uint32_t fTme = getMillis();
	#endif


	if (pwrMode == POWER_MODE_WAKEUP_ONRADIO) {												// check communication on power mode 1

		tmpCCBurst = com->detect_burst();
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
	com->set_idle();																		// set communication module to idle
	led->set(LED_STAT::NONE);																// switch off all led's

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
