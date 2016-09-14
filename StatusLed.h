//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------



#ifndef _LD_H
#define _LD_H

#include "HAL.h"

/**
* @brief LED patterns are defined in HAL.h and HAL_extern.h while we use different patterns depending on
* the LED's connected to the hardware. LED's are defined in hardware.h in user sketch area...
*/

enum ledStat { pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long, nothing = 255 };

class LD {
public:		//---------------------------------------------------------------------------------------------------------
	uint8_t active;															// something active

	struct s_blPat blPtr;													// empty blink pattern struct
	uint8_t lCnt;															// counter for positioning inside of blink string
	uint8_t dCnt;															// duration counter
	
	LD();																	// class constructor
	void poll(void);														// poll function to process blink pattern
	void set(ledStat stat);													// function to set the blink pattern
	void blinkRed(void);													// blocking blink pattern, not processed via poll
};

extern LD led;																// declaration is in AS.cpp

#endif
