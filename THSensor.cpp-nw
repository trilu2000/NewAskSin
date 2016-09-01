//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin th sensor class ------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

//#define TH_DBG																				// debug message flag
#include "THSensor.h"

waitTimer sensTmr;																				// timer instance for timing purpose
#define measureDelay 500																		// time between measurement and sending the message

//-------------------------------------------------------------------------------------------------------------------------
//- user defined functions -
//-------------------------------------------------------------------------------------------------------------------------
void THSensor::config(void Init(), void Measure(), uint8_t *Val) {
	fInit = Init;
	fMeas = Measure;
	ptrVal = Val;
	if (fInit) fInit();
}
void THSensor::timing(uint8_t mode, uint32_t sendDelay, uint8_t levelChange) {
	// mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change
	mMode = mode;
	mSendDelay = sendDelay;
	mLevelChange = levelChange;
	sensTmr.set(500);
}
void THSensor::sensPoll(void) {

	if (!sensTmr.done() ) return;																// step out while timer is still running
	
	if (mMode == 0) {
		if (!sState) {																			// bit not set, there for measure and set bit
			sState = 1;
			sensTmr.set(measureDelay);															// we are upfront of the timing, remain timing with measurement time
			if (fMeas) fMeas();																	// call the measurement function
			sensVal[0] = msgCnt;																// copy the current message counter
			sensVal[1] = *ptrVal;																// copy the current sensor value
						
		} else {																				// bit is set, measurement is done, so we should send
			sState = 0;																			// remove bit while next time measurement is needed

			if (mSendDelay == 0) sensTmr.set((calcSendSlot() *250) - measureDelay);				// set a new measurement time
			else sensTmr.set(mSendDelay - measureDelay);

			msgCnt++;																			// increase the message counter
			hm.sendSensor_event(regCnl,1,sensVal);												// prepare the message and send

		}
	} else if (mMode == 1) {
		if (sensVal[1] + mLevelChange > *ptrVal) return;										// check if previous value + level change is greater then current value - exit
		if (sensVal[1] - mLevelChange < *ptrVal) return;										// check if previous value - level change is smaller then current value - exit
		
		// if we are here, timeout was gone and we have a significant change of the value
		sensVal[0] = msgCnt++;																	// copy the current message counter
		sensVal[1] = *ptrVal;																	// copy the current sensor value

		hm.sendSensor_event(regCnl,1,sensVal);													// prepare the message and send
	
	}

}

uint32_t THSensor::calcSendSlot(void) {
	uint8_t a[4];
	a[0] = HMID[2];
	a[1] = HMID[1];
	a[2] = HMID[0];
	a[3] = 0;	
	uint32_t result = ((( *(uint32_t*)&a << 8) | (hm.sn.msgCnt)) * 1103515245 + 12345) >> 16;
	result = (result & 0xFF) + 480;
	//dbg << "calcSendSlot: " << result << '\n'; 
	return result;
}

//-------------------------------------------------------------------------------------------------------------------------
//- mandatory functions for every new module to communicate within HM protocol stack -
//-------------------------------------------------------------------------------------------------------------------------
void THSensor::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef TH_DBG
	dbg << F("CCE, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';
	#endif
}
void THSensor::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd byte = ramp time; 3rd byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef TH_DBG
	dbg << F("PSE, value:") << _HEXB(data[0]);
//	if (len > 1) dbg << F(", ram_TIME: ") << _HEXB(data[1]);
//	if (len > 2) dbg << F(", duraTime: ") << _HEXB(data[2]);
	dbg << '\n';
	#endif
		
	//hm->sendACK_STATUS(regCnl, data[0], modDUL);
}
void THSensor::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef TH_DBG
	dbg << F("PSR\n");
	#endif
	
	hm.sendINFO_ACTUATOR_STATUS(regCnl, modStat, modDUL);
}
void THSensor::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef TH_DBG
	dbg << F("PME, type: ")  << _HEXB(type) << F(", data: ")  << _HEX(data, len) << '\n';
	#endif
	
//	if ((type == 0x3e) || (type == 0x40) || (type == 0x41)) {
//		hm->sendACK_STATUS(regCnl, modStat, modDUL);
//	} else {
//		hm->sendACK();
//	}
}

void THSensor::poll(void) {
	// just polling, as the function name said
	sensPoll();
}


//-------------------------------------------------------------------------------------------------------------------------
//- predefined, no reason to touch -
//-------------------------------------------------------------------------------------------------------------------------
void THSensor::regInHM(uint8_t cnl, uint8_t lst) {
	RG::s_modTable *pModTbl = &modTbl[cnl];													// pointer to the respective line in the module table

	pModTbl->isActive = 1;
	pModTbl->mDlgt = myDelegate::from_function<CLASS_NAME, &CLASS_NAME::hmEventCol>(this);
	pModTbl->lstCnl = (uint8_t*)&lstCnl;
	pModTbl->lstPeer = (uint8_t*)&lstPeer;

	hm.ee.getList(cnl, 1, 0, (uint8_t*)&lstCnl);											// load list1 in the respective buffer
	regCnl = cnl;																			// stores the channel we are responsible fore
	/*hm.rg.regUserModuleInAS(cnl, lst, myDelegate::from_function<THSensor, &THSensor::hmEventCol>(this), (uint8_t*)&lstCnl, (uint8_t*)&lstPeer);
	regCnl = cnl;*/																			// stores the channel we are responsible fore
}
void THSensor::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	// dbg << "by3:" << by3 << " by10:" << by10 << " d:" << _HEX(data, len) << '\n'; _delay_ms(100);
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}
void THSensor::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4
	#ifdef TH_DBG
	dbg << F("peerAddEvent: pCnl1: ") << _HEXB(data[0]) << F(", pCnl2: ") << _HEXB(data[1]) << F(", pIdx1: ") << _HEXB(data[2]) << F(", pIdx2: ") << _HEXB(data[3]) << '\n';
	#endif
	
//	if ((data[0]) && (data[1])) {															// dual peer add
//		if (data[0]%2) {																	// odd
//			//hm->setListFromModule(regCnl,data[2],(uint8_t*)peerOdd,sizeof(peerOdd));
//			//hm->setListFromModule(regCnl,data[3],(uint8_t*)peerEven,sizeof(peerEven));
//		} else {																			// even
//			//hm->setListFromModule(regCnl,data[2],(uint8_t*)peerEven,sizeof(peerEven));
//			//hm->setListFromModule(regCnl,data[3],(uint8_t*)peerOdd,sizeof(peerOdd));
//		}
//	} else {																				// single peer add
//		//if (data[0]) hm->setListFromModule(regCnl,data[2],(uint8_t*)peerSingle,sizeof(peerSingle));
//		//if (data[1]) hm->setListFromModule(regCnl,data[3],(uint8_t*)peerSingle,sizeof(peerSingle));
//	}
}
