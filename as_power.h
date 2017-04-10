/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin power management class -----------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _AS_POWER_H
#define _AS_POWER_H

#include "waittimer.h"


/* power modes needed */
/* POWER_MODE_NO_SLEEP         There is nothing to do. Devices active all time. No power savings
*/
/* POWER_MODE_WAKEUP_ONBURST   Check every 250ms if there is a transmission signal
*                              If signal recognized, wait 50ms and check again - if it is still active
*                              Then wake up the device for some time.If no signal recognized, continue sleeping
*/
/* POWER_MODE_WAKEUP_ONTIMER   Sleep as long as no waittimer raised an event
*/
/* POWER_MODE_WAKEUP_ONEXTINT  Sleep for ever until an external interrupt was triggered
*/
class POM {
public:
	waittimer timer;

	POM(uint8_t mode);
	void stayAwake(uint16_t time);
	void poll(void);
};

#endif 
