//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin dimmer class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define DI_DBG																			// debug message flag
#include "cmDimmer.h"


//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
void cmDimmer::config(void Init(uint8_t), void xSwitch(uint8_t, uint8_t,uint8_t), uint8_t *temperature) {

	fInit = Init;
	fSwitch = xSwitch;
	if (temperature) pTemp = temperature;
	
	// set output pins
	fInit(regCnl);

	// some basic settings for start
	curStat = nxtStat = 6;																	// set relay status to off
	modStat = 0x00;
	
	// send the initial status info
	sendStat = 2;
	msgTmr.set(msgDelay);
}

void cmDimmer::trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *duraTime) {
	
	// some sanity
	activeOffDlyBlink = 0;																	// got a new key press, off delay blink is not needed
	delayTmr.set(0);																		// also delay timer is not needed any more

	// m> 10 88 B0 11 63 19 63 01 02 04 02 01 C8 00 00 7D 00
	// m> 0E 86 B0 11 63 19 63 01 02 04 02 01 C8 00 00
	if (rampTime) rampTme = (uint16_t)rampTime[0]<<8 | (uint16_t)rampTime[1];				// if ramp time is given, bring it in the right format
	else rampTme = 0;																		// otherwise empty variable

	if (duraTime) duraTme = (uint16_t)duraTime[0]<<8 | (uint16_t)duraTime[1];				// duration time if given
	else duraTme = 0;																		// or clear value

	// set value in modStat and ramp time if given
	if (setValue > 200) setValue = 200;
	modStat = setValue;

	adjDlyPWM = intTimeCvt(rampTme);														// get the ramp on time
	delayTmr.set(adjDlyPWM);																// set the ramp time to poll delay, otherwise we will every time end here
	adjDlyPWM /= 200;																		// break down the ramp time to smaller slices for adjusting PWM

	if ((duraTme) && (setValue)) {															// duration time makes only sense if the led is on, default after duration is led off
		delayTmr.set(0);
		delayTmr.set(intTimeCvt(rampTme) + intTimeCvt(duraTme));							// poll routine needed when ramp and duration time is finished

	} else {
		rampTme = duraTme = 0;																// times not needed any more

	}
	//dbg << F("RL:trigger11, val:") << setValue << F(", rampT:") << intTimeCvt(rampTme) << F(", duraT:") << intTimeCvt(duraTme) << '\n';
}
void cmDimmer::trigger40(uint8_t msgLng, uint8_t msgCnt) {

	// some sanity
	activeOffDlyBlink = 0;																	// got a new key press, off delay blink is not needed
	delayTmr.set(0);																		// also delay timer is not needed any more
	
	// check for multi execute flag
	if (( msgLng) && (!lstPeer.lgMultiExec) && (cnt == msgCnt)) return;						// trigger was long, but we have no multi execute
	cnt = msgCnt;																			// remember message counter

	// set short or long
	l3 = (msgLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;


	// check against action type	
	if        (l3->actionType == 0) {		// off

	} else if (l3->actionType == 1) {		// jmpToTarget

		// SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
		if      (curStat == 1) nxtStat = l3->jtDlyOn;										// delay on
		else if (curStat == 2) nxtStat = l3->jtRampOn;										// ramp on
		else if (curStat == 3) nxtStat = l3->jtOn;											// on
		else if (curStat == 4) nxtStat = l3->jtDlyOff;										// delay off
		else if (curStat == 5) nxtStat = l3->jtRampOff;										// ramp off
		else if (curStat == 6) nxtStat = l3->jtOff;											// currently off
		delayTmr.set(0);																	// set timer to 0 for avoiding delays

	} else if (l3->actionType == 2) {		// toogleToCounter
		l3->actionType = 0;

		if (msgCnt%2) {
			modStat = 200;
			adjDlyPWM = byteTimeCvt(l3->rampOnTime) / 200;
			
		} else {
			modStat = 0;
			adjDlyPWM = byteTimeCvt(l3->rampOffTime) / 200;
			
		}
		
	} else if (l3->actionType == 3) {		// toogleInversToCounter
		l3->actionType = 0;

		if (msgCnt%2) {
			modStat = 0;
			adjDlyPWM = byteTimeCvt(l3->rampOffTime) / 200;
			
			} else {
			modStat = 200;
			adjDlyPWM = byteTimeCvt(l3->rampOnTime) / 200;
			
		}


	} else if (l3->actionType == 4) {		// upDim
		upDim();

		if ((l3->onTime) && (l3->onTime != 255)) {											// check onTimer, if set then set jump table accordingly
			delayTmr.set(byteTimeCvt(l3->onTime) );											// set the timer
		} else l3->actionType = 0;


	} else if (l3->actionType == 5) {		// downDim
		downDim();
		l3->actionType = 0;

		
	} else if (l3->actionType == 6) {		// toogleDim
		toggleDim();																		// jump into toggle dim function, direction is given there
		l3->actionType = 0;																	// no further action required


	} else if (l3->actionType == 7) {		// toogleDimToCounter
		// check for even or odd, if even set onMinLvl 
		if (msgCnt % 2) {																	// odd - dimmer on

			if (modStat < l3->onMinLevel) {													// minimum level already set?
				modStat = l3->onMinLevel;													// if not, set to minimum level
				adjDlyPWM = byteTimeCvt(l3->rampOnTime);									// take rampTime in consideration
				adjDlyPWM /= (modStat - l3->onMinLevel);									// divide adjustment time in small slices
			}																				// to dim smooth

		} else {																			// even - dimmer off
			
			if (modStat > l3->offLevel) {													// check if dimmer is already off
				modStat = l3->offLevel;														// if not, set to off level
				adjDlyPWM = byteTimeCvt(l3->rampOffTime);									// take rampOff Time in consideration
				adjDlyPWM /= (modStat - l3->offLevel);										// divide it to small slices to dim smoothly
			}
			
		}

		
	} else if (l3->actionType == 8) {		// toogleDimInversToCounter
		// same as 7, but changed order
		if (msgCnt % 2) {																	// odd - dimmer off

			if (modStat > l3->offLevel) {													// see action type 7, even
				modStat = l3->offLevel;
				adjDlyPWM = byteTimeCvt(l3->rampOffTime);
				adjDlyPWM /= (modStat - l3->offLevel);
			}

		} else {																			// even - dimmer on
			
			if (modStat < l3->onMinLevel) {													// see action type 7, odd
				modStat = l3->onMinLevel;
				adjDlyPWM = byteTimeCvt(l3->rampOnTime);
				adjDlyPWM /= (modStat - l3->onMinLevel);
			}
			
		}

	}

	//showStruct();																			// some debug messages
	//dbg << "a: " << l3->actionType << ", c: " << curStat << ", n: " << nxtStat << '\n';	// some debug again
}
void cmDimmer::trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal) {

	uint8_t isLng = (msgBLL & 0x40)?1:0;													// is it a long message?
	uint8_t ctTbl;

	// set short or long
	l3 = (isLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;										// set pointer to the right part of the list3, short or long

	
	// SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if      (curStat == 1) ctTbl = l3->ctDlyOn;												// delay on
	else if (curStat == 2) ctTbl = l3->ctRampOn;											// ramp on
	else if (curStat == 3) ctTbl = l3->ctOn;												// on
	else if (curStat == 4) ctTbl = l3->ctDlyOff;											// delay off
	else if (curStat == 5) ctTbl = l3->ctRampOff;											// ramp off
	else if (curStat == 6) ctTbl = l3->ctOff;												// currently off
	
	// X GE COND_VALUE_LO                         - geLo -  > low           - 0
	// X GE COND_VALUE_HI                         - geHi -  > high          - 1
	// X LT COND_VALUE_LO                         - ltLo -  < low           - 2
	// X LT COND_VALUE_HI                         - ltHi -  < high          - 3
	// COND_VALUE_LO LE X LT COND_VALUE_HIGH      - betW -  low <> high     - 4
	// X LT COND_VALUE_LO OR X GE COND_VALUE_HIGH - outS -  < low or > high - 5

	//dbg << "curStat: " << curStat  << ", isLng: " << isLng << ", val: " << msgVal  << ", cond: " << ctTbl << '\n';

	if      (ctTbl == 0) if (msgVal > l3->ctValLo) trigger40(isLng, msgCnt);
	else if (ctTbl == 1) if (msgVal > l3->ctValHi) trigger40(isLng, msgCnt);
	else if (ctTbl == 2) if (msgVal < l3->ctValLo) trigger40(isLng, msgCnt);
	else if (ctTbl == 3) if (msgVal < l3->ctValHi) trigger40(isLng, msgCnt);
	else if (ctTbl == 4) if ((msgVal > l3->ctValLo) && (msgVal < l3->ctValHi)) trigger40(isLng, msgCnt);
	else if (ctTbl == 5) if ((msgVal < l3->ctValLo) && (msgVal > l3->ctValHi)) trigger40(isLng, msgCnt);

}

void cmDimmer::toggleDim(void) {
	if (modStat == 0)   directionDim = 1;													// remember the direction , down or up
	if (modStat == 200) directionDim = 0;
	
	if (directionDim) upDim();																// jump into the right function based on the direction
	else downDim();
}
void cmDimmer::upDim(void) {

	// calculate the value
	if (modStat >= 200) return;																// reached or above max value, nothing to do
	modStat += l3->dimStep;																	// increase by dim steps
	if (modStat > 200) modStat = 200;														// more then 100%, go back to 100%
	curStat = 3;																			// dimmer is on, important for on button setting
	
	// new value will be set by polling function, time for increase has to be set manually
	adjDlyPWM = 1;																			// do the adjustment in 1ms steps
}
void cmDimmer::downDim(void) {

	// calculate the value
	if (modStat == 0) return;																// dimmer already off
	if (modStat <= l3->dimStep) {															// dimmer value smaller then dim step
		modStat = l3->dimStep;																// set to minimum dim step
		curStat = 6;																		// set status to off, important for one button settings
	}
	modStat -= l3->dimStep;																	// lower the value
	
	// new value will be set by polling function, time for increase has to be set manually
	adjDlyPWM = 1;																			// do the adjustment in 1ms steps
}

void cmDimmer::adjPWM(void) {

	// something to do?
	if (setStat == modStat) return;															// nothing to do
	if (!adjTmr.done()) return;																// timer not done, wait until then
	//dbg << "m" << modStat << " s" << setStat << '\n';
	
	// calculate next step
	if (modStat > setStat) setStat++;														// do we have to go up
	else setStat--;																			// or down
	
	// set value on PWM channel and timer for next adjustment
	if (lstCnl.characteristic) {															// check if we should use quadratic approach

		characteristicStat = setStat * setStat;												// recalculate the value
		characteristicStat /= 200;															// divide it by 200
		if ((setStat) && (!characteristicStat)) characteristicStat = 1;						// till 15 it is below 1
		fSwitch(regCnl, characteristicStat, lstCnl.characteristic);									// set accordingly

	} else {
		fSwitch(regCnl, setStat, lstCnl.characteristic);									// set accordingly

	}
	adjTmr.set(adjDlyPWM);																	// set timer for next action
}
void cmDimmer::blinkOffDly(void) {

	// some sanity
	if (!activeOffDlyBlink) return;															// blink off flag not set, jump out

	// adjust timer not needed for PWM at the moment - take it for blinking delay
	if (!adjTmr.done()) return;

	// check in which cycle we are, 1 means led is off, 0 led is on again 
	if (statusOffDlyBlink) {
		statusOffDlyBlink = 0;																// switch led on next time
		adjTmr.set(10);																		// off for 30 ms
		fSwitch(regCnl, 1, lstCnl.characteristic);											// set led to minimum
		
	} else {
		statusOffDlyBlink = 1;																// switch led off next time
		adjTmr.set(500);																	// on for 500 ms
		if (lstCnl.characteristic) fSwitch(regCnl, characteristicStat, lstCnl.characteristic);	// take the quadratic value
		else fSwitch(regCnl, modStat,lstCnl.characteristic);								// restore origin value
	}
}
void cmDimmer::sendStatus(void) {

	if (!sendStat) return;																	// nothing to do
	if (!msgTmr.done()) return;																// not the right time
		
	// prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80
	if      (modStat == setStat) modDUL  = 0;
	else if (modStat <  setStat) modDUL  = 0x10;
	else if (modStat >  setStat) modDUL  = 0x20;
	if (!delayTmr.done() )       modDUL |= 0x40;
		
	// check which type has to be send - if it is an ACK and modDUL != 0, then set timer for sending a actuator status
	if      (sendStat == 1) hm->sendACK_STATUS(regCnl, modStat, modDUL);					// send ACK
	else if (sendStat == 2) hm->sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);			// send status

	// check if it is a stable status, otherwise schedule next info message
	if (modDUL >= 0x40) {																	// status is currently changing
		sendStat = 2;																		// send next time a info status message
		msgTmr.set(delayTmr.remain()+5);

	} else if (modDUL) {
		sendStat = 2;																		// send next time a info status message
		msgTmr.set(msgDelay);

	} else sendStat = 0;																	// no need for next time
}
void cmDimmer::dimPoll(void) {
	
	adjPWM();																				// check if something is to be set on the PWM channel
	blinkOffDly();																			// check if off delay blinking is needed
	sendStatus();																			// check if there is some status to send
	
	// check temperature against settings
	// pTemp
	
	// check if something is to do on the dimmer
	if (!delayTmr.done() ) return;															// timer not done, wait until then


	// upDim, check if onTimer was running
	if (l3->actionType == 4)  {		// updim
		modStat = l3->offLevel;
		adjDlyPWM = 1;																		// do the adjustment in 1ms steps
		l3->actionType = 0;																	// no further action required
	}

	// trigger11, check if onTimer was running
	if (duraTme)  {
		modStat = 0;
		adjDlyPWM = intTimeCvt(rampTme);													// get the ramp on time
		adjDlyPWM /= 200;																	// break down the ramp time to smaller slices for adjusting PWM
		rampTme = duraTme = 0;																// no further action required
	}
	
	// - jump table section, only
	if (l3->actionType != 1) return;														// only valid for jump table
	if (curStat == nxtStat) return;															// no status change expected
	activeOffDlyBlink = 0;	

	// check the different status changes, {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if        (nxtStat == 1) {		// dlyOn
		#ifdef DI_DBG
		dbg << F("dlyOn\n");
		#endif 
		
		curStat = nxtStat;																	// remember current status
		nxtStat = l3->jtDlyOn;																// get next status from jump table
		
		if (l3->onDly) {																	// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->onDly));											// activate the timer and set next status
		}
		

	} else if (nxtStat == 2) {		// rampOn
		#ifdef DI_DBG
		dbg << F("rampOn\n");
		#endif
		
		// check modStat against onLevel, if not compare, set the right values
		if (modStat != l3->onLevel)	{														// modStat not set, so first time
			modStat	= l3->onLevel;															// set module status accordingly settings
			adjDlyPWM = byteTimeCvt(l3->rampOnTime);										// get the ramp on time
			delayTmr.set(adjDlyPWM);														// set the ramp time to poll delay, otherwise we will every time end here
			adjDlyPWM /= 200;																// break down the ramp time to smaller slices for adjusting PWM
		}
		
		// check if ramp on is done, set next status
		if (modStat == setStat) {															// ramp on is done
			curStat = nxtStat;																// set current status accordingly
			nxtStat = l3->jtRampOn;															// set next status accordingly the jump table
		}

		
	} else if (nxtStat == 3) {		// on
		#ifdef DI_DBG
		dbg << F("on\n");
		#endif
		
		curStat = nxtStat;																	// remember current status, when timer not set, we stay here for ever
		
		if ((l3->onTime) && (l3->onTime != 255)) {											// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->onTime));											// activate the timer and set next status

			// nxtStat = l3->jtOn; seems to be wrong here, because original device goes to rampOff and stays in off, so refill jumptable
			nxtStat = 5;																	// go to ramp off
			l3->jtRampOff = 6;																// jump from rampOff to off
			l3->jtOff = 6;																	// stay in off mode
		} //else nxtStat = l3->jtOn;														// not sure in which scenario it is needed


	} else if (nxtStat == 4) {		// dlyOff
		#ifdef DI_DBG
		dbg << F("dlyOff\n");
		#endif

		curStat = nxtStat;																	// remember current status
		nxtStat = l3->jtDlyOff;																// get jump table for next status

		if (l3->offDly) {																	// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->offDly));											// activate the timer and set next status
			if (l3->offDlyBlink) activeOffDlyBlink = 1;
		}


	} else if (nxtStat == 5) {		// rampOff
		#ifdef DI_DBG
		dbg << F("rampOff\n");
		#endif
		
		// check modStat against offLevel, if not similar, set the right values
		if (modStat != l3->offLevel) {														// check for first time and set the correct values
			modStat	= l3->offLevel;															// set the PWM to the right value
			adjDlyPWM = byteTimeCvt(l3->rampOffTime);										// get the ramp off time
			delayTmr.set(adjDlyPWM);														// set ramp off time to the poll timer, other wise we will check every ms again
			adjDlyPWM /= 200;																// split PWN timer to slices for smooth dimming
		}
		
		// check if ramp on is done, set next status
		if (modStat == setStat) {															// check if ramp off is done
			curStat = nxtStat;																// remember the current status
			nxtStat = l3->jtRampOff;														// get the next status from jump table
		}


	} else if (nxtStat == 6) {		// off
		#ifdef DI_DBG
		dbg << F("off\n");
		#endif

		curStat = nxtStat;																	// remember the current status
		//nxtStat = l3->jtOff;																// get the next status from jump table

		if ((l3->offTime) && (l3->offTime != 255)) {										// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->offTime));											// activate the timer and set next status
			// refill jumptable

		}

	}

}

  //- helpers defined functions -------------------------------------------------------------------------------------------
