/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin wait timer functions -------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "waittimer.h"
#include "HAL.h"

static uint8_t timer_max;


/**
* @brief Constructor to initialize waittimer
*/
waittimer::waittimer() {
	timer_array[timer_max] = this;
	timer_max += 1;
}

/**
* @brief Query if the timer has expired
*
* @return 0 if timer is still running, 1 if not.
*         If the timer was never set(), return value is 1
*/
uint8_t  waittimer::done(void) {
	if (!checkTime) return 1;																// not armed, so nothing to do
	if ((get_millis() - startTime) < checkTime) return 0;									// not ready yet
	checkTime = 0;																			// if we are here, timeout was happened
	return 1;																				// return a 1 for done
}

/**
* @brief Start the timer
*
* @param ms Time until timer is done() (unit: ms)
*/
void     waittimer::set(uint32_t wait_millis) {
	uint8_t armed = (wait_millis) ? 1 : 0;
	if (!armed) return;
	startTime = get_millis();
	checkTime = wait_millis;
}

/**
* @brief Query the remaing time until the timer is done
*
* @return Time until timer is done() (unit: ms)
*/
uint32_t waittimer::remain(void) {
	if (!checkTime) return 0;
	return (checkTime - (get_millis() - startTime));
}

/* returns the status of the timer
* 0 not active 
* 1 active and remaining time is 0 or below, but not progressed via done()
* 2 active and remaining time is above 0 */
uint8_t waittimer::completed(void) {
	if (!checkTime) return 0;																// not armed, so return not active
	else if ((get_millis() - startTime) >= checkTime) return 1;								// timer done, but not progressed
	else return 2;																			// time not ready, need some additional time
}