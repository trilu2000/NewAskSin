//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin relay class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define RM_DBG																				// debug message flag
#include "cmRemote.h"

//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
void cmRemote::buttonAction(uint8_t bEvent) {
	// possible events of this function:
	//   0 - short key press
	//   1 - double short key press        - not needed yet
	//   2 - long key press
	//   3 - repeated long key press
	//   4 - end of long key press         - not needed yet
	//   5 - double long key press         - not needed yet
	//   6 - end of double long key press  - not needed yet
	//
	// 255 - key press, for stay awake issues

	hm->pw.stayAwake(1000);																	// overcome the problem of not getting a long repeated key press
	if (bEvent == 255) return;																// was only a wake up message

	#ifdef RM_DBG																			// some debug message
		dbg << F("RM buttonAction, cnl: ") << regCnl << ", s:" << bEvent << '\n';
	#endif

	// at the moment this channel module will only work for channel > 0 while key for maintanance channel need
	// some special functionality, like link to toogle and pairing

	if ((bEvent >= 2) && (bEvent <= 3)) buttonInfo.longpress = 1;							// set the long key flag if requested
	else buttonInfo.longpress = 0;															// otherwise it is a short
	
	if (bEvent == 3) hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo);							// send the message
	else hm->sendREMOTE(regCnl, (uint8_t*)&buttonInfo, AS_ACK_REQ);

	// not sure if there is a need for a call back function
	//if (callBack) callBack(regCnl, bEvent);												// call the callback function

	buttonInfo.counter++;																	// increase the button counter
}




//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void cmRemote::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM setToggle\n");
	#endif
	
}

void cmRemote::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM configCngEvent, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif

}

void cmRemote::pairSetEvent(uint8_t *data, uint8_t len) {
	// not sure if it will happen while beeing a remote channel
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM pairSetEvent, value:") << _HEXB(data[0]) << '\n';
	#endif

}

void cmRemote::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM pairStatusReq\n");
	#endif
	
}

void cmRemote::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM peerMsgEvent, type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
	//if (type == 0x3E) trigger40( data[4], data[5]);
	//if (type == 0x40) trigger40((data[0] & 0x40), data[1]);
	//if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);
	
	hm->sendACK();
}

