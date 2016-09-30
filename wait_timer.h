/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin wait timer functions -------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _WAIT_TIMER_H
#define _WAIT_TIMER_H

#include "HAL.h"

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
class waitTimer {

private:		//---------------------------------------------------------------------------------------------------------
	uint8_t  armed;
	uint32_t checkTime;
	uint32_t startTime;

public:		//---------------------------------------------------------------------------------------------------------
	waitTimer();
	uint8_t  done(void);
	void     set(uint32_t ms);
	uint32_t remain(void);
};

#endif