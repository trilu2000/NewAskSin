//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin cmBlind class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include <cmBlind.h>

//#define CM_BLIND_DBG

/**
 * @brief Config the blind module
 *
 * @param init(uint8_t):                 pointer to user init function
 * @param updateState(uint8_t, uint8_t): pointer to user updateState function
 */
void cmBlind::config(void init(uint8_t), void updateState(uint8_t, uint8_t)) {

	fInit = init;
	fUpdateState = updateState;
	
	// set output pins
	fInit(regCnl);

	// some basic settings for start: set module status to off
	curState = AS_CM_JT_OFF;
	nxtState = AS_CM_JT_OFF;
	modState = 0x00;
	
	// send the initial status info
	stateToSend = AS_CM_STATETOSEND_STATE;
	msgTmr.set(msgDelay);
}

/**
 * @brief Process set events from master (central unit).
 *
 * @param setValue:     the new module value
 * @param rampTime:     pointer to ramp time
 * @param durationTime: pointer to duration time
 */
void cmBlind::trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *durationTime) {
	modState = (setValue > 200) ? 200 : setValue;									// setValue max 200

	// some sanity
	delayTmr.set(0);																// also delay timer is not needed any more

//	if (durationTime) {
//		modDurationTime = (uint16_t)durationTime[0]<<8 | (uint16_t)durationTime[1];	// duration time if given
//	} else {
//		modDurationTime = 0;														// or clear value
//	}

	adjDlyPWM = intTimeCvt(modRampTime);											// get the ramp on time
	delayTmr.set(adjDlyPWM);														// set the ramp time to poll delay, otherwise we will every time end here
	adjDlyPWM /= 200;																// break down the ramp time to smaller slices for adjusting PWM


	// TODO: Check if we need for blind actor
//	if (modDurationTime && modState) {												// duration time makes only sense if state is > 0
//		delayTmr.set(0);
//		delayTmr.set(intTimeCvt(modRampTime) + intTimeCvt(modDurationTime));		// poll routine needed when ramp and duration time is finished
//	} else {
//		modRampTime = modDurationTime = 0;											// times not needed any more
//	}
//	dbg << F("RL:trigger11, val:") << modState << F(", rampT:") << intTimeCvt(modRampTime) << F(", duraT:") << intTimeCvt(modDurationTime) << '\n';
}

/**
 * @brief Process events from remote push buttons.
 *
 * @param keyLong:  1 for key pressed long
 * @param msgCount: the key counter. increments on each key release
 */
void cmBlind::trigger40(uint8_t keyLong, uint8_t msgCount) {
	// some sanity
	delayTmr.set(0);																// reset delay timer

	dbg << F("REFERENCE_RUNNING_TIME_BOTTOM_TOP: ") << modReferenceTimeBottomTop << '\n';
	dbg << F("REFERENCE_RUNNING_TIME_TOP_BOTTOM: ") << modReferenceTimeTopBottom << '\n';


	// check for multi execute flag
	if (keyLong && (!lstPeer.LONG_MULTIEXECUTE) && (fMsgCount == msgCount)) {
		return;																		// trigger was long, but we have no multi execute
	}

	fMsgCount = msgCount;															// remember message counter

	l3 = (keyLong) ? ((s_l3*)&lstPeer + 1) : ((s_l3*)&lstPeer);						// set short or long

	/*
	 * check against action type
	 */
	if        (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_INACTIVE) {
		// do nothing

	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_JUMP_TO_TARGET) {
		nxtState = stateMachine_setNextState(curState);
		delayTmr.set(0);															// set timer to 0 for avoiding delays

	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_TOGGLE_TO_COUNTER) {
		l3->ACTION_TYPE = AS_CM_ACTIONTYPE_INACTIVE;

		if (msgCount%2 == 1) {
			modState = 200;
			adjDlyPWM = modReferenceTimeTopBottom / 200;
		} else {
			modState = 0;
			adjDlyPWM = modReferenceTimeBottomTop / 200;
		}
		
	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_TOGGLE_INVERSE_TO_COUNTER) {
		l3->ACTION_TYPE = AS_CM_ACTIONTYPE_INACTIVE;

		if (msgCount%2 == 1) {
			modState = 0;
			adjDlyPWM = modReferenceTimeBottomTop / 200;
		} else {
			modState = 200;
			adjDlyPWM = modReferenceTimeTopBottom / 200;
			
		}
	}

	#ifdef CM_BLIND_DBG
		debugActionType(l3->ACTION_TYPE);
		dbg << F(", curState: ");
		debugState(curState);
		dbg << F(", nxtState: ");
		debugState(nxtState);
		dbg << '\n';
	#endif
}

