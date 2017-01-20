/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Remote ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module switch
*        pointer to channel table are forwarded to the master class. 
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/
#include "cm_remote.h"

/**
* @brief This function has to be called to handover all pin information to initialize the hardware port/pin
* and to register the pin change interrupt. The channel module will remember on the pin information and poll the
* pin status. If a change/interrupt is recognized, the module will act accordingly.
* There is a more passive option to use this module. Helpful if you don't use a dedicated pin per channel.
* In such a case you can call the buttonAction() directly from user sketch.
* @param   port/pin information    PIN_C2
*/
CM_REMOTE::CM_REMOTE(const uint8_t peer_max, const s_pin_def *ptr_key_pin) : CM_MASTER(peer_max) {
	key_pin = ptr_key_pin;

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cm_remote_ChnlReg;
	lstC.def = cm_remote_ChnlDef;
	lstC.len = sizeof(cm_remote_ChnlReg);

	lstP.lst = 4;																			// setup the peer list with all dependencies
	lstP.reg = cm_remote_PeerReg;
	lstP.def = cm_remote_PeerDef;
	lstP.len = sizeof(cm_remote_PeerReg);

	static uint8_t lstCval[sizeof(cm_remote_ChnlReg)];
	lstC.val = lstCval;
	//lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	static uint8_t lstPval[sizeof(cm_remote_PeerReg)];
	lstP.val = lstPval;
	//lstP.val = new uint8_t[lstP.len];

	l1 = (s_l1*)lstC.val;																	// set list structures to something useful
	l4 = (s_l4*)lstP.val;
}

CM_REMOTE::CM_REMOTE(const uint8_t peer_max) : CM_MASTER(peer_max) {

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cm_remote_ChnlReg;
	lstC.def = cm_remote_ChnlDef;
	lstC.len = sizeof(cm_remote_ChnlReg);

	lstP.lst = 4;																			// setup the peer list with all dependencies
	lstP.reg = cm_remote_PeerReg;
	lstP.def = cm_remote_PeerDef;
	lstP.len = sizeof(cm_remote_PeerReg);

	static uint8_t lstCval[sizeof(cm_remote_ChnlReg)];
	lstC.val = lstCval;
	//lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	static uint8_t lstPval[sizeof(cm_remote_PeerReg)];
	lstP.val = lstPval;
	//lstP.val = new uint8_t[lstP.len];

	l1 = (s_l1*)lstC.val;																	// set list structures to something useful
	l4 = (s_l4*)lstP.val;
}

void CM_REMOTE::cm_init() {
	if (key_pin) {
		register_PCINT(key_pin);
		button_ref.status = check_PCINT(key_pin, 0);										// get the latest information
		button_check.configured = 1;														// poll the pin make only sense if it was configured, store result here
		DBG(RE, F("RE:init_pin, cnl: "), lstC.cnl, F(", pin: "), key_pin->PINBIT, F(", port: "), key_pin->VEC, F(", LONG_PRESS_TIME: "), byteTimeCvt(l1->LONG_PRESS_TIME), F(", DBL_PRESS_TIME: "), byteTimeCvt(l1->DBL_PRESS_TIME), F(", AES_ACTIVE: "), l1->AES_ACTIVE, '\n');
	}

	initRemote(lstC.cnl);																	// call external init function to set the input pins
	DBG(RE, F("cmRemote, cnl: "), lstC.cnl, '\n');
}

/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */

void CM_REMOTE::cm_poll(void) {
	#define repeatedLong 250

	if (!button_check.configured) return;													// if port is not configured, poll makes no sense
	button_ref.status = check_PCINT(key_pin, 1);											// check if an interrupt had happened

	/* button was just pressed, start for every option */
	if (button_ref.status == 2) {
		timer.set(byteTimeCvt(l1->LONG_PRESS_TIME));										// set timer to detect a long
		pom->stayAwake(byteTimeCvt(l1->LONG_PRESS_TIME) + 500);								// stay awake to check button status
		button_check.armed = 1;																// set it armed
	}
	if (!button_check.armed) return;

	/* button was just released, keyShortSingle, keyShortDouble, keyLongRelease */
	if (button_ref.status == 3) {
		timer.set(byteTimeCvt(l1->DBL_PRESS_TIME));											// set timer to clear the repeated flags
		pom->stayAwake(byteTimeCvt(l1->DBL_PRESS_TIME) + 500);								// stay awake to check button status

		/* keyLongRelease */
		if (button_check.last_long) { 	
			button_action(4);

		/* keyShortDouble */
		} else if (button_check.last_short) {
			button_check.last_short = 0;
			button_action(2);

		/* keyShortSingle */
		} else if (!button_check.last_long) {
			button_check.last_short = 1;													// waiting for a key short double
			button_action(1);
		}
		button_check.last_long = 0;															// some cleanup
	} 
	
	/* button is still pressed or released, if timer is running we have to wait */
	if (!timer.done()) return;																				

	/* button is still pressed, but timed out, seems to be a keyLong */
	if (button_ref.status == 0) {	
		pom->stayAwake(repeatedLong + 500);													// stay awake to check button status
		timer.set(repeatedLong);															// set timer to detect a repeated long
		button_check.last_long = 1;															// remember that it was a long
		button_action(3);																	// last key state was a long, now it is a repeated long
	} 
	
	/* button is not pressed for a longer time, clean up */
	if (button_ref.status == 1) {	
		button_check = {};
		button_check.configured = 1;
		//button_check.armed = button_check.last_long = button_check.last_short = 0;
	}
}


/**
* @brief Sending out an Asksin message by the given action. Function can be called from user sketch directly.
* Intend is, not every remote channel needs an hardware pin anymore. Looking for further projects like a
* wall mount touch remote.
* @param   bEvent   1 - short key press
*                   2 - double short key press
*                   3 - repeated long key press
*                   4 - end of long key press
*                 255 - key press, for stay awake issues
*/
void CM_REMOTE::button_action(uint8_t event) {
	// at the moment this channel module will only work for channel > 0 while key for maintanance channel need
	// some special functionality, like link to toogle and pairing

	DBG(RE, F("RM:buttonAction, cnl: "), lstC.cnl, F(", evt:"), event, '\n');
	if ((event == 1) && (l1->DBL_PRESS_TIME)) return;										// when double press is set, we do not report a key single

	pom->stayAwake(1000);																	// make some time to send the message
	if (event == 255) return;																// was only a wake up message

	if ((event == 3) || (event == 4)) button_info.longpress = 1;							// set the long key flag if requested
	else button_info.longpress = 0;															// otherwise it is a short
	button_info.lowbat = bat->get_status();
	button_info.channel = lstC.cnl;
	button_info.counter = button_ref.counter;

	hm->send_REMOTE((event != 3) ? 1 : 0, this, (uint8_t*)&button_info);
	if (event != 3) button_ref.counter++;													// increase the button counter
}

