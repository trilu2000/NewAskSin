/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin config button class --------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


// public:		//---------------------------------------------------------------------------------------------------------
CBN::CBN(uint8_t mode, uint8_t pin_def) {
	def_key = pin_def;
	button_check.configured = 1;															// poll the pin make only sense if it was configured, store result here
	button_check.scenario = mode;
}

CBN::CBN(const uint8_t mode) {
	button_check.configured = 0;															// poll the pin make only sense if it was configured, store result here
	button_check.scenario = mode;
}

void CBN::init(void) {
	register_PCINT(def_key);																// prepare hardware and register interrupt

	status = check_PCINT(def_key, 0);														// get the latest information

}
void CBN::poll(void) {
	/* we check the following options - keyShort, keyLong, keyLongRelease, keyDblLong, keyDblLongRelease */
	#define detectLong      3000
	#define timeoutDouble   1000
	
	if (!button_check.scenario) return;														// mode not set, nothing to do
	if (!button_check.configured) return;													// pin info not set, nothing to do

	// 0 for button is pressed, 1 for released, 2 for falling and 3 for rising edge
	status = check_PCINT(def_key, 1);														// check if an interrupt had happened

	/* button was just pressed, start for every option */
	if (status == 2) {
		timer.set(detectLong);																// set timer to detect a long
		pom->stayAwake(detectLong + 100);													// stay awake to check button status
		button_check.armed = 1;																// set it armed
	}
	if (!button_check.armed) return;														// nothing to do any more

	/* button was just released, keyShortSingle, keyLongRelease, keyDblLongRelease */
	if (status == 3) {
		timer.set(timeoutDouble);															// set timer to clear the repeated flags
		pom->stayAwake(timeoutDouble + 100);													// stay awake to check button status

		if ((button_check.last_long) && (!button_check.check_dbl)) {
		/* keyLongRelease, could be the start of a keyDblLong */
			button_check.check_dbl = 1;
			button_check.last_long = 0;
			button_action(MSG_CBN::keyLongRelease);

		} else if ((button_check.last_long) && (button_check.check_dbl)) {
		/* keyLongDblRelease, end of story */
			button_action(MSG_CBN::keyDblLongRelease);

		} else {
		/* keyShortSingle */
			button_action(MSG_CBN::keyShort);
		}
	}

	/* button is still pressed or released but timer is running we have to wait */
	if (!timer.done()) return;

	/* button is still pressed, but timed out, seems to be a keyLong or keyDblLong */
	if (status == 0) {
		timer.set(detectLong);																// set timer to detect a long
		pom->stayAwake(detectLong + 100);													// stay awake to check button status
		if (button_check.last_long) return;													// we had recognized the status already
		button_check.last_long = 1;															// remember that it was a long
		if (button_check.check_dbl) button_action(MSG_CBN::keyDblLong);						// we had already a long, this time it is a double long
		else button_action(MSG_CBN::keyLong);												// first time long
	}

	/* button is not pressed for a longer time, clean up */
	if (status == 1) {
		button_check.armed = 0;
		button_check.last_long = 0;
		button_check.check_dbl = 0;
	}
}

void CBN::button_action(MSG_CBN::E mode) {
	//led.blinkRed();																		// show via led that we have some action in place


	DBG(CB, F("CBN: "));																	// ...and some information
	if (mode == MSG_CBN::keyShort) {
		DBG(CB, F("keyShort"));					
		led->stop();
		led->set(LED_STAT::LED_RED_L);
		if (button_check.scenario == 1) hm.send_DEVICE_INFO(MSG_REASON::INITIAL);				// send pairing string
//		else if (button_check.scenario == 2) cmm[1]->set_toggle();							// send toggle to user module registered on channel 1

	} else if (mode == MSG_CBN::keyLong) {
		DBG(CB, F("keyLong"));
		led->set(LED_STAT::RESET_SLOW);

	} else if (mode == MSG_CBN::keyLongRelease) {
		DBG(CB, F("keyLongRelease"));
		if (button_check.scenario == 2) hm.send_DEVICE_INFO(MSG_REASON::INITIAL);				// send pairing string

	} else if (mode == MSG_CBN::keyDblLong) {
		DBG(CB, F("keyDblLong"));
		led->set(LED_STAT::RESET_FAST);

	} else if (mode == MSG_CBN::keyDblLongRelease) {
		DBG(CB, F("keyDblLongRelease"));

		uint8_t *localResDis = cmm[0]->lstC.ptr_to_val(0x18);								// get register address
		if ((localResDis) && (*localResDis)) return;										// if we got a valid pointer and local reset disable flag is set, we cannot reset the device

		DBG(CB, F(", set factory defaults"));
		dev_operate.reset = 2;																// initiate the factory defaults
		led->stop();
		led->set(LED_STAT::LED_RED_L);

	}
	DBG(CB, '\n');
}
