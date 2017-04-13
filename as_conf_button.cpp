/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin config button class --------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


// public:		//---------------------------------------------------------------------------------------------------------
CBN::CBN(uint8_t cbn_mode, uint8_t pin_def) {
	mode = cbn_mode;
	def_key = pin_def;
}

CBN::CBN(uint8_t cbn_mode) {
	mode = cbn_mode;
}

void CBN::init(void) {
	if (!mode) return;																		// pin info not set, nothing to do

	register_PCINT(def_key);																// prepare hardware and register interrupt
	status = check_PCINT(def_key, 0);														// get the latest information
}

void CBN::poll(void) {
	/* we check the following options - keyShort, keyLong, keyLongRelease, keyDblLong, keyDblLongRelease */
	if (!mode) return;																		// mode not set, nothing to do

	/* 0 for button is pressed, 1 for released, 2 for falling and 3 for rising edge */
	status = check_PCINT(def_key, 1);														// check if an interrupt had happened


	/* button was just pressed, start for every option */
	if (status == 2) {
		set_cbn_timer(detectLong);															// set timer to detect a long and stay awake for the same time
		flags.armed = 1;																	// set it armed
	}
	if (!flags.armed) return;																// nothing to do any more


	/* button was just released, keyShortSingle, keyLongRelease, keyDblLongRelease */
	if (status == 3) {
		set_cbn_timer(timeoutDouble);														// set timer to clear the repeated flags and stay awake for the same time

		if ((flags.last_long) && (!flags.check_dbl)) {										// keyLongRelease, could be the start of a keyDblLong 
			flags.check_dbl = 1;
			flags.last_long = 0;
			button_action(MSG_CBN::keyLongRelease);

		} else if ((flags.last_long) && (flags.check_dbl)) {								// keyLongDblRelease, end of story
			button_action(MSG_CBN::keyDblLongRelease);

		} else {																			// keyShortSingle
			button_action(MSG_CBN::keyShort);
		}
	}

	/* button is still pressed or released but timer is running we have to wait */
	if (!timer.done()) return;

	/* button is still pressed, but timed out, seems to be a keyLong or keyDblLong */
	if (status == 0) {
		set_cbn_timer(detectLong);															// set timer to detect a long and stay awake for the same time
		if (flags.last_long) return;														// we had recognized the long status already, nothing to do any more

		flags.last_long = 1;																// remember that it was a long, while first time here
		if (flags.check_dbl) button_action(MSG_CBN::keyDblLong);							// we had already a long, this time it is a double long
		else button_action(MSG_CBN::keyLong);												// first time long
	}

	/* button is not pressed for a longer time, clean up */
	if (status == 1) flags = {};
}


void CBN::button_action(MSG_CBN::E action) {
	//led.blinkRed();																		// show via led that we have some action in place

	DBG(CB, F("CBN: "));																	// ...and some information

	if (action == MSG_CBN::keyShort) {
		DBG(CB, F("keyShort"));					
		led.stop();
		led.set(LED_STAT::LED_RED_L);
		if (mode == 1) hm.send_DEVICE_INFO(MSG_REASON::INITIAL);							// send pairing string
//		else if (button_check.scenario == 2) cmm[1]->set_toggle();							// send toggle to user module registered on channel 1

	} else if (action == MSG_CBN::keyLong) {
		DBG(CB, F("keyLong"));
		led.set(LED_STAT::RESET_SLOW);

	} else if (action == MSG_CBN::keyLongRelease) {
		DBG(CB, F("keyLongRelease"));
		if (mode == 2) hm.send_DEVICE_INFO(MSG_REASON::INITIAL);							// send pairing string

	} else if (action == MSG_CBN::keyDblLong) {
		DBG(CB, F("keyDblLong"));
		led.set(LED_STAT::RESET_FAST);

	} else if (action == MSG_CBN::keyDblLongRelease) {
		DBG(CB, F("keyDblLongRelease"));

		uint8_t *localResDis = cmm[0]->lstC.ptr_to_val(0x18);								// get register address
		if ((localResDis) && (*localResDis)) return;										// if we got a valid pointer and local reset disable flag is set, we cannot reset the device

		DBG(CB, F(", set factory defaults"));
		dev_operate.reset = 2;																// initiate the factory defaults
		led.stop();
		led.set(LED_STAT::LED_RED_L);

	}
	DBG(CB, '\n');
}


void set_cbn_timer(uint32_t millis) {
	cbn.timer.set(millis);		
	pom.stayAwake(millis + 100);
}