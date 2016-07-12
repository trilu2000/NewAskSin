//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin relay class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define RL_DBG																			// debug message flag
#include "cmSwitch.h"

//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
void cmSwitch::config(void Init(uint8_t), void xSwitch(uint8_t,uint8_t)) { //, uint8_t minDelay) {

	fInit = Init;
	fSwitch = xSwitch;

	// set output pins
	fInit(regCnl);

	// some basic settings for start
	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	curStat = nxtStat = 6;																	// set relay status to off
	modStat = setStat = 0;
	
	msgDelay = (rand()%2000)+1000;															// set message delay

	// send the initial status info
	sendStat = 2;
	msgTmr.set(msgDelay);

	l3 = (s_l3*)&lstPeer;																	// set pointer to something useful
	l3->actionType = 0;																		// and secure that no action will happened in polling function

}
void cmSwitch::trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *duraTime) {

	// some sanity
	delayTmr.set(0);																		// also delay timer is not needed any more
	l3->actionType = 0;

	// convert the timer values
	if (rampTime) rampTme = (uint16_t)rampTime[0]<<8 | (uint16_t)rampTime[1];				// if ramp time is given, bring it in the right format
	else rampTme = 0;																		// otherwise empty variable

	if (duraTime) duraTme = (uint16_t)duraTime[0]<<8 | (uint16_t)duraTime[1];				// duration time if given
	else duraTme = 0;																		// or clear value

	// set values to proceed
	tr11Value = setValue;																	// remember the value to be set

	if (rampTme) {																			// ramp time is given
		tr11 = 1;																			// indicate we are coming from trigger11
		curStat = 1;																		// delay on
		delayTmr.set(intTimeCvt(rampTme));													// set the timer accordingly

	} else modStat = setValue;																// otherwise set value directly

	if (duraTme) tr11 = 1;																	// indicate we are coming from trigger11
	//dbg << F("RL:trigger11, val:") << setValue << F(", rampT:") << intTimeCvt(rampTme) << F(", duraT:") << intTimeCvt(duraTme) << '\n';
}
void cmSwitch::trigger40(uint8_t msgLng, uint8_t msgCnt) {

	// some sanity
	delayTmr.set(0);																		// also delay timer is not needed any more
	rampTme = duraTme = 0;
	
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
		else if (curStat == 3) nxtStat = l3->jtOn;											// on
		else if (curStat == 4) nxtStat = l3->jtDlyOff;										// delay off
		else if (curStat == 6) nxtStat = l3->jtOff;											// currently off
		delayTmr.set(0);																	// set timer to 0 for avoiding delays

	} else if (l3->actionType == 2) {		// toogleToCounter

		modStat = (msgCnt%2)?200:0;															// odd

	} else if (l3->actionType == 3) {		// toogleInversToCounter

		modStat = (msgCnt%2)?0:200;															// odd

	}
	//dbg << "a: " << l3->actionType << ", c: " << curStat << ", n: " << nxtStat << ", onDly: " << _HEXB(l3->onDly) << ", onTime: " << _HEXB(l3->onTime) << ", offDly: " << _HEXB(l3->offDly) << ", offTime: " << _HEXB(l3->offTime) << '\n';
}
void cmSwitch::trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal) {

	uint8_t isLng = (msgBLL & 0x40)?1:0;													// is it a long message?
	uint8_t ctTbl;
	
	// set short or long
	l3 = (isLng)?(s_l3*)&lstPeer+1 :(s_l3*)&lstPeer;										// set pointer to the right part of the list3, short or long


	// SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if      (curStat == 1) ctTbl = l3->ctDlyOn;												// delay on
	else if (curStat == 3) ctTbl = l3->ctOn;												// on
	else if (curStat == 4) ctTbl = l3->ctDlyOff;											// delay off
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
void cmSwitch::adjRly(void) {

	// something to do?
	if (setStat == modStat) return;															// nothing to do
	//dbg << "m" << modStat << " s" << setStat << '\n';

	// calculate next step, SwJtOn {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	curStat = modStat?3:6;
	setStat = modStat;																		// follow action
	fSwitch(regCnl, setStat);																// set accordingly
	
	msgTmr.set(0);
}
void cmSwitch::sendStatus(void) {

	if (!sendStat) return;																	// nothing to do
	if (!msgTmr.done()) return;																// not the right time
	
	// prepare message; UP 0x10, DOWN 0x20, ERROR 0x30, DELAY 0x40, LOWBAT 0x80
	if      (modStat == setStat) modDUL  = 0;
	//else if (modStat <  setStat) modDUL  = 0x10;
	//else if (modStat >  setStat) modDUL  = 0x20;
	if (!delayTmr.done() )       modDUL |= 0x40;
	
	// check which type has to be send - if it is an ACK and modDUL != 0, then set timer for sending a actuator status
	if      (sendStat == 1) hm->sendACK_STATUS(regCnl, modStat, modDUL);					// send ACK
	else if (sendStat == 2) hm->sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);			// send status

	// check if it is a stable status, otherwise schedule next info message
	if (modDUL)  {																			// status is currently changing
		sendStat = 2;																		// send next time a info status message
		msgTmr.set(delayTmr.remain()+5);
	} else sendStat = 0;																	// no need for next time
	//sendStat = 0;
}