/**
 * @brief Process events from sensor devices.
 *
 * TODO: make ready
 *
 * @param msgLong:  1 for a long key message
 * @param msgCount: the key counter. increments on each key release
 */
void cmBlind::trigger41(uint8_t msgLong, uint8_t msgCount, uint8_t msgVal) {
	uint8_t isLng = (msgLong & 0x40) ? 1 : 0;												// is it a long message?
	uint8_t ctTbl;

	// set short or long
	l3 = (isLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;										// set pointer to the right part of the list3, short or long

	ctTbl = stateMachine_setNextState(curState);
	
	// X GE COND_VALUE_LO                         - geLo -  > low           - 0
	// X GE COND_VALUE_HI                         - geHi -  > high          - 1
	// X LT COND_VALUE_LO                         - ltLo -  < low           - 2
	// X LT COND_VALUE_HI                         - ltHi -  < high          - 3
	// COND_VALUE_LO LE X LT COND_VALUE_HIGH      - betW -  low <> high     - 4
	// X LT COND_VALUE_LO OR X GE COND_VALUE_HIGH - outS -  < low or > high - 5

	//dbg << "curState: " << curState  << ", isLng: " << isLng << ", val: " << msgVal  << ", cond: " << ctTbl << '\n';

	if ( ((ctTbl == AS_CM_JT_NONE) && (msgVal > l3->COND_VALUE_LO)) ||
	     ((ctTbl == AS_CM_JT_ONDELAY) && (msgVal > l3->COND_VALUE_HI)) ||
	     ((ctTbl == AS_CM_JT_RAMPON) and (msgVal < l3->COND_VALUE_LO)) ||
	     ((ctTbl == AS_CM_JT_ON) && (msgVal < l3->COND_VALUE_HI)) ||
	     ((ctTbl == AS_CM_JT_OFFDELAY) && (msgVal > l3->COND_VALUE_LO) && (msgVal < l3->COND_VALUE_HI)) ||
	     ((ctTbl == AS_CM_JT_RAMPOFF) && (msgVal < l3->COND_VALUE_LO) && (msgVal > l3->COND_VALUE_HI)) ) {

		trigger40(isLng, msgCount);
	}
}

/**
 * @brief set next state in state machine.
 *
 * TODO: Maybe we can do this over a jump table
 *
 * @param curState:   reference to curState
 * @return            the nextState
 */
uint8_t cmBlind::stateMachine_setNextState(uint8_t curState) {
	uint8_t nextState;

	// SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if      (curState == AS_CM_JT_ONDELAY) {
		nextState = l3->JT_ONDELAY;

	} else if (curState == AS_CM_JT_REFON) {
		nextState = l3->JT_REFON;

	} else if (curState == AS_CM_JT_RAMPON) {
		nextState = l3->JT_RAMPON;

	} else if (curState == AS_CM_JT_ON) {
		nextState = l3->JT_ON;

	} else if (curState == AS_CM_JT_OFFDELAY) {
		nextState = l3->JT_OFFDELAY;

	} else if (curState == AS_CM_JT_REFOFF) {
		nextState = l3->JT_REFOFF;

	} else if (curState == AS_CM_JT_RAMPOFF) {
		nextState = l3->JT_RAMPOFF;

	} else if (curState == AS_CM_JT_OFF) {
		nextState = l3->JT_OFF;
	}

	return nextState;
}

inline void cmBlind::updateState(void) {

	// something to do?
	if (setState == modState) return;												// nothing to do

	if (!adjTmr.done()) return;														// timer not done, wait until then
	//dbg << "m" << modState << " s" << setState << '\n';
	
	// calculate next step
	if (modState > setState) {
		setState++;																	// do we have to go up
	} else {
		setState--;																	// or down
	}
	
	fUpdateState(regCnl, setState);													// set accordingly

	adjTmr.set(adjDlyPWM);															// set timer for next action
}

