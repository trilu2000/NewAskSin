//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin config key function --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _CBN_H
#define _CBN_H

#include "HAL.h"
#include "wait_timer.h"

namespace MSG_CBN {
	enum E : uint8_t { keyShort = 1, keyLong = 2, keyLongRelease = 3, keyDblLong = 4, keyDblLongRelease = 5, };
};


/**
 * @short Class for the handling the configuration key of the device
 * Right now there are three possible scenarios which could be configured via the mode flag.
 *
 * - scenario 00 ----------------------------
 * no config button used
 *
 * - scenario 01 ----------------------------
 * short press       = pair
 * long press        = nothing
 * double long       = reset
 *
 * - scenario 02 ----------------------------
 * short press       = toggle channel one
 * long press        = pair
 * double long       = reset
 *
 * Interface to the hardware is done via the register byte address of the port
 * and hand over of the respective bit number within this byte
 */
class CBN  {
  public:		//---------------------------------------------------------------------------------------------------------

	struct s_button_check {
		uint8_t configured : 1;												// poll the pin make only sense if it was configured, store result here
		uint8_t scenario   : 2;												// scenario, see description above
		uint8_t armed      : 1;												// if this is set to 1, poll function should be entered
		uint8_t check_dbl  : 1;												// if the last key press was a long to detect a double long
		uint8_t last_long  : 1;												// if the last keypress was a long to detect a double long
		uint8_t            : 2;
	} button_check;
	  
	struct s_button_ref {
		uint8_t port;														// port information for checking interrupt
		uint8_t pin;														// pin information for checking interrupt
		uint8_t status;														// variable to store current status in polling function
		uint8_t counter = 0;												// channel specific message counter
	} button_ref;

	waitTimer timer;														// button timer functionality

	CBN(uint8_t mode = 0);
	CBN(uint8_t mode, uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC);																	// constructor 
	void poll(void);														// poll to process status of button
	void button_action(MSG_CBN::E mode);									// function to drive the output signal
};


#endif 