void cmDimmer::showStruct(void) {

	dbg << "\nctRampOn " << l3->ctRampOn << ", ctRampOff " << l3->ctRampOff << ", ctDlyOn " << l3->ctDlyOn << \
	        ", ctDlyOff " << l3->ctDlyOff << ", ctOn " << l3->ctOn << ", ctOff " << l3->ctOff << \
			", ctValLo " << l3->ctValLo << ", ctValHi " << l3->ctValHi << '\n' << '\n';
	 
	dbg << "onDly " << l3->onDly << ", onTime " << l3->onTime << ", offDly " << l3->offDly << ", offTime " << l3->offTime << \
	       ", actionType " << l3->actionType << ", lgMultiExec " << l3->lgMultiExec << \
		   ", offTimeMode " << l3->offTimeMode << ", onTimeMode " << l3->onTimeMode << '\n' << '\n'; 

	dbg << "jtOn " << l3->jtOn << ", jtOff " << l3->jtOff << ", jtDlyOn " << l3->jtDlyOn << ", jtDlyOff " << l3->jtDlyOff << \
	       ", jtRampOn " << l3->jtRampOn << ", jtRampOff " << l3->jtRampOff << '\n' << '\n';
	
	dbg << "offDlyBlink " << l3->offDlyBlink << ", onLvlPrio " << l3->onLvlPrio << ", onDlyMode " << l3->onDlyMode << ", offLevel " << l3->offLevel << \
	       ", onMinLevel " << l3->onMinLevel << ", onLevel " << l3->onLevel << '\n' << '\n';

	dbg << "rampSstep " << l3->rampSstep << ", rampOnTime " << l3->rampOnTime << ", rampOffTime " << l3->rampOffTime << '\n' << '\n';

	dbg << "dimMinLvl " << l3->dimMinLvl << ", dimMaxLvl " << l3->dimMaxLvl << ", dimStep " << l3->dimStep << \
	       ", offDlyNewTime " << l3->offDlyNewTime << ", offDlyOldTime " << l3->offDlyOldTime << '\n' << '\n';

	dbg << '\n';
}