/**
 * @brief Send module state in dependency of modState, setState and delayTmr
 *
 * TODO: make ready
 */
inline void cmBlind::sendState(void) {
	uint8_t  extState = AS_CM_EXTSTATE_NONE;										// extended state (down, up, running)

	if (stateToSend == AS_CM_STATETOSEND_NONE) return;									// nothing to do
	if (!msgTmr.done()) return;														// not the right time
		
	if (modState < setState) {
		extState  = AS_CM_EXTSTATE_UP;

	} else if (modState >  setState) {
		extState  = AS_CM_EXTSTATE_DOWN;
	}

	if (!delayTmr.done()) {															// set running bit if timer running
		extState |= AS_CM_EXTSTATE_RUNNING;
	}
		
	if (stateToSend == AS_CM_STATETOSEND_ACK) {											// Check which type has to be send
		hm->sendACK_STATUS(regCnl, modState, extState);								// send ACK
	} else if (stateToSend == AS_CM_STATETOSEND_STATE) {
		hm->sendINFO_ACTUATOR_STATUS(regCnl, modState, extState);					// send status
	}

	// check if it is a stable status, otherwise schedule next info message
	if (extState >= AS_CM_EXTSTATE_RUNNING) {										// status is currently changing
		stateToSend = AS_CM_STATETOSEND_STATE;											// send next time a info status message
		msgTmr.set(delayTmr.remain() + 5);

	} else if (extState > AS_CM_STATETOSEND_NONE) {
		stateToSend = AS_CM_STATETOSEND_STATE;											// send next time a info status message
		msgTmr.set(msgDelay);

	} else {
		stateToSend = AS_CM_STATETOSEND_NONE;											// no need for next time
	}
}

