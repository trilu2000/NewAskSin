//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin remote class ---------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define RM_DBG																				// debug message flag
#include "cmRemote.h"

waitTimer cmrTmr;																			// button timer functionality

//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
/**
* @brief This function has to be called to handover all pin information to initialize the hardware port/pin
* and to register the pin change interrupt. The channel module will remember on the pin information and poll the
* pin status. If a change/interrupt is recognized, the module will act accordingly.
* There is a more passive option to use this module. Helpful if you don't use a dedicated pin per channel.
* In such a case you can call the buttonAction() directly from user sketch.
* @param   port/pin information    PIN_C2
*/
void cmRemote::config(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC) {
	registerPCINT(PINBIT, DDREG, PORTREG, PINREG, PCINR, PCIBYTE, PCICREG, PCIMASK, PCIEREG, VEC);// prepare hardware and register interrupt
	port = VEC;																					// port information for checking interrupt
	pin = PINBIT;																				// pin information for checking interrupt
	chkRPT.is_configured = 1;																	// poll the pin make only sense if it was configured, store result here
	btn = checkPCINT(port, pin, 0);																// get the latest information
}

/**
* @brief Sending out an Asksin message by the given action. Function can be called from user sketch directly.
* Intend is, not every remote channel needs an hardware pin anymore. Looking for further projects like a 
* wall mount toch switch.
* @param   bEvent   0 - short key press
*                   1 - double short key press
*                   2 - long key press
*                   3 - repeated long key press
*                   4 - end of long key press
*                   5 - double long key press
*                   6 - end of double long key press
*                 255 - key press, for stay awake issues
*/
void cmRemote::buttonAction(uint8_t bEvent) {

	hm->pw.stayAwake(1000);																		// overcome the problem of not getting a long repeated key press
	if (bEvent == 255) return;																	// was only a wake up message

	#ifdef RM_DBG																				// some debug message
	dbg << F("RM buttonAction, cnl: ") << regCnl << ", s:" << bEvent << '\n';
	#endif

	// at the moment this channel module will only work for channel > 0 while key for maintanance channel need
	// some special functionality, like link to toogle and pairing

	if ((bEvent >= 2) && (bEvent <= 3)) buttonInfo.longpress = 1;								// set the long key flag if requested
	else buttonInfo.longpress = 0;																// otherwise it is a short
	
//	if (bEvent == 3) hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo);								// send the message
//	else hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo, AS_ACK_REQ);

	// not sure if there is a need for a call back function
	//if (callBack) callBack(regCnl, bEvent);													// call the callback function

	buttonInfo.counter++;																		// increase the button counter
}

void cmRemote::buttonPoll(void) {

	#define detectLong      3000																// has to be replaced by list1 content
	#define repeatedLong    300
	#define timeoutDouble   1000


	if (!chkRPT.is_configured) return;															// if port is not configured, poll makes no sense
	btn = checkPCINT(port, pin, 1);																// check if an interrupt had happened


	if (btn == 2) {													// button was just pressed
		//dbg << "armed \n";
		cmrTmr.set(detectLong);																	// set timer to detect a long
		hm->pw.stayAwake(detectLong + 500);														// stay awake to check button status
		chkRPT.armed = 1;																		// set it armed
		return;																					// all done while button was pressed
	}
	if (!chkRPT.armed) return;																	// nothing to do any more, wait till the pin status is changed


	if (btn == 3) {													// button was just released, keyShortSingle, keyShortDouble, keyLongRelease

		//dbg << "3 lstLng:" << lstLng << " dblLng:" << dblLng << " lngRpt:" << lngRpt << " lstSht:" << lstSht << '\n';
		cmrTmr.set(timeoutDouble);																// set timer to clear the repeated flags
		hm->pw.stayAwake(timeoutDouble + 500);													// stay awake to check button status

		if ((chkRPT.last_long) && (chkRPT.last_rpt_long)) {										// keyLongRelease
				buttonAction(5);

		} else if (chkRPT.last_long) {															// no action, only remember for a double
			chkRPT.last_dbl_long = 1;															// waiting for a key long double

		} else if (chkRPT.last_short) {															// keyShortDouble
			chkRPT.last_short = 0;
			buttonAction(2);

		} else if ((!chkRPT.last_long) && (!chkRPT.last_dbl_long)) {							// keyShortSingle
			chkRPT.last_short = 1;																// waiting for a key short double
			buttonAction(1);

		}
		chkRPT.last_long = chkRPT.last_rpt_long = 0;											// some cleanup


	} else if ((btn == 0) && (cmrTmr.done())) {						// button is still pressed, but timed out, seems to be a long
		//dbg << "0 lstLng:" << lstLng << " dblLng:" << dblLng << " lngRpt:" << lngRpt << " lstSht:" << lstSht << '\n';

		hm->pw.stayAwake(detectLong + 500);														// stay awake to check button status

		if (chkRPT.last_long) {																	// keyLongRepeat
			cmrTmr.set(repeatedLong);															// set timer to detect a repeated long
			chkRPT.last_rpt_long = 1;
			buttonAction(4);																	// last key state was a long, now it is a repeated long

		} else if (chkRPT.last_dbl_long) {														// keyLongDouble
			cmrTmr.set(detectLong);																// set timer to detect next long
			buttonAction(6);																	// double flag is set, means was the second time for a long													

		} else {																				// keyLongSingle
			cmrTmr.set(detectLong);																// set timer to detect a repeated long
			chkRPT.last_long = 1;																// remember last was long
			buttonAction(3);																	// first time detect a long

		}


	} else if ((btn == 1) && (cmrTmr.done())) {						// button is not pressed for a longer time, check if the double flags timed out
		//if (armFlg) dbg << "r\n";
		chkRPT.armed = chkRPT.last_dbl_long = chkRPT.last_long = chkRPT.last_rpt_long = 0;

	}
}