//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void cmDimmer::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef DI_DBG
	dbg << F("RL\n");
	#endif

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (curStat == 3) nxtStat = 4;															// currently on, next status should be off
	else nxtStat = 1;																		// currently off, next status should be on
	//onDly   = 0; onTime  = 255; offDly  = 0; offTime = 255;								// set timers
}
void cmDimmer::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef DI_DBG
	dbg << F("CCE, lst1: ") << pHex(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif

	// get message delay
	msgDelay = lstCnl.statusInfoMinDly * 500;
	
	srand(17);
	if (lstCnl.statusInfoRandom) msgDelay += (rand()%(uint16_t)(lstCnl.statusInfoRandom*1000));

	if (!msgDelay) msgDelay = 2000;
	//dbg << "md" << msgDelay << '\n';
}
void cmDimmer::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd and 3rd byte = ramp time; 4th and 5th byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef DI_DBG
	dbg << F("PSE, value:") << pHexB(data[0]);
	if (len > 1) { dbg << F(", rampTime: ") << pHex((data+1), 2); };
	if (len > 3) { dbg << F(", duraTime: ") << pHex((data+3), 2); };
	dbg << '\n';
	#endif

	trigger11(data[0], (len > 1)?data+1:NULL, (len > 3)?data+3:NULL);

	// status will send via dimPoll function, therefor we have to indicate that an ACK has to be send
	//hm->sendACK_STATUS(regCnl, data[0], modDUL);
	sendStat = 1;																			// ACK should be send
	msgTmr.set(100);																		// give some time
}
void cmDimmer::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef DI_DBG
	dbg << F("PSR\n");
	#endif
	
	// status will send via dimPoll function, therefor we have to indicate that an ACK has to be send
	//hm->sendACK_STATUS(regCnl, data[0], modDUL);
	sendStat = 2;																			// ACK should be send
	msgTmr.set(0);																			// immediately
}
void cmDimmer::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef DI_DBG
	dbg << F("PME, type: ")  << pHexB(type) << F(", data: ")  << pHex(data, len) << '\n';
	#endif
	
	if (type == 0x40) trigger40((data[0] & 0x40)?1:0, data[1]);								// filter out the long message bit
	if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);

	if ((type == 0x3e) || (type == 0x40) || (type == 0x41)) {
		// status will send via dimPoll function, therefor we have to indicate that an ACK has to be send
		//hm->sendACK_STATUS(regCnl, modStat, modDUL);
		sendStat = 1;																		// ACK should be send
		msgTmr.set(100);																	// immediately

	} else {
		hm->sendACK();

	}
}

void cmDimmer::poll(void) {
	dimPoll();
}

//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
void cmDimmer::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	hm = instPtr;																			// set pointer to the HM module
	hm->rg.regInAS(cnl, lst, s_mod_dlgt(this,&cmDimmer::hmEventCol), (uint8_t*)&lstCnl,(uint8_t*)&lstPeer);
	regCnl = cnl;																			// stores the channel we are responsible fore
}
void cmDimmer::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	// dbg << "by3:" << by3 << " by10:" << by10 << " d:" << pHex(data, len) << '\n';
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
void cmDimmer::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4
	#ifdef DI_DBG
	dbg << F("peerAddEvent: pCnl1: ") << pHexB(data[0]) << F(", pCnl2: ") << pHexB(data[1]) << F(", pIdx1: ") << pHexB(data[2]) << F(", pIdx2: ") << pHexB(data[3]) << '\n';
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
void cmDimmer::firstStart(void) {
	//#ifdef DI_DBG
	dbg << F("firstStart\n");
	//#endif
}