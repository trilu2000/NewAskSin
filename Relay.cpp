//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin relay class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define RL_DBG																				// debug message flag
#include "Relay.h"

waitTimer delayTmr;																				// delay timer for relay
waitTimer msgTmr;																				// message timer for sending status

//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
void Relay::config(void Init(), void Switch(uint8_t), uint8_t minDelay) {

	fInit = Init;
	fSwitch = Switch;

	minDly = minDelay;																			// remember minimum delay for sending the status

	// set output pins
	fInit();

	// some basic settings for start
	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	curStat = nxtStat = 6;																		// set relay status to off
	modDUL = 0x40;																				// otherwise status gets not send
	adjRly(0);
}
void Relay::trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime) {

	if (rampTime) rampTme = (uint16_t)rampTime[0]<<8 | (uint16_t)rampTime[1];					// store ramp time
	else rampTme = 0;

	if (duraTime) duraTme = (uint16_t)duraTime[0]<<8 | (uint16_t)duraTime[1];					// duration time if given
	else duraTme = 0;

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (value) {
		curStat = 6;																			// set current status to off
		nxtStat = 1;																			// and the next one to delay on, if ramptime is set poll will recognize

	} else {
		curStat = 3;
		nxtStat = 4;
	}
	
	if ((rampTme) || (duraTme)) modDUL = 0x40;													// important for the status message
	else modDUL = 0x00;

	//dbg << F("RL:trigger11, val:") << value << F(", nxtS:") << nxtStat << F(", rampT:") << intTimeCvt(rampTme) << F(", duraT:") << intTimeCvt(duraTme) << '\n';
}
void Relay::trigger40(uint8_t msgLng, uint8_t msgCnt) {

	// check for repeated message
	if (( msgLng) && (cnt == msgCnt) && (!lstPeer.lgMultiExec)) return;							// trigger was long, but we have no multi execute
	if ((!msgLng) && (cnt == msgCnt)) return;													// not a long message but counter is the same, means a repeated messgae
	cnt = msgCnt;																				// remember message counter

	// fill the respective variables
	uint8_t actTp = (msgLng)?lstPeer.lgActionType:lstPeer.shActionType;							// get action type = {off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3}

	if        (actTp == 0) {						// off

	} else if (actTp == 1) {						// jmpToTarget
		// SwJtOn {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}

		if      (curStat == 6) nxtStat = (msgLng)?lstPeer.lgSwJtOff    :lstPeer.shSwJtOff;		// currently off
		else if (curStat == 3) nxtStat = (msgLng)?lstPeer.lgSwJtOn     :lstPeer.shSwJtOn;		// on
		else if (curStat == 4) nxtStat = (msgLng)?lstPeer.lgSwJtDlyOff :lstPeer.shSwJtDlyOff;	// delay off
		else if (curStat == 1) nxtStat = (msgLng)?lstPeer.lgSwJtDlyOn  :lstPeer.shSwJtDlyOn;	// delay on

		onDly   = (msgLng)?lstPeer.lgOnDly   :lstPeer.shOnDly;									// set timers
		onTime  = (msgLng)?lstPeer.lgOnTime  :lstPeer.shOnTime;
		offDly  = (msgLng)?lstPeer.lgOffDly  :lstPeer.shOffDly;
		offTime = (msgLng)?lstPeer.lgOffTime :lstPeer.shOffTime;

	} else if (actTp == 2) {						// toogleToCnt, if msgCnt is even, then next state is on
		nxtStat = (msgCnt % 2 == 0)?3:6;														// even - relay dlyOn, otherwise dlyOff
		onDly   = 0; onTime  = 255; offDly  = 0; offTime = 255;									// set timers

	} else if (actTp == 3) {						// toggleToCntInv, if msgCnt is even, then next state is off, while inverted
		nxtStat = (msgCnt % 2 == 0)?6:3;														// even - relay dlyOff, otherwise dlyOn
		onDly   = 0; onTime  = 255; offDly  = 0; offTime = 255;									// set timers
	}

	if ((onDly) || (offDly)) {
		modDUL = 0x40;

	} else {
		modDUL = 0x00;																			// no timer running
		modStat = ((nxtStat == 1) || (nxtStat == 3))?0xc8:0x00;									// therefore we show the future state of the relay

	}
	//dbg << "a: " << actTp << ", c: " << curStat << ", n: " << nxtStat << ", onDly: " << pHexB(onDly) << ", onTime: " << pHexB(onTime) << ", offDly: " << pHexB(offDly) << ", offTime: " << pHexB(offTime) << '\n';
}
void Relay::trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal) {

	uint8_t isLng = msgBLL & 0x40;																// is it a long message?
	uint8_t ctTbl, ctLo, ctHi;
	
	if      (curStat == 6) ctTbl = (isLng)?lstPeer.lgCtOff   :lstPeer.shCtOff;					// currently off
	else if (curStat == 3) ctTbl = (isLng)?lstPeer.lgCtOn    :lstPeer.shCtOn;					// on
	else if (curStat == 4) ctTbl = (isLng)?lstPeer.lgCtDlyOff:lstPeer.shCtDlyOff;				// delay off
	else if (curStat == 1) ctTbl = (isLng)?lstPeer.lgCtDlyOn :lstPeer.shCtDlyOn;				// delay on
	
	ctLo = (isLng)?lstPeer.lgCtValLo :lstPeer.shCtValLo;
	ctHi = (isLng)?lstPeer.lgCtValHi :lstPeer.shCtValHi;
	
	// X GE COND_VALUE_LO                         - geLo -  > low           - 0
	// X GE COND_VALUE_HI                         - geHi -  > high          - 1
	// X LT COND_VALUE_LO                         - ltLo -  < low           - 2
	// X LT COND_VALUE_HI                         - ltHi -  < high          - 3
	// COND_VALUE_LO LE X LT COND_VALUE_HIGH      - betW -  low <> high     - 4
	// X LT COND_VALUE_LO OR X GE COND_VALUE_HIGH - outS -  < low or > high - 5

	//dbg << "c: " << curStat  << ", bll: " << msgBLL << ", cnt: " << msgCnt << ", val: " << msgVal  << ", cond: " << ctTbl << '\n';

	if      (ctTbl == 0) if (msgVal > ctLo) trigger40(isLng, msgCnt);
	else if (ctTbl == 1) if (msgVal > ctHi) trigger40(isLng, msgCnt);
	else if (ctTbl == 2) if (msgVal < ctLo) trigger40(isLng, msgCnt);
	else if (ctTbl == 3) if (msgVal < ctHi) trigger40(isLng, msgCnt);
	else if (ctTbl == 4) if ((msgVal > ctLo) && (msgVal < ctHi)) trigger40(isLng, msgCnt);
	else if (ctTbl == 5) if ((msgVal < ctLo) && (msgVal > ctHi)) trigger40(isLng, msgCnt);

}
void Relay::adjRly(uint8_t status) {
	fSwitch(status);																			// switch relay 
	modStat = (status)?0xC8:0x00;																// module status, needed for status request, etc

	if (modDUL) {
		sendStat = 1;																			// we should send the current status change
		msgTmr.set((minDly*1000)+(rand()%2048));
	}
}
void Relay::adjStat(uint8_t status) {
	if (modStat == status) return;

	modStat = status;																			// set new status to message byte
	modDUL = 0;
	sendStat = 1;																				// we should send the current status change
	msgTmr.set(100);																			// send the new status next time

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (status == 0xc8) curStat = nxtStat = 3;													// set relay status accordingly
	else curStat = nxtStat = 6;	
}

