//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de>, <dirk@hfma.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _CM_H
	#define _CM_H

	#include "HAL.h"

	#define GET_2_BYTE_VALUE(n) ( ((uint16_t)n >> 8) | ((uint16_t)n << 8) )			// macro for changing byte order for ARV little endian

	#define AS_CM_JT_NONE                                 0
	#define AS_CM_JT_ONDELAY                              1
	#define AS_CM_JT_REFON                                2
	#define AS_CM_JT_RAMPON                               3
	#define AS_CM_JT_ON                                   4
	#define AS_CM_JT_OFFDELAY                             5
	#define AS_CM_JT_REFOFF                               6
	#define AS_CM_JT_RAMPOFF                              7
	#define AS_CM_JT_OFF                                  8

	#define AS_CM_ACTIONTYPE_INACTIVE                     0
	#define AS_CM_ACTIONTYPE_JUMP_TO_TARGET               1
	#define AS_CM_ACTIONTYPE_TOGGLE_TO_COUNTER            2
	#define AS_CM_ACTIONTYPE_TOGGLE_INVERSE_TO_COUNTER    3
	#define AS_CM_ACTIONTYPE_UPDIM                        4
	#define AS_CM_ACTIONTYPE_DOWNDIM                      5
	#define AS_CM_ACTIONTYPE_TOGGLEDIM                    6
	#define AS_CM_ACTIONTYPE_TOGGLEDIM_TO_COUNTER         7
	#define AS_CM_ACTIONTYPE_TOGGLEDIM_TO_COUNTER_INVERSE 8

	#define AS_CM_STATETOSEND_NONE                        0
	#define AS_CM_STATETOSEND_ACK                         1
	#define AS_CM_STATETOSEND_STATE                       2

	#define AS_CM_EXTSTATE_NONE                           0x00
	#define AS_CM_EXTSTATE_UP                             0x10
	#define AS_CM_EXTSTATE_DOWN                           0x20
	#define AS_CM_EXTSTATE_RUNNING                        0x40
	#define AS_CM_EXTSTATE_LOWBAT                         0x80

	void debugActionType(uint8_t actionType);
	void debugState(uint8_t state);
	inline void debugShowStruct(void);
#endif
