/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin config button class --------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _AS_CONF_BUTTON_H
#define _AS_CONF_BUTTON_H

#include "waittimer.h"

#define detectLong      3000
#define timeoutDouble   1000

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
 * short press       = send pairing string
 * long press        = nothing
 * double long       = reset to factory defaults
 *
 * - scenario 02 ----------------------------
 * short press       = toggle channel one
 * long press        = send pairing string
 * double long       = reset to factory defaults
 *
 * - scenario 03 ----------------------------
 * short press       = send pairing string
 * long press        = nothing
 * double long       = reset to factory defaults
 *
 * depending on channel 0, list 0, register xxx functionality is switched to
 * short press       = like a peered key
 * long press        = like a peered key
 * double long       = nothing
 *
 * Interface to the hardware is done via the register byte address of the port
 * and hand over of the respective bit number within this byte
 */
class CBN  {
public:		//---------------------------------------------------------------------------------------------------------

	struct s_button_check_flags {
		uint8_t armed      : 1;												// if this is set to 1, poll function should be entered
		uint8_t check_dbl  : 1;												// if the last key press was a long to detect a double long
		uint8_t last_long  : 1;												// if the last keypress was a long to detect a double long
		uint8_t            : 5;												// variable to store current status in polling function
	} flags;
	  
	waittimer timer;														// button timer functionality
	uint8_t mode;															// there are several modes available, see description above
	uint8_t status;															// variable to store current status in polling function
	uint8_t def_key;														// hold the pin number

	CBN(uint8_t cbn_mode, uint8_t pin_def);									// constructor with pin definition
	CBN(uint8_t cbn_mode);													// without pin, should be only used as mode 0

	void init(void);														// initialize the config key
	void poll(void);														// poll to process status of button
	void button_action(MSG_CBN::E action);									// function to drive the output signal
};

void set_cbn_timer(uint32_t millis);										// helper function to set the button timer and the stay_awake timer

#endif 