void Relay::poll(void) {

	// check if there is some status to send
	if ((sendStat) && (msgTmr.done() )) {
		sendStat = 0;																			// no need for next time
		modDUL = (curStat == nxtStat)?0x00:0x40;												// set change variable
		hm->sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);									// send status
	}
		
	// check if something is to do on the relay
	if (curStat == nxtStat) return;																// no status change expected
	//hm->pw.stayAwake(100);
	if (!delayTmr.done()) return;																// timer not done, wait until then
	
	// check the different status changes
	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if        ((curStat == 1) && (nxtStat == 3)) {		// dlyOn -> on
		adjRly(1);
		curStat = nxtStat;																		// set current status accordingly
		
		if ((onTime) && (onTime != 255)) {														// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(onTime));													// activate the timer and set next status
			onTime = 0;																			// clean variable
			nxtStat = 4;																		// and set the next status variable
		}

		if (duraTme) {																			// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(intTimeCvt(duraTme));													// activate the timer and set next status
			duraTme = 0;																		// clean variable
			nxtStat = 4;																		// and set the next status variable
		} 
				
	} else if ((curStat == 3) && (nxtStat == 4)) {		// on -> dlyOff
		curStat = nxtStat;																		// set current status accordingly
		nxtStat = 6;																			// and set the next status variable

		if (offDly) {																			// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(offDly));													// activate the timer and set next status
			offDly = 0;																			// clean variable
		}

		if (rampTme) {																			// check if there is something in the ramp timer, set next status accordingly
			delayTmr.set(intTimeCvt(rampTme));													// activate the timer and set next status
			rampTme = 0;																		// clean variable
		}


	} else if ((curStat == 4) && (nxtStat == 6)) {		// dlyOff -> off
		adjRly(0);
		curStat = nxtStat;																		// set current status accordingly
		
		if ((offTime) && (offTime != 255)) {													// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(offTime));													// activate the timer and set next status
			offTime = 0;																		// clean variable
			nxtStat = 1;																		// and set the next status variable
		}

		if (duraTme) {																			// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(intTimeCvt(duraTme));													// activate the timer and set next status
			duraTme = 0;																		// clean variable
			nxtStat = 1;																		// and set the next status variable
		}
		
	} else if ((curStat == 6) && (nxtStat == 1)) {		// off -> dlyOn
		curStat = nxtStat;																		// set current status accordingly
		nxtStat = 3;																			// and set the next status variable

		if (onDly) {																			// check if there is something in the duration timer, set next status accordingly
			delayTmr.set(byteTimeCvt(onDly));													// activate the timer and set next status
			onDly = 0;																			// clean variable
		}

		if (rampTme) {																			// check if there is something in the ramp timer, set next status accordingly
			delayTmr.set(intTimeCvt(rampTme));													// activate the timer and set next status
			rampTme = 0;																		// clean variable
		}
		
	}
	
}

