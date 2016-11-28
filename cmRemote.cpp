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
void cmRemote::cm_init(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC) {

	registerPCINT(PINBIT, DDREG, PORTREG, PINREG, PCINR, PCIBYTE, PCICREG, PCIMASK, PCIEREG, VEC);// prepare hardware and register interrupt

	button_ref.port = VEC;																	// port information for checking interrupt
	button_ref.pin = PINBIT;																// pin information for checking interrupt
	button_ref.status = checkPCINT(button_ref.port, button_ref.pin, 0);						// get the latest information

	button_check.configured = 1;															// poll the pin make only sense if it was configured, store result here
	DBG(RE, F("RE:config, cnl: "), lstC.cnl, F(", pin: "), button_ref.pin, F(", port: "), button_ref.port, '\n');
}


#define repeatedLong 250
void cmRemote::cm_poll(void) {

	if (!button_check.configured) return;													// if port is not configured, poll makes no sense
	button_ref.status = checkPCINT(button_ref.port, button_ref.pin, 1);						// check if an interrupt had happened


	if (button_ref.status == 2) {								// button was just pressed
		//dbg << "armed \n";
		timer.set(l1->LONG_PRESS_TIME);													// set timer to detect a long
		pom.stayAwake(l1->LONG_PRESS_TIME + 500);											// stay awake to check button status
		button_check.armed = 1;																// set it armed
		return;																				// all done while button was pressed
	}
	if (!button_check.armed) return;														// nothing to do any more, wait till the pin status is changed


	if (button_ref.status == 3) {								// button was just released, keyShortSingle, keyShortDouble, keyLongRelease

		//dbg << "3 lstLng:" << chkRPT.last_long << " dblLng:" << chkRPT.last_dbl_long << " lngRpt:" << chkRPT.last_rpt_long << " lstSht:" << chkRPT.last_short << '\n';
		timer.set(l1->DBL_PRESS_TIME);														// set timer to clear the repeated flags
		pom.stayAwake(l1->DBL_PRESS_TIME + 500);											// stay awake to check button status

		if (button_check.last_long) { 									// keyLongRelease
			button_check.wait_dbl_long = 1;														// waiting for a key long double
			buttonAction(5);

		} else if (button_check.last_short) {								// keyShortDouble
			button_check.last_short = 0;
			buttonAction(2);

		} else if ((!button_check.last_long) && (!button_check.wait_dbl_long)) {// keyShortSingle
			button_check.last_short = 1;															// waiting for a key short double
			buttonAction(1);

		} else if (button_check.last_rpt_long && button_check.wait_dbl_long) {	// keyLongDoubleRelease														
			buttonAction(7);

		}
		button_check.last_long = button_check.last_rpt_long = 0;										// some cleanup


	} else if ((button_ref.status == 0) && (timer.done())) {	// button is still pressed, but timed out, seems to be a long
		//dbg << "0 lstLng:" << chkRPT.last_long << " dblLng:" << chkRPT.last_dbl_long << " lngRpt:" << chkRPT.last_rpt_long << " lstSht:" << chkRPT.last_short << '\n';

		pom.stayAwake(l1->LONG_PRESS_TIME + 500);											// stay awake to check button status

		if (button_check.last_rpt_long  && button_check.wait_dbl_long) {		// detect a repeated double long
			timer.set(l1->LONG_PRESS_TIME);												// set timer to detect next long

		} else if (button_check.last_long) {								// keyLongRepeat
			timer.set(repeatedLong);														// set timer to detect a repeated long
			button_check.last_rpt_long = 1;
			buttonAction(4);																// last key state was a long, now it is a repeated long

		} else if (button_check.wait_dbl_long) {							// keyLongDouble
			timer.set(l1->LONG_PRESS_TIME);												// set timer to detect next long
			button_check.last_rpt_long = 1;														// remember last was long
			buttonAction(6);																// double flag is set, means was the second time for a long													

		} else {													// keyLongSingle
			timer.set(repeatedLong);														// set timer to detect a repeated long
			button_check.last_long = 1;															// remember last was long
			buttonAction(3);																// first time detect a long

		}


	} else if ((button_ref.status == 1) && (timer.done())) {	// button is not pressed for a longer time, clean up
		//if (chkRPT.armed) dbg << "r\n";
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
void cmRemote::buttonAction(uint8_t bEvent) {
	// at the moment this channel module will only work for channel > 0 while key for maintanance channel need
	// some special functionality, like link to toogle and pairing

	pom.stayAwake(1000);																	// overcome the problem of not getting a long repeated key press
	if (bEvent == 255) return;																// was only a wake up message

	DBG(RE, F("RM:buttonAction, cnl: "), lstC.cnl, F(", evt:"), bEvent, '\n');

	if ((bEvent == 3) || (bEvent == 4)) button_info.longpress = 1;							// set the long key flag if requested
	else button_info.longpress = 0;															// otherwise it is a short
	dbg << "x:" << button_info.longpress << '\n';

	//	if (bEvent == 3) hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo);						// send the message
	//	else hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo, AS_ACK_REQ);

	button_info.counter++;																	// increase the button counter
}

