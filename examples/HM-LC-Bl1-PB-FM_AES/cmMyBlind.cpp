//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin cmMyBlind class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include "cmMyBlind.h"

//#define CM_BLIND_DBG

/**
 * @brief Config the blind module
 *
 * @param init(uint8_t):                 pointer to user init function
 * @param updateState(uint8_t, uint8_t): pointer to user updateState function
 */
void cmMyBlind::config(void init(uint8_t), void updateState(uint8_t, uint8_t, uint32_t), uint8_t initialPos) {

	fInit = init;
	fUpdateState = updateState;
	
	// set output pins
	fInit(regCnl);

	// some basic settings for start: set module status to off
	curState = AS_CM_JT_OFF;
	nxtState = AS_CM_JT_OFF;
	l3 = ((s_l3*)&lstPeer);															// initialize l3 with values of short section
	adjTmrStatus = AS_MODULE_TIMER_DONE;
	motorValue = initialPos;
	motorValueOld = initialPos;
	modState = initialPos;
	setState = modState;

	motorState = AS_MODULE_MOTOR_STATE_STOP;
	motorStateOld = AS_MODULE_MOTOR_STATE_STOP;

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
void cmMyBlind::trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *durationTime) {
//	modState = (setValue > 200) ? 200 : setValue;									// setValue max 200

	// some sanity
	delayTmr.set(0);																// also delay timer is not needed any more

//	adjDlyPWM = intTimeCvt(modRampTime);											// get the ramp on time
//	delayTmr.set(adjDlyPWM);														// set the ramp time to poll delay, otherwise we will every time end here
//	adjDlyPWM /= 200;																// break down the ramp time to smaller slices for adjusting PWM


	// TODO: Check if we need for blind actor
//	if (modDurationTime && modState) {												// duration time makes only sense if state is > 0
//		delayTmr.set(0);
//		delayTmr.set(intTimeCvt(modRampTime) + intTimeCvt(modDurationTime));		// poll routine needed when ramp and duration time is finished
//	} else {
//		modRampTime = modDurationTime = 0;											// times not needed any more
//	}
//	dbg << F("RL:trigger11, val:") << modState << F(", rampT:") << intTimeCvt(modRampTime) << F(", duraT:") << intTimeCvt(modDurationTime) << '\n';

	// quick hack
	fUpdateState(1, setValue, modReferenceTimeTopBottom);

}

/**
 * @brief Process events from remote push buttons.
 *
 * @param keyLong:  1 for key pressed long
 * @param keyCount: the key counter. increments on each key release
 */
void cmMyBlind::trigger40(uint8_t keyLong, uint8_t keyCount) {
	// some sanity
	delayTmr.set(0);																// reset delay timer

//	dbg << F("REFERENCE_RUNNING_TIME_BOTTOM_TOP: ") << modReferenceTimeBottomTop << '\n';
//	dbg << F("REFERENCE_RUNNING_TIME_TOP_BOTTOM: ") << modReferenceTimeTopBottom << '\n';

	// check for multi execute flag
	if (keyLong && (!lstPeer.LONG_MULTIEXECUTE) && (fKeyCount == keyCount)) {
		return;																		// trigger was long, but we have no multi execute
	}

	stateMachine_nextStep((keyLong+1), keyCount, 0);
}

void   cmMyBlind::stateMachine_nextStep_disabled(uint8_t keyCode, uint8_t keyCount, uint8_t sensorValue) {
	fKeyCount = keyCount;															// remember message counter

	l3 = (keyCode == AS_CM_KEYCODE_SHORT) ? ((s_l3*)&lstPeer) : ((s_l3*)&lstPeer + 1);	// select short or long register values

	/*
	 * check against action type
	 */
	if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_INACTIVE) {
		// do nothing

	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_JUMP_TO_TARGET) {
		stateMachine_poll(keyCode, keyCount, sensorValue);
		delayTmr.set(0);															// reset timer

	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_TOGGLE_TO_COUNTER) {
		// TODO:
		
	} else if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_TOGGLE_INVERSE_TO_COUNTER) {
		// TODO:

	}
}

// ToDo:
// Quick Hack so the module work in my enviroment
// must be corrected
void   cmMyBlind::stateMachine_nextStep(uint8_t keyCode, uint8_t keyCount, uint8_t sensorValue) {
	fKeyCount = keyCount;															// remember message counter

	l3 = (keyCode == AS_CM_KEYCODE_SHORT) ? ((s_l3*)&lstPeer) : ((s_l3*)&lstPeer + 1);	// select short or long register values

	if (l3->ACTION_TYPE == AS_CM_ACTIONTYPE_JUMP_TO_TARGET) {
		if (l3->JT_ON == AS_CM_JT_ONDELAY) {
			fUpdateState(1, 200, modReferenceTimeTopBottom);
		} else if (l3->JT_OFF == AS_CM_JT_OFFDELAY) {
			fUpdateState(1, 0, modReferenceTimeTopBottom);
		}
	}

}