void cmSwitch::rlyPoll(void) {

	adjRly();																				// check if something is to be set on the Relay channel
	sendStatus();																			// check if there is some status to send

	// check if something is to do on the dimmer
	if (!delayTmr.done() ) return;															// timer not done, wait until then
	

	// - trigger11, check if rampTime or onTimer is set
	if ((tr11) && (rampTme)) {																// ramp timer was set, now we have to set the value
		modStat = tr11Value;																// set the value we had stored
		if (!duraTme) tr11 = 0;																// check duration time, other wise nothing to do anymore
		rampTme = 0;																		// not necessary to do it again
	}

	if ((tr11) && (duraTme))  {																// coming from trigger 11, we should set the duration period
		delayTmr.set(intTimeCvt(duraTme));													// set the duration time
		duraTme = 0;																		// but indicate, it was done
		curStat = modStat?4:1;																// delay off or on depending on modStat value
		
	} else if (tr11) {																		// check if something is to do from trigger11
		modStat = tr11Value^200;															// invert the status
		tr11 = 0;																			// trigger11 ready

	}
	

	// - jump table section, only
	if (l3->actionType != 1) return;														// only valid for jump table
	if (curStat == nxtStat) return;															// no status change expected

	// check the different status changes, {no=>0, dlyOn=>1, rampOn=>2, on=>3, dlyOff=>4, rampOff=>5, off=>6}
	if        (nxtStat == 1) {		// dlyOn
		#ifdef RL_DBG
		dbg << F("dlyOn\n");
		#endif
		
		curStat = nxtStat;																	// remember current status
		nxtStat = l3->jtDlyOn;																// get next status from jump table
		
		if (l3->onDly) {																	// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->onDly));											// activate the timer and set next status
		}
		

	} else if (nxtStat == 3) {		// on
		#ifdef RL_DBG
		dbg << F("on\n");
		#endif
		
		curStat = nxtStat;																	// remember current status, when timer not set, we stay here for ever
		nxtStat = l3->jtOn;																	// get next status from jump table
		modStat = 200;																		// switch relay on
		
		if ((l3->onTime) && (l3->onTime != 255)) {											// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->onTime));											// activate the timer and set next status
			nxtStat = 6;																	// go to off
			l3->jtOff = 6;																	// stay in off mode
		} 

	} else if (nxtStat == 4) {		// dlyOff
		#ifdef RL_DBG
		dbg << F("dlyOff\n");
		#endif

		curStat = nxtStat;																	// remember current status
		nxtStat = l3->jtDlyOff;																// get jump table for next status

		if (l3->offDly) {																	// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->offDly));											// activate the timer and set next status
		}


	} else if (nxtStat == 6) {		// off
		#ifdef RL_DBG
		dbg << F("off\n");
		#endif

		curStat = nxtStat;																	// remember the current status
		nxtStat = l3->jtOff;																// get the next status from jump table
		modStat = 0;																		// switch off relay

		if ((l3->offTime) && (l3->offTime != 255)) {										// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(l3->offTime));											// activate the timer and set next status
			nxtStat = 3;																	// go to on
			l3->jtOn = 3;																	// stay in on mode
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void cmSwitch::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef RL_DBG
	dbg << F("RL\n");
	#endif

	modStat ^= 200;																			// xor the relay status
	sendStat = 2;																			// send next time a info status message
	
}
void cmSwitch::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef RL_DBG
	dbg << F("CCE, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif

}
void cmSwitch::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd and 3rd byte = ramp time; 4th and 5th byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef RL_DBG
	dbg << F("PSE, value:") << _HEXB(data[0]);
	if (len > 1) { dbg << F(", rampTime: ") << _HEX((data+1), 2); };
	if (len > 3) { dbg << F(", duraTime: ") << _HEX((data+3), 2); };
	dbg << '\n';
	#endif

	trigger11(data[0], (len > 1)?data+1:NULL, (len > 3)?data+3:NULL);

	sendStat = 1;																			// ACK should be send
	msgTmr.set(10);																			// give some time

}
void cmSwitch::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef RL_DBG
	dbg << F("PSR\n");
	#endif
	
	sendStat = 2;																			// send next time a info status message
	msgTmr.set(10);																			// wait a short time to set status

}
void cmSwitch::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef RL_DBG
	dbg << F("PME, type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
	if (type == 0x3E) trigger40( data[4], data[5]);
	if (type == 0x40) trigger40((data[0] & 0x40), data[1]);
	if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);
	
	if ((type == 0x3e) || (type == 0x40) || (type == 0x41)) {
		sendStat = 1;																			// send next time a ack info message
		msgTmr.set(10);																			// wait a short time to set status

	} else {
		hm->sendACK();

	}
}

void cmSwitch::poll(void) {
	rlyPoll();
}


//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
cmSwitch::cmSwitch(void) {
}

void cmSwitch::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	hm = instPtr;																			// set pointer to the HM module
	hm->rg.regInAS(cnl, lst, s_mod_dlgt(this,&cmSwitch::hmEventCol), (uint8_t*)&lstCnl,(uint8_t*)&lstPeer);
	regCnl = cnl;																			// stores the channel we are responsible fore
}
void cmSwitch::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	// dbg << "by3:" << by3 << " by10:" << by10 << " d:" << pHex(data, len) << '\n';
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}
void cmSwitch::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4
	#ifdef RL_DBG
	dbg << F("peerAddEvent: pCnl1: ") << _HEXB(data[0]) << F(", pCnl2: ") << _HEXB(data[1]) << F(", pIdx1: ") << _HEXB(data[2]) << F(", pIdx2: ") << _HEXB(data[3]) << '\n';
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