void cmBlind::poll(void) {
	updateState();																	// check if something is to be set on the PWM channel
	sendState();																	// check if there is some status to send


	/*
	 * Here we process the state machine
	 */

	// check if something is to do
	if (!delayTmr.done() ) return;													// timer not done, wait until then

	// trigger11, check if onTimer was running
//	if (modDurationTime)  {
		modState = 0;
		adjDlyPWM = intTimeCvt(modRampTime);										// get the ramp on time
		adjDlyPWM /= 200;															// break down the ramp time to smaller slices for adjusting PWM
//		modRampTime = modDurationTime = 0;											// no further action required
//	}
	
	// - jump table section, only
	if (l3->ACTION_TYPE != AS_CM_ACTIONTYPE_JUMP_TO_TARGET) return;					// only valid for jump table
	if (curState == nxtState) return;												// no status change expected

	if        (nxtState == AS_CM_JT_ONDELAY) {
		#ifdef CM_BLIND_DBG
			dbg << F("ONDELAY\n");
		#endif 
		
		curState = nxtState;														// remember current status
		nxtState = l3->JT_ONDELAY;													// get next status from jump table
		
		if (l3->ONDELAY_TIME) {														// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ONDELAY_TIME));							// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_REFON) {
		#ifdef CM_BLIND_DBG
			dbg << F("REFON\n");
		#endif

		curState = nxtState;														// remember current status
		nxtState = l3->JT_REFON;													// get next status from jump table

	} else if (nxtState == AS_CM_JT_RAMPON) {
		#ifdef CM_BLIND_DBG
			dbg << F("RAMPON\n");
		#endif
		
		// check modStat against onLevel, if not compare, set the right values
		if (modState != l3->ON_LEVEL)	{											// modStat not set, so first time
			modState	= l3->ON_LEVEL;												// set module status accordingly settings
			adjDlyPWM = modReferenceTimeTopBottom;									// get the ramp on time
			delayTmr.set(adjDlyPWM);												// set the ramp time to poll delay, otherwise we will every time end here
			adjDlyPWM /= 200;														// break down the ramp time to smaller slices for adjusting PWM
		}
		
		// check if ramp on is done, set next status
		if (modState == setState) {													// ramp on is done
			curState = nxtState;													// set current status accordingly
			nxtState = l3->JT_RAMPON;												// set next status accordingly the jump table
		}

	} else if (nxtState == AS_CM_JT_ON) {
		#ifdef CM_BLIND_DBG
			dbg << F("ON\n");
		#endif
		
		curState = nxtState;														// remember current status, when timer not set, we stay here for ever
		
		if ((l3->ON_TIME) && (l3->ON_TIME != 255)) {								// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ON_TIME));									// activate the timer and set next status

			// TODO: jump over off delay?
			nxtState = AS_CM_JT_RAMPOFF;											// go to ramp off
			l3->JT_RAMPOFF = AS_CM_JT_OFF;											// jump from rampOff to off
			l3->JT_OFF = AS_CM_JT_OFF;												// stay in off mode
		}

	} else if (nxtState == AS_CM_JT_OFFDELAY) {
		#ifdef CM_BLIND_DBG
			dbg << F("OFFDELAY\n");
		#endif

		curState = nxtState;														// remember current state
		nxtState = l3->JT_OFFDELAY;													// get jump table for next state

		if (l3->OFFDELAY_TIME) {													// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFFDELAY_TIME));							// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_REFOFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("REFOFF\n");
		#endif

		curState = nxtState;														// remember current state
		nxtState = l3->JT_REFOFF;													// get jump table for next state

	} else if (nxtState == AS_CM_JT_RAMPOFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("RAMPOFF\n");
		#endif
		
		// check modStat against offLevel, if not similar, set the right values
		if (modState != l3->OFF_LEVEL) {											// check for first time and set the correct values
			modState	= l3->OFF_LEVEL;											// set the PWM to the right value
			adjDlyPWM = modReferenceTimeBottomTop;									// get the ramp off time
			delayTmr.set(adjDlyPWM);												// set ramp off time to the poll timer, other wise we will check every ms again
			adjDlyPWM /= 200;														// split PWN timer to slices for smooth dimming
		}
		
		// check if ramp on is done, set next status
		if (modState == setState) {													// check if ramp off is done
			curState = nxtState;													// remember the current status
			nxtState = l3->JT_RAMPOFF;												// get the next status from jump table
		}


	} else if (nxtState == AS_CM_JT_OFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("OFF\n");
		#endif

		curState = nxtState;														// remember the current status
		nxtState = l3->JT_ON;														// get the next status from jump table

		if ((l3->OFF_TIME) && (l3->OFF_TIME != 255)) {								// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFF_TIME));								// activate the timer and set next status
		}
	}

}



/************************************************************************************
 * mandatory functions for every new module to communicate within HM protocol stack *
 ************************************************************************************/

/**
 * @brief SetToggle will be called by config button in mode 2 by a short key press.
 *        Here we can toggle the status of the actor.
 *
 * TODO: Should check against internal device kay config
 */
inline void cmBlind::setToggle(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("TOGGLE_BLIND\n");
	#endif

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (curState == AS_CM_JT_ON) {												// currently on, next status should be off
		nxtState = AS_CM_JT_OFF;

	} else if (nxtState == AS_CM_JT_OFF) {										// currently off, next status should be on
		nxtState = AS_CM_JT_ON;
	}
}

/**
 * @brief it's only for information purpose if channel config was changed (List0/1 or List3/4)
 */
void cmBlind::configCngEvent(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("Channel config changed, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif

	modReferenceTimeTopBottom = GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_TOP_BOTTOM);
	modReferenceTimeBottomTop = GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_BOTTOM_TOP);

	msgDelay = lstCnl.STATUSINFO_MINDELAY * 500;									// get message delay

	if (lstCnl.STATUSINFO_RANDOM) {
		// maybe we outsource this call to external module
		hm->initPseudoRandomNumberGenerator();
		msgDelay += (rand()%(uint16_t)(lstCnl.STATUSINFO_RANDOM * 1000));
	}

	if (!msgDelay) {
		msgDelay = 2000;
	}
}

/**
 * @brief We received a message from master to set a new value.
 *        Typical you will find three bytes in data:
 *           1st byte = value;
 *           2nd and 3rd byte = ramp time;
 *           4th and 5th byte = on duration time;
 *
 *        after setting the new value we have to send an enhanced ACK:
 *           <- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54
 */
void cmBlind::pairSetEvent(uint8_t *data, uint8_t len) {
	#ifdef CM_BLIND_DBG
		dbg << F("PSE, value:") << _HEXB(data[0]);
		if (len > 1) { dbg << F(", modRampTime: ") << _HEX((data+1), 2); };
		if (len > 3) { dbg << F(", modDurationTime: ") << _HEX((data+3), 2); };
		dbg << '\n';
	#endif

	trigger11(data[0], ((len > 1) ? data+1 : NULL), ((len > 3) ? data+3 : NULL));

	stateToSend = AS_CM_STATETOSEND_ACK;											// ACK should be send
	msgTmr.set(100);																// give some time
}

/**
 * @brief We received a status request.
 *        Appropriate answer is an InfoActuatorStatus message
 */
inline void cmBlind::pairStatusReq(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("statusRequest\n");
	#endif
	
	stateToSend = AS_CM_STATETOSEND_STATE;											// ACK should be send
	msgTmr.set(0);																	// immediately
}

/**
 * @brief We received a peer event.
 *       In type you will find the type of peer event.
 *          0x3E = switch
 *          0x40 = remote
 *          0x41 = sensor
 *
 *       Appropriate answer is an ACK
 */
void cmBlind::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	#ifdef CM_BLIND_DBG
		dbg << F("peer message type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
	if ( (type == 0x40) || (type == 0x41) ) {
		if (type == 0x40) {
			trigger40( ((data[0] & 0x40) ? 1 : 0), data[1]);							// filter out the long message bit

		} else if (type == 0x41) {
			trigger41( (data[0] & 0x7F), data[1], data[2]);
		}

		stateToSend = AS_CM_STATETOSEND_ACK;										// ACK should be send
		msgTmr.set(100);															// immediately

	} else {
		hm->sendACK();
	}
}



/**********************************
 * predefined, no reason to touch *
 **********************************/

/**
 * @brief Register module in HM
 */
void cmBlind::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	hm = instPtr;																	// set pointer to the HM module
	hm->rg.regInAS(
		cnl,
		lst,
		s_mod_dlgt(this,&cmBlind::hmEventCol),
		(uint8_t*)&lstCnl,
		(uint8_t*)&lstPeer
	);

	regCnl = cnl;																	// stores the channel we are responsible fore
}

