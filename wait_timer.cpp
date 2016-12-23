/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin wait timer functions -------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "wait_timer.h"

/**
* @brief Constructor to initialize waittimer
*/
waitTimer::waitTimer() {
	armed = 0;																					// if we are here, timeout was happened, next loop status 1 will indicated
}

/**
* @brief Query if the timer has expired
*
* @return 0 if timer is still running, 1 if not.
*         If the timer was never set(), return value is 1
*/
uint8_t  waitTimer::done(void) {
	if (!armed) return 1;																		// not armed, so nothing to do
	if ((get_millis() - startTime) < checkTime) return 0;										// not ready yet
	checkTime = 0;
	armed = 0;																					// if we are here, timeout was happened, next loop status 1 will indicated
	return 1;
}

/**
* @brief Start the timer
*
* @param ms Time until timer is done() (unit: ms)
*/
void     waitTimer::set(uint32_t ms) {
	armed = ms ? 1 : 0;
	if (armed) {
		startTime = get_millis();
		checkTime = ms;
	}
}

/**
* @brief Query the remaing time until the timer is done
*
* @return Time until timer is done() (unit: ms)
*/
uint32_t waitTimer::remain(void) {
	if (!armed) return 0;
	return (checkTime - (get_millis() - startTime));
}