//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void cmRemote::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef RM_DBG																				// some debug message
	dbg << F("RM setToggle\n");
	#endif
}

void cmRemote::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef RM_DBG																				// some debug message
	dbg << F("RM configCngEvent, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif
}

void cmRemote::pairSetEvent(uint8_t *data, uint8_t len) {
	// not sure if it will happen while beeing a remote channel
	#ifdef RM_DBG																				// some debug message
	dbg << F("RM pairSetEvent, value:") << _HEXB(data[0]) << '\n';
	#endif
}

void cmRemote::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef RM_DBG																				// some debug message
	dbg << F("RM pairStatusReq\n");
	#endif
}

void cmRemote::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef RM_DBG																				// some debug message
	dbg << F("RM peerMsgEvent, type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
	//if (type == 0x3E) trigger40( data[4], data[5]);
	//if (type == 0x40) trigger40((data[0] & 0x40), data[1]);
	//if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);
	
	hm->sendACK();
}

void cmRemote::poll(void) {
	buttonPoll();																				// jump into fuction to poll the pin status
}


//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
/**
* @brief Standard function for all user module to register itself into the channel module table.
* 
* @param    cnl       Indicates the device channel, where the module should work for
* @param    lst       Is it a list3 or list4 device (options 3 and 4 only)
* @param   *instPtr   Pointer to the main instance of AS class
*/
void cmRemote::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	
	#ifdef RM_DBG																			// some debug message
	dbg << F("RM regInHM, cnl: ") << cnl << F(" , lst: ") << lst << '\n';
	#endif

	hm = instPtr;																			// set pointer to the HM module
	hm->rg.regInAS(cnl, lst, s_mod_dlgt(this,&cmRemote::hmEventCol), (uint8_t*)&lstCnl,(uint8_t*)&lstPeer);
	regCnl = cnl;																			// stores the channel we are responsible fore
	buttonInfo.channel = cnl;																// remembers the channel number
}

/**
* @brief Messaging channel between AS main class and user modules. All information related to the
* registered channel are forwarded via this function into the channel module.
* @param   by3, by10, by11   As from AS main class received...
* @param   *data             Pointer to a byte array with the message
* @param   len               Len of the byte array
*/
void cmRemote::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {

	#ifdef RM_DBG																			// some debug message
	//dbg << F("RM hmEventCol, by3: ") << by3 << F(" , by10: ") << by10 << F(" , data: ") << _HEX(data, len) << '\n';
	#endif

	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}

/**
* @brief Will be called by AS main class only. Signals that the channel was added as a peer to another device.
* Here we can store default settings per peer like a list3 or list4. If it is a list3 device, we have to differentiate
* if only one peer channel were added or two. Example is a switch - if there is only one peer channel added, we assume a 
* toogle setup. If two peer channels were added, one channel is for off, the other is for on...
*/
void cmRemote::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4

	#ifdef RM_DBG																			// some debug message
	dbg << F("RM peerAddEvent: pCnl1: ") << _HEXB(data[0]) << F(", pCnl2: ") << _HEXB(data[1]) << F(", pIdx1: ") << _HEXB(data[2]) << F(", pIdx2: ") << _HEXB(data[3]) << '\n';
	#endif
	
	//if ((data[0]) && (data[1])) {															// dual peer add
	//	if (data[0]%2) {																	// odd
	//		hm->ee.setList(regCnl, 3, data[2], (uint8_t*)peerOdd);
	//		hm->ee.setList(regCnl, 3, data[3], (uint8_t*)peerEven);
	//	} else {																			// even
	//		hm->ee.setList(regCnl, 3, data[3], (uint8_t*)peerOdd);
	//		hm->ee.setList(regCnl, 3, data[2], (uint8_t*)peerEven);
	//	}
	//} else {																				// single peer add
		if (data[0]) hm->ee.setList(regCnl, 4, data[3], (uint8_t*)peerSingle);
		if (data[1]) hm->ee.setList(regCnl, 4, data[4], (uint8_t*)peerSingle);
	//}
}