/**
 * @brief Here we process the state machine
 *
 * TODO: make ready
 */
void   cmMyBlind::stateMachine_poll(uint8_t keyCode, uint8_t keyCount, uint8_t sensorValue) {
	curStateCount = (curStateCount < 255) ? curStateCount++ : 255;

	if (l3->ACTION_TYPE != AS_CM_ACTIONTYPE_JUMP_TO_TARGET) return;							// only valid for jump table

	if (keyCode != AS_CM_KEYCODE_NONE) {
		l3 = (keyCode == AS_CM_KEYCODE_SHORT) ? ((s_l3*)&lstPeer + 1) : ((s_l3*)&lstPeer);	// select short or long register values
		fKeyCount = keyCount;																// remember message counter
	}

	if ( !delayTmr.done() && (keyCode == AS_CM_KEYCODE_NONE) ) {							// check if something is to do
		return;																				// else, return
	}
	/**
	 * Go ahead only if timer not running and no key was pressed
	 */

	if ( (curState == nxtState) && (keyCode == AS_CM_KEYCODE_NONE)) {							// no status change expected
		return;
	}

	#ifdef CM_BLIND_DBG
		debugActionType(l3->ACTION_TYPE);
		dbg << F(", curState: ");
		debugState(curState);
		dbg << F(", nxtState: ");
		debugState(nxtState);

		dbg << '\n';
	#endif

	curState = nxtState;																	// remember the current status
	curStateCount = 0;

	if        (nxtState == AS_CM_JT_OFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("OFF, TIME: ") << l3->OFF_TIME << '\n';
		#endif

		nxtState = l3->JT_OFF;																// get the next status from jump table
		if ( (l3->OFF_TIME) && (l3->OFF_TIME != 0xFF) ) {									// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFF_TIME));										// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_ONDELAY) {
		#ifdef CM_BLIND_DBG
			dbg << F("ONDELAY, TIME: ") << l3->ONDELAY_TIME << '\n';
		#endif

		nxtState = l3->JT_ONDELAY;															// get the next status from jump table
		if (l3->ONDELAY_TIME) {																// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ONDELAY_TIME));									// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_REFON) {
		#ifdef CM_BLIND_DBG
			dbg << F("REFON\n");
		#endif

		nxtState = l3->JT_REFON;															// get next status from jump table
		// TODO: REFON-Running-Time

	} else if (nxtState == AS_CM_JT_RAMPON) {
		#ifdef CM_BLIND_DBG
			dbg << F("RAMPON\n");
		#endif

		nxtState = l3->JT_RAMPON;															// get next status from jump table^
		adjTmr.set(																			// activate the timer for driving motor
			calcAdjTime(motorValue, AS_MODULE_MOTOR_DIRECTION_DOWN, modReferenceTimeTopBottom)
		);


		adjTmrStatus = AS_MODULE_TIMER_ARMED;

	} else if (nxtState == AS_CM_JT_ON) {
		#ifdef CM_BLIND_DBG
			dbg << F("ON, TIME: ") << l3->ON_TIME << '\n';
		#endif

		nxtState = l3->JT_ON;																// get the next status from jump table
		if ( (l3->ON_TIME) && (l3->ON_TIME != 0xFF) ) {										// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->ON_TIME));											// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_OFFDELAY) {
		#ifdef CM_BLIND_DBG
			dbg << F("OFFDELAY, TIME: ") << l3->OFFDELAY_TIME << '\n';
		#endif

		nxtState = l3->JT_OFFDELAY;															// get jump table for next state
		if (l3->OFFDELAY_TIME) {															// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->OFFDELAY_TIME));									// activate the timer and set next status
		}

	} else if (nxtState == AS_CM_JT_REFOFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("REFOFF");
		#endif

		nxtState = l3->JT_REFOFF;															// get jump table for next state

	} else if (nxtState == AS_CM_JT_RAMPOFF) {
		#ifdef CM_BLIND_DBG
			dbg << F("RAMPOFF\n");
		#endif

		nxtState = l3->JT_RAMPOFF;															// get jump table for next state
		adjTmr.set(modReferenceTimeBottomTop);												// activate the timer for driving motor
		adjTmr.set(
			calcAdjTime(motorValue, AS_MODULE_MOTOR_DIRECTION_UP, modReferenceTimeBottomTop)
		);
		adjTmrStatus = AS_MODULE_TIMER_ARMED;
	}
}

uint32_t cmMyBlind::calcAdjTime(uint8_t curValue, uint8_t direction, uint32_t maxTime) {
	uint32_t adjTime = 0;

	adjTime = (uint32_t) (maxTime * curValue / 200);
	if (direction == AS_MODULE_MOTOR_DIRECTION_UP) {
		adjTime = (uint32_t) (maxTime - adjTime);
	}

	return adjTime;
}

uint8_t cmMyBlind::calcMotorValue(uint8_t motorState) {
	uint8_t motorValue = 0;
	uint32_t timeRemain = adjTmr.remain();

	if (timeRemain > 0) {
		if (motorState == AS_MODULE_MOTOR_STATE_DOWN) {
			motorValue = (uint8_t)(200 / (modReferenceTimeTopBottom / timeRemain));

		} else if (motorState == AS_MODULE_MOTOR_STATE_UP) {
			motorValue = (uint8_t) 200 - (200 / (modReferenceTimeBottomTop / timeRemain));

		}
	}

	return motorValue;
}

/**
 * @brief Process events from sensor devices.
 *
 * TODO: make ready
 *
 * @param msgLong:  1 for a long key message
 * @param keyCount: the key counter. increments on each key release
 */
void cmMyBlind::trigger41(uint8_t msgLong, uint8_t keyCount, uint8_t msgVal) {
	uint8_t isLng = (msgLong & 0x40) ? 1 : 0;												// is it a long message?
	uint8_t ctTbl;

	// set short or long
	l3 = (isLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;										// set pointer to the right part of the list3, short or long

//	ctTbl = stateMachine_setNextState(curState);
	
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

		trigger40(isLng, keyCount);
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
uint8_t cmMyBlind::stateMachine_setNextState(uint8_t curState) {
	uint8_t nextState;

	// SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if        (curState == AS_CM_JT_OFF) {
		nextState = l3->JT_OFF;

	} else if (curState == AS_CM_JT_ONDELAY) {
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
	}

	#ifdef CM_BLIND_DBG
		dbg << "curState: " << curState  << ", nextState: " << nextState << '\n';
	#endif

	return nextState;
}

inline void cmMyBlind::updateState(void) {
	if ( (adjTmrStatus == AS_MODULE_TIMER_ARMED) && adjTmr.done() ) {						// Check if timer done and can trigger
		adjTmrStatus = AS_MODULE_TIMER_TRIGGERT;
	}

//	if (!adjTmr.done()) {
	if (motorState != AS_MODULE_MOTOR_STATE_STOP) {
		motorValue = calcMotorValue(motorState);
	}
//	}

	if (motorValue != motorValueOld) {
		#ifdef CM_BLIND_DBG
			dbg << F("motorState: ") << motorState << F(", motorValue: ") << motorValue << '\n';
		#endif
		//	fUpdateState(regCnl, setState);															// set accordingly
		motorValueOld = motorValue;
	}

	/*
	 * Check if curState has changed and "stable" for at least 3 poll loops and the adjTmr not triggert
	 */
	if (oldState == curState && curStateCount < 3 && adjTmrStatus != AS_MODULE_TIMER_TRIGGERT) {
		return;																				// nothing to do
	}

	oldState = curState;

	if (adjTmrStatus == AS_MODULE_TIMER_TRIGGERT) {											// Execution action on timer trigger
		adjTmrStatus = AS_MODULE_TIMER_DONE;
		motorState = AS_MODULE_MOTOR_STATE_STOP;

	} else {
		if (curState == AS_CM_JT_RAMPOFF) {
			motorState = AS_MODULE_MOTOR_STATE_UP;

		} else if (curState == AS_CM_JT_RAMPON) {
			motorState = AS_MODULE_MOTOR_STATE_DOWN;
		}
	}
}

/**
 * @brief Send module state in dependency of modState, setState and delayTmr
 *
 * TODO: make ready
 */
inline void cmMyBlind::sendState(void) {
	uint8_t  extState = AS_CM_EXTSTATE_NONE;										// extended state (down, up, running)

	if (stateToSend == AS_CM_STATETOSEND_NONE) return;								// nothing to do

	if (!msgTmr.done()) return;														// not the right time

	if (modState < setState) {
		extState  = AS_CM_EXTSTATE_UP;

	} else if (modState >  setState) {
		extState  = AS_CM_EXTSTATE_DOWN;
	}

	if (!delayTmr.done()) {															// set running bit if timer running
		extState |= AS_CM_EXTSTATE_RUNNING;
	}
		
	if (stateToSend == AS_CM_STATETOSEND_ACK) {										// Check which type has to be send
		hm->sendACK_STATUS(regCnl, modState, extState);								// send ACK

	} else if (stateToSend == AS_CM_STATETOSEND_STATE) {
		hm->sendINFO_ACTUATOR_STATUS(regCnl, modState, extState);					// send status
	}

	// check if it is a stable status, otherwise schedule next info message
	if (extState >= AS_CM_EXTSTATE_RUNNING) {										// status is currently changing
		stateToSend = AS_CM_STATETOSEND_STATE;										// send next time a info status message
		msgTmr.set(delayTmr.remain() + 5);

	} else if (extState > AS_CM_STATETOSEND_NONE) {
		stateToSend = AS_CM_STATETOSEND_STATE;										// send next time a info status message
		msgTmr.set(msgDelay);

	} else {
		stateToSend = AS_CM_STATETOSEND_NONE;										// no need for next time
	}
}

void cmMyBlind::setSendState(uint8_t state) {
	modState = state;
	setState = state;
	hm->sendINFO_ACTUATOR_STATUS(regCnl, modState, AS_CM_EXTSTATE_NONE);		// send status
}

void cmMyBlind::poll(void) {
	updateState();																	// check if something is to be set on the PWM channel
	sendState();																	// check if there is some status to send

	stateMachine_poll(0, 0, 0);
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
inline void cmMyBlind::setToggle(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("TOGGLE_BLIND\n");
	#endif

	// Todo: Hack. Must be resolved
	fUpdateState(1, 201, modReferenceTimeTopBottom);

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
//	if (curState == AS_CM_JT_ON) {												// currently on, next status should be off
//		nxtState = AS_CM_JT_OFF;

//	} else if (nxtState == AS_CM_JT_OFF) {										// currently off, next status should be on
//		nxtState = AS_CM_JT_ON;
//	}
}

/**
 * @brief it's only for information purpose if channel config was changed (List0/1 or List3/4)
 */
void cmMyBlind::configCngEvent(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("Channel config changed, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif

	modReferenceTimeTopBottom = (uint32_t)GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_TOP_BOTTOM) * 100;
	modReferenceTimeBottomTop = (uint32_t)GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_BOTTOM_TOP) * 100;

	#ifdef CM_BLIND_DBG
		dbg << F("modReferenceTimeTopBottom: ") << modReferenceTimeTopBottom << '\n';
		dbg << F("modReferenceTimeBottomTop: ") << modReferenceTimeBottomTop << '\n';
	#endif

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
 *        SET_LOCK  (Type 0x11, Subtype: 0x01)
 *        LEVEL_SET (Type 0x11, Subtype: 0x02)
 *        STOP      (Type 0x11, Subtype: 0x03)
 *
 *        Typical you will find three bytes in data:
 *           1st byte = value;
 *           2nd and 3rd byte = ramp time;
 *           4th and 5th byte = on duration time;
 *
 *        after setting the new value we have to send an enhanced ACK:
 *           <- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54
 */
void cmMyBlind::pairSetEvent(uint8_t *data, uint8_t len) {
	#ifdef CM_BLIND_DBG
		dbg << F("PSE, value:") << _HEXB(data[0]);
		if (len > 1) { dbg << F(", modRampTime: ") << _HEX((data+1), 2); };
		if (len > 3) { dbg << F(", modDurationTime: ") << _HEX((data+3), 2); };
		dbg << '\n';
	#endif

	trigger11(
		((len > 0) ? data[0] : 255),											// send 255 if stop was sent
		((len > 1) ? data+1 : NULL),											// sent modRampTime if len > 1
		((len > 3) ? data+3 : NULL)												// sent modDurationTime if len > 3
	);

	// we don't need this. ack_status was sent implicit
	// stateToSend = AS_CM_STATETOSEND_ACK;										// ACK should be send
	// msgTmr.set(100);															// give some time
}

/**
 * @brief We received a status request.
 *        Appropriate answer is an InfoActuatorStatus message
 */
inline void cmMyBlind::pairStatusReq(void) {
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
void cmMyBlind::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
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
		//msgTmr.set(100);															// immediately

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
void cmMyBlind::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	hm = instPtr;																	// set pointer to the HM module
	hm->rg.regInAS(
		cnl,
		lst,
		s_mod_dlgt(this,&cmMyBlind::hmEventCol),
		(uint8_t*)&lstCnl,
		(uint8_t*)&lstPeer
	);

	regCnl = cnl;																	// stores the channel we are responsible fore
}

/**
 * @brief HM event controller
 */
void cmMyBlind::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
//	dbg << "hmEventCol >>>>>>> by3:" << by3 << " by10:" << by10 << " d:" << _HEX(data, len) << '\n'; _delay_ms(100);
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x00) && (by10 == 0x02)) firstStart();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);			// set
	else if ((by3 == 0x11) && (by10 == 0x03)) pairSetEvent(data, 0);			// stop
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
void cmMyBlind::peerAddEvent(uint8_t *data, uint8_t len) {
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

inline void cmMyBlind::firstStart(void) {
	#ifdef CM_BLIND_DBG
		dbg << F("firstStart\n");
	#endif
}