//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void Relay::setToggle(void) {
	// setToggle will be addressed by config button in mode 2 by a short key press
	// here we can toggle the status of the actor
	#ifdef RL_DBG
	dbg << F("RL\n");
	#endif

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (curStat == 3) nxtStat = 4;																// currently on, next status should be off
	else nxtStat = 1;																			// currently off, next status should be on
	onDly   = 0; onTime  = 255; offDly  = 0; offTime = 255;										// set timers
}
void Relay::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef RL_DBG
	dbg << F("CCE, lst1: ") << pHex(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif
}
void Relay::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd and 3rd byte = ramp time; 4th and 5th byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef RL_DBG
	dbg << F("PSE, value:") << pHexB(data[0]);
	if (len > 1) { dbg << F(", rampTime: ") << pHex((data+1), 2); };
	if (len > 3) { dbg << F(", duraTime: ") << pHex((data+3), 2); };
	dbg << '\n';
	#endif

	trigger11(data[0], (len > 1)?data+1:NULL, (len > 3)?data+3:NULL);

	hm->sendACK_STATUS(regCnl, data[0], modDUL);
}
void Relay::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef RL_DBG
	dbg << F("PSR\n");
	#endif
	
	hm->sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);
}
void Relay::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef RL_DBG
	dbg << F("PME, type: ")  << pHexB(type) << F(", data: ")  << pHex(data, len) << '\n';
	#endif
	
	if (type == 0x40) trigger40((data[0] & 0x40), data[1]);
	if (type == 0x41) trigger41((data[0] & 0x7F), data[1], data[2]);
	
	if ((type == 0x3e) || (type == 0x40) || (type == 0x41)) {
		hm->sendACK_STATUS(regCnl, modStat, modDUL);

	} else {
		hm->sendACK();

	}
}

//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
void Relay::regInHM(uint8_t cnl, uint8_t lst, AS *instPtr) {
	hm = instPtr;																			// set pointer to the HM module
	hm->rg.regInAS(cnl, lst, s_mod_dlgt(this,&Relay::hmEventCol), (uint8_t*)&lstCnl,(uint8_t*)&lstPeer);
	regCnl = cnl;																			// stores the channel we are responsible fore
}
void Relay::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	// dbg << "by3:" << by3 << " by10:" << by10 << " d:" << pHex(data, len) << '\n'; _delay_ms(100);
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}
void Relay::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4
	#ifdef RL_DBG
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
