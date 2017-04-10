/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin power management class -----------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/


#include "newasksin.h"


/**
* @brief Initialize the power module
*/
POM::POM(uint8_t mode) {
}

/**
* @brief Stay awake for specific time
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
	timer.done();

	/* poll through the timer array to identify if all timers are completed or how far the next wake up time is
	*  timer completion means, a timer was checked with the done() function and checktime was reseted to 0, so the underlaying action was progressed */
/*	uint32_t next_wakeup = 0;
	uint8_t timer_active = 0;

	for (uint8_t i = 0; i < timer_max; i++) {
		uint8_t temp_status = timer_array[i]->completed();
		//dbg << i << ':' << temp_status << ", ";
		if (temp_status == 0) continue;														// timer not set, nothing to do

		if (temp_status == 1) {																// timer active and needs to be processed by calling done() function
			timer_active = temp_status;

		}
		else {																			// timer active and some remaining time
			uint32_t temp_time = timer_array[i]->remain();									// get the remaining time
			if ((!next_wakeup) || (next_wakeup > temp_time)) next_wakeup = temp_time;		// remind the remaining time if it is the next one

		}
	}*/

	//dbg << timer_active << ", " << next_wakeup << '\n';



	//	if (pwrMode == POWER_MODE_NO_SLEEP) return;												// no power savings, there for we can exit
	//	if (!timer.done()) return;																// timer active, jump out
	//if (checkWakeupPin()) return;															// wakeup pin active

	// some communication still active, jump out
	//	if ((snd_msg.active) || (list_msg.active) || (config_mode.active) || (pair_mode.active) || (cbn->button_check.armed)) return;

	//	#ifdef PW_DBG																			// only if pw debug is set
	//		dbg << '.';																			// ...and some information
	// uint32_t fTme = getMillis();
	//	#endif


	//	if (pwrMode == POWER_MODE_WAKEUP_ONRADIO) {												// check communication on power mode 1

	//		tmpCCBurst = com->detect_burst();
	//		if ((tmpCCBurst) && (!chkCCBurst)) {												// burst detected for the first time
	//			chkCCBurst = 1;																	// set the flag

	//			#ifdef PW_DBG																	// only if pw debug is set
	//				dbg << '1';																	// ...and some information
	//			#endif

	//		} else if ((tmpCCBurst) && (chkCCBurst)) {											// burst detected for the second time
	//			chkCCBurst = 0;																	// reset the flag
	//			stayAwake(500);																	// stay awake for some time to check if we receive a valid message

	//			#ifdef PW_DBG																	// only if pw debug is set
	//				dbg << '2';																	// ...and some information
	//			#endif

	//			return;																			// we don't want proceed further, jump out

	//		} else if ((!tmpCCBurst) && (chkCCBurst)) {											// secondary test was negative, reset the flag
	//			chkCCBurst = 0;																	// reset the flag

	//			#ifdef PW_DBG																	// only if pw debug is set
	//				dbg << '-';																	// ...and some information
	//			#endif			
	//		}
	//	}

	//	// if we are here, we could go sleep. set cc module idle, switch off led's and sleep
	//	com->set_idle();																		// set communication module to idle
	//	led->set(LED_STAT::NONE);																// switch off all led's

	// start the respective watchdog timers
	//	cli();
	//	if      ((pwrMode == POWER_MODE_WAKEUP_ONRADIO) && (!chkCCBurst)) startWDG250ms();
	//	else if ((pwrMode == POWER_MODE_WAKEUP_ONRADIO) && (chkCCBurst))  startWDG32ms();
	//	else if  (pwrMode == POWER_MODE_WAKEUP_32MS)                      startWDG32ms();
	//	else if  (pwrMode == POWER_MODE_WAKEUP_64MS)                      startWDG64ms();
	//	else if  (pwrMode == POWER_MODE_WAKEUP_250MS)                     startWDG250ms();
	//	else if  (pwrMode == POWER_MODE_WAKEUP_8000MS)                    startWDG8000ms();

	// todo: move sei() to setSleep() before sleep_cpu();
	//	sei();

	//	#if defined(PW_DBG)||defined(SER_DBG)||defined(SN_DBG)||defined(RV_DBG)||defined(EE_DBG)||defined(LD_DBG)||defined(CC_DBG)||defined(AS_DBG)||defined(AES_DBG)||defined(BT_DBG)||defined(TH_DBG)
	//		Serial.flush();																		// give UART some time to send last chars
	//	#endif

	//	setSleep();																				// call sleep function in HAL

	/*************************
	* Wake up at this point *
	*************************/
	//	if (pwrMode != POWER_MODE_WAKEUP_EXT_INT) {
	//		stopWDG();																			// stop the watchdog
	//	}

	//	stayAwake(6);																			// stay awake for a very short time to get things done

	//	#ifdef PW_DBG																			// only if pw debug is set
	//		dbg << ':';// << (getMillis() -fTme) << '\n';										// ...and some information
	//	#endif




}



