//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin power management function --------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _AS_POWER_H
#define _AS_POWER_H

#include "waittimer.h"



/*
 * POWER_MODE_NO_SLEEP:       There is nothing to do. Devices active all time. No power savings.
 *                            Todo: maybe set idle mode to save some energy
 *
 * POWER_MODE_WAKEUP_ONRADIO: Check every 250ms if there is a transmission signal
 *                            If signal recognized, wait 50ms and check again - if it is still active
 *                            Then wake up the device for some time. If no signal recognized, continue sleeping
 *                            Todo: wake on radio should do the CC1101 and wake up the MC over interrupt
 *
 * POWER_MODE_WAKEUP_250MS:   Sleep for 250ms, wake up - check if something is to do, otherwise continue sleeping
 *                            Communication module stays idle.
 *
 * POWER_MODE_WAKEUP_8000MS:  Sleep for 8000ms, wake up - check if something is to do, otherwise continue sleeping
 *                            Communication module stays idle.
 *
 * POWER_MODE_WAKEUP_EXT_INT: Sleep for ever until an external interrupt was triggered.
 */
#define POWER_MODE_NO_SLEEP       0
#define POWER_MODE_WAKEUP_ONRADIO 1
#define POWER_MODE_WAKEUP_32MS    2
#define POWER_MODE_WAKEUP_64MS    3
#define POWER_MODE_WAKEUP_250MS   4
#define POWER_MODE_WAKEUP_8000MS  5
#define POWER_MODE_WAKEUP_EXT_INT 6

class POM {
public:		//---------------------------------------------------------------------------------------------------------

	uint8_t pwrMode;														// remember the level of power savings
	uint8_t chkCCBurst;
	uint8_t comStat;
	uint8_t tmpCCBurst;
	waitTimer timer;														// power timer functionality

	POM(uint8_t power_mode);

	//void setMode(uint8_t mode);
	void stayAwake(uint16_t time);

	void poll(void);
};


#endif 