/**
 * @brief HM event controller
 */
void cmBlind::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
//	dbg << "hmEventCol >>>>>>> by3:" << by3 << " by10:" << by10 << " d:" << _HEX(data, len) << '\n';
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x00) && (by10 == 0x02)) firstStart();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}

/**
 * @brief We received an peer add event, which means, there was a peer added in this respective channel.
 *           1st byte and 2nd byte shows the peer channel,
 *           3rd and 4th byte gives the peer index
 *
 *        No need for sending an answer, but we could set default data to the respective list3/4
 */
void cmBlind::peerAddEvent(uint8_t *data, uint8_t len) {
	#ifdef CM_BLIND_DBG
		dbg << F("peerAddEvent: peerCh1: ") << _HEXB(data[0]) << F(", peerCh2: ") << _HEXB(data[1]) << F(", peerIndex1: ") << _HEXB(data[2]) << F(", peerIndex2: ") << _HEXB(data[3]) << '\n';
	#endif
	
	if ((data[0]) && (data[1])) {															// dual peer add
		if (data[0]%2) {																	// odd
			hm->ee.setList(regCnl, 3, data[2], (uint8_t*)peerOdd);
			hm->ee.setList(regCnl, 3, data[3], (uint8_t*)peerEven);
		} else {																			// even
			hm->ee.setList(regCnl, 3, data[3], (uint8_t*)peerOdd);
			hm->ee.setList(regCnl, 3, data[2], (uint8_t*)peerEven);
		}

	} else {																				// single peer add
		if (data[0]) hm->ee.setList(regCnl, 3, data[3], (uint8_t*)peerSingle);
		if (data[1]) hm->ee.setList(regCnl, 3, data[4], (uint8_t*)peerSingle);
	}
}

inline void cmBlind::firstStart(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("firstStart\n");
	#endif
}
