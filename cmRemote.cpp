/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Remote ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module switch
*        pointer to channel table are forwarded to the master class. 
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/
#include "cmRemote.h"

cmRemote::cmRemote(const uint8_t peer_max) : cmMaster(peer_max) {

	lstC.lst = 1;																			// setup the channel list with all dependencies
	lstC.reg = cmRemote_ChnlReg;
	lstC.def = cmRemote_ChnlDef;
	lstC.len = sizeof(cmRemote_ChnlReg);

	lstP.lst = 4;																			// setup the peer list with all dependencies
	lstP.reg = cmRemote_PeerReg;
	lstP.def = cmRemote_PeerDef;
	lstP.len = sizeof(cmRemote_PeerReg);

	lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	lstP.val = new uint8_t[lstP.len];


	l1 = (s_l1*)lstC.val;																	// set list structures to something useful
	l4 = (s_l4*)lstP.val;

}

void cmRemote::cm_init() {
	initRemote(lstC.cnl);																	// call external init function to set the input pins
	DBG(RE, F("cmRemote, cnl: "), lstC.cnl, '\n');

}

/**------------------------------------------------------------------------------------------------------------------------
*- user defined functions -
* ------------------------------------------------------------------------------------------------------------------------- */

/**
* @brief This function has to be called to handover all pin information to initialize the hardware port/pin
* and to register the pin change interrupt. The channel module will remember on the pin information and poll the
* pin status. If a change/interrupt is recognized, the module will act accordingly.
* There is a more passive option to use this module. Helpful if you don't use a dedicated pin per channel.
* In such a case you can call the buttonAction() directly from user sketch.
* @param   port/pin information    PIN_C2
*/
void cmRemote::cm_init_pin(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC) {

	registerPCINT(PINBIT, DDREG, PORTREG, PINREG, PCINR, PCIBYTE, PCICREG, PCIMASK, PCIEREG, VEC);// prepare hardware and register interrupt

	button_ref.port = VEC;																	// port information for checking interrupt
	button_ref.pin = PINBIT;																// pin information for checking interrupt
	button_ref.status = checkPCINT(button_ref.port, button_ref.pin, 0);						// get the latest information

	button_check.configured = 1;															// poll the pin make only sense if it was configured, store result here
	DBG(RE, F("RE:init_pin, cnl: "), lstC.cnl, F(", pin: "), button_ref.pin, F(", port: "), button_ref.port, F(", LONG_PRESS_TIME: "), byteTimeCvt(l1->LONG_PRESS_TIME), F(", DBL_PRESS_TIME: "), byteTimeCvt(l1->DBL_PRESS_TIME), F(", AES_ACTIVE: "), l1->AES_ACTIVE, '\n');
}


#define repeatedLong 250
#define LONG_PRESS_TIME  byteTimeCvt(l1->LONG_PRESS_TIME)
#define DBL_PRESS_TIME  byteTimeCvt(l1->DBL_PRESS_TIME)

void cmRemote::cm_poll(void) {

	if (!button_check.configured) return;													// if port is not configured, poll makes no sense
	button_ref.status = checkPCINT(button_ref.port, button_ref.pin, 1);						// check if an interrupt had happened

	/* button was just pressed, start for every option */
	if (button_ref.status == 2) {
		timer.set(LONG_PRESS_TIME);															// set timer to detect a long
		pom.stayAwake(LONG_PRESS_TIME + 500);												// stay awake to check button status
		button_check.armed = 1;																// set it armed
	}
	if (!button_check.armed) return;

	/* button was just released, keyShortSingle, keyShortDouble, keyLongRelease */
	if (button_ref.status == 3) {
		timer.set(DBL_PRESS_TIME);															// set timer to clear the repeated flags
		pom.stayAwake(DBL_PRESS_TIME + 500);												// stay awake to check button status

		/* keyLongRelease, could be a long_double */
		if (button_check.last_long) { 	
			button_check.wait_dbl_long = 1;													// waiting for a key long double
			button_action(5);

		/* keyShortDouble */
		} else if (button_check.last_short) {
			button_check.last_short = 0;
			button_action(2);

		/* keyShortSingle */
		} else if ((!button_check.last_long) && (!button_check.wait_dbl_long)) {
			button_check.last_short = 1;													// waiting for a key short double
			button_action(1);

		/* keyLongDoubleRelease */
		} else if (button_check.last_rpt_long && button_check.wait_dbl_long) {														
			button_action(7);
		}
		button_check.last_long = button_check.last_rpt_long = 0;							// some cleanup
	} 
	
	/* button is still pressed or released, if timer is running we have to wait */
	if (!timer.done()) return;																				

	/* button is still pressed, but timed out, seems to be a long */
	if (button_ref.status == 0) {	
		pom.stayAwake(LONG_PRESS_TIME + 500);												// stay awake to check button status

		/* detect a repeated double long */
		if (button_check.last_rpt_long  && button_check.wait_dbl_long) {					
			timer.set(LONG_PRESS_TIME);														// set timer to detect next long

		/* keyLongRepeat */
		} else if (button_check.last_long) {	
			timer.set(repeatedLong);														// set timer to detect a repeated long
			button_check.last_rpt_long = 1;
			button_action(4);																// last key state was a long, now it is a repeated long

		/* keyLongDouble */
		} else if (button_check.wait_dbl_long) {
			timer.set(LONG_PRESS_TIME);														// set timer to detect next long
			button_check.last_rpt_long = 1;													// remember last was long
			button_action(6);																// double flag is set, means was the second time for a long													

		/* keyLongSingle */
		} else {
			timer.set(repeatedLong);														// set timer to detect a repeated long
			button_check.last_long = 1;														// remember last was long
			button_action(3);																// first time detect a long
		}
	} 
	
	/* button is not pressed for a longer time, clean up */
	if (button_ref.status == 1) {	
		button_check = {};
		button_check.configured = 1;
		//button_check.armed = button_check.wait_dbl_long = button_check.last_long = button_check.last_rpt_long = button_check.last_short = 0;
	}
}


/**
* @brief Sending out an Asksin message by the given action. Function can be called from user sketch directly.
* Intend is, not every remote channel needs an hardware pin anymore. Looking for further projects like a
* wall mount toch switch.
* @param   bEvent   1 - short key press
*                   2 - double short key press
*                   3 - long key press
*                   4 - repeated long key press
*                   5 - end of long key press
*                   6 - double long key press
*                   7 - end of double long key press
*                 255 - key press, for stay awake issues
*/
void cmRemote::button_action(uint8_t event) {
	// at the moment this channel module will only work for channel > 0 while key for maintanance channel need
	// some special functionality, like link to toogle and pairing

	pom.stayAwake(1000);																	// overcome the problem of not getting a long repeated key press
	if (event == 255) return;																// was only a wake up message

	DBG(RE, F("RM:buttonAction, cnl: "), lstC.cnl, F(", evt:"), event, '\n');

	if ((event == 3) || (event == 4)) button_info.longpress = 1;							// set the long key flag if requested
	else button_info.longpress = 0;															// otherwise it is a short
	dbg << "x:" << button_info.longpress << '\n';

	//	if (bEvent == 3) hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo);						// send the message
	//	else hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo, AS_ACK_REQ);

	button_info.counter++;																	// increase the button counter
}