void cmRemote::poll(void) {
	// possible events of this function:
	//   0 - short key press
	//   1 - double short key press        - not needed yet
	//   2 - long key press
	//   3 - repeated long key press
	//   4 - end of long key press         - not needed yet
	//   5 - double long key press         - not needed yet
	//   6 - end of double long key press  - not needed yet
	//
	// 255 - key press, for stay awake issues


	// this flag is set to 0 if stat_curr and stat_last are set to key released and all timers are done
	// it will be set to 1 from outside this function if there is a change on stat_curr done
	if (check_repeat.poll == 0) return;															// check if we have to poll


	// we use several timer in this function, they are set by lstCnl.LONG_PRESS_TIME, lstCnl.DBL_PRESS_TIME, timers are used to detect a 
	// double short (if this timer is set, we should send out the double short message only, short key press are not signalized 
	// long key press or double long
	// some time between two longs to detect a double long
	// some time between repeated long messages
	
	//	dbg << "cFlag: " << cFlag << ", cTime: " << cTime << ", cStat: " << cStat << ", lStat: " << lStat << ", dblLo: " << dblLo << ", lngKeyTme: " << lngKeyTme << ", kTime: " << kTime << ", millis(): " << millis() << '\n';



	if ((stat_curr == 1) && (stat_last == 1)) {													// could be only a timeout
		// button is released and was released before, so it could be only a timeout for a
		// double short key press (1) or double long key press (5) to recognize if the button was pressed again in the given time  
		
		if ((check_repeat.last_long) && (wt_dbl_long.done() )) {								// timeout for double long reached

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR timeout double long\n");											// ...and some information
			#endif

			memset((void*)&check_repeat, 0, 1);													// no need for further check, while timeout

		} else if ((check_repeat.last_short) && (wt_dbl_short.done() )) {						// timeout for double short reached

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR timeout double short\n");											// ...and some information
			#endif

			memset((void*)&check_repeat, 0, 1);													// no need for further check, while timeout
		}
		#ifdef RM_DBG																			// some debug message
			dbg << F("CR error, we forgot to reset the poll flag\n");							// ...and some information
		#endif


	} else if ((stat_curr == 1) && (stat_last == 0)) {											// key release
		// coming from a short or long key press, end of long key press by checking against check_repeat.rpt_long
		// could be a short - signal it and start the timer to detect a double short
		// could be a double short, nothing to do any more
		// could be the end of a long, double long or repeated long

		if ((!check_repeat.last_short) && (!check_repeat.last_long) && (!check_repeat.last_dbl_long)) {	// no short or long was set before, so we have detected a short keypress

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR short\n");															// ...and some information
			#endif
			
			buttonAction(0);																	// send a short key press
			check_repeat.last_short = 1;														// the next one could be a only a double short, so the the respective flag
			wt_dbl_short.set(lstCnl.DBL_PRESS_TIME);											// set the respective timer to detect a double short keypress

		} else if (check_repeat.last_short) {													// double short was set while last key press was detected as short

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR double short\n");													// ...and some information
			#endif

			buttonAction(1);																	// send double short key press as message
			memset((void*)&check_repeat, 0, 1);													// detect a double short, cleanup all repeat variables

		} else if ((check_repeat.last_rpt_long) || (check_repeat.last_long)) {					// coming from a long key press or a repeated long, next could be a double long

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR end of long\n");													// ...and some information
			#endif
			
			buttonAction(4);																	// send end of long key press
			check_repeat.last_rpt_long = 0;														// key was released, so could not be repeated any more
			wt_dbl_long.set(TIMEOUT_DBL_LONG);													// set the timer to detect a double long

		} else if (check_repeat.last_dbl_long) {												// we are coming from a double long key press, signaling was done while key was pressed

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR end of a double long\n");											// ...and some information
			#endif

			buttonAction(6);																	// send end of double long key press
			memset((void*)&check_repeat, 0, 1);													// detect end of double long, cleanup all repeat variables

		}



	} else if ((stat_curr == 0) && (stat_last == 1)) {											// detect a key press
		// key is pressed just now, set timer to detect a long key press
		// short is detected while releasing the button
		// repeated long is detected after the key press was seen as long

		wt_long_press.set(lstCnl.LONG_PRESS_TIME);												// set next timeout
		buttonAction(255);																		// prevent from sleep



	} else if ((stat_curr == 0) && (stat_last == 0)) {											// key is still pressed
		// key is still pressed, options are - 
		// long key press, check against long keypress timer and rpt_long flag
		// repeated long, flag and timer was set while a long was detected before
		// double long, same as long key press, but dbl_long flag is set

		if ((!check_repeat.last_rpt_long) && (!check_repeat.last_long) && (wt_long_press.done())) {	// seems to be the first time we have detected a long

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR long key press\n");												// ...and some information
			#endif

			buttonAction(2);																	// send long key press
				
			check_repeat.last_long = 1;															// next time it will be a repeated long
			wt_rpt_long.set(TIMEOUT_REPEATED_LONG);												// set the timeout to check if it is a repeated long

		} else if ((check_repeat.last_long) && (wt_rpt_long.done())) {							// repeated long detect

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR repeated long\n");													// ...and some information
			#endif

			buttonAction(3);																	// repeated long key press
			check_repeat.last_rpt_long = 1;														// next time it will be a repeated long again
			wt_rpt_long.set(TIMEOUT_REPEATED_LONG);												// set the timeout to check again

		} else if ((check_repeat.last_long) && (!check_repeat.last_rpt_long) && (wt_dbl_long.done())) {	// double Long detected
			// last_rpt_long was cleared in key release function

			#ifdef RM_DBG																		// some debug message
				dbg << F("CR double long\n");													// ...and some information
			#endif

			buttonAction(5);																	// double long key press
			check_repeat.last_long = 0;															// clear flag while detected a double long
			check_repeat.last_dbl_long = 1;														// set the double long flag

		}
	}

	stat_last = stat_curr;																	// remember last key state

}


//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
void cmRemote::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	
	#ifdef RM_DBG																			// some debug message
		dbg << F("RM regInHM, cnl: ") << cnl << F(" , lst: ") << lst << '\n';
	#endif

	hm = instPtr;																			// set pointer to the HM module
	hm->rg.regInAS(cnl, lst, s_mod_dlgt(this,&cmRemote::hmEventCol), (uint8_t*)&lstCnl,(uint8_t*)&lstPeer);
	regCnl = cnl;																			// stores the channel we are responsible fore
	buttonInfo.channel = cnl;																// remembers the channel number
}

void cmRemote::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {

	#ifdef RM_DBG																			// some debug message
		//dbg << F("RM hmEventCol, by3: ") << by3 << F(" , by10: ") << by10 << F(" , data: ") << _HEX(data, len) << '\n'; _delay_ms(100);
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
