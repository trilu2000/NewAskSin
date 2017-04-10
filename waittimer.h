/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin wait timer class -----------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _WAITTIMER_H
#define _WAITTIMER_H

#include <stdint.h>


/**
* @short Timer class for non-blocking delays
*
* The following examples shows how to use the waitTimer class to
* perform an action every 500ms. Note that the first time loop()
* is called, delay.done() will return true and the action will
* be performed. The example also shows how to avoid the execution
* time of the action to influence the new delay time by setting
* the delay before performing the action.
* @code
* void loop()
* {
*     static waitTimer delay;
*     if (delay.done()) {
*         delay.set(500); // perform next action after 500ms
*         // do something now
*     }
* }
* @endcode
*
* @attention The waitTimer can only make sure a minimum time passes
* between set() and done(). If calls to done() are delayed due to other
* actions, more time may pass. Also, actual delay times strongly depend
* on the behaviour of the system clock.
*
* @see http://www.gammon.com.au/forum/?id=12127
*/
class waittimer {

private:	//---------------------------------------------------------------------------------------------------------
	uint32_t startTime;
	uint32_t checkTime;

public:		//---------------------------------------------------------------------------------------------------------
	waittimer();
	uint8_t  done(void);
	void     set(uint32_t wait_millis);
	uint32_t remain(void);
	uint8_t  completed(void);
};


extern uint8_t timer_max;																	// counter for registered waittimers
extern waittimer *timer_array[];															// timer array to have a central reference to all timers for power management

#endif