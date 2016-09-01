//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin cmPowerSens class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include <cmPowerSens.h>

#define CM_POWER_SENS_DBG


/**
 * @brief Config the blind module
 *
 * @param init(uint8_t):                 pointer to user init function
 * @param updateState(uint8_t, uint8_t): pointer to user updateState function
 */
void cmPowerSens::config(void init(uint8_t)) {

	fInit = init;
	//fUpdateState = updateState;
	
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

void cmPowerSens::addToEnergyCount(int tenthKw){

	const uint32_t maxVal = 8388607;
	energyCount = energyCount + tenthKw;
	if(energyCount > maxVal){
		energyCount = energyCount - maxVal;
	}
}


void cmPowerSens::sendPowerEvent(uint32_t eCountToSend, uint32_t powerToSend){
	uint8_t buf[6] = { 0, 0, 0, 0, 0, 0 };

	// energy count
	buf[2] = (uint8_t) (eCountToSend);
	buf[1] = (uint8_t) (eCountToSend >> 8);
	buf[0] = ((uint8_t) (eCountToSend >> 16 )) & 0x7F;

	// current power
	buf[5] = (uint8_t) (power);
	buf[4] = (uint8_t) (power >> 8);
	buf[3] = (uint8_t) (power >> 16);

	// TODO: set power value

	#ifdef SER_DBG
		/*Serial << F("sendPowerEvent1: ") <<  eCountToSend << '\n';
		Serial << F("sendPowerEvent2.2: ") << (uint8_t) (eCountToSend ) << '\n';
		Serial << F("sendPowerEvent2.1: ") << (uint8_t)(eCountToSend >> 8) << '\n';
		Serial << F("sendPowerEvent2.0: ") << (uint8_t) (eCountToSend >> 16) << '\n';
		Serial << F("sendPowerEvent3: ") << _HEX(buf, 6) << '\n';*/
	#endif

	hm.sendINFO_POWER_EVENT(buf);
	lastReportedEnergyCount = energyCount;
}


uint16_t toUint16(uint8_t left, uint8_t right){
	uint16_t res = left;
	res = (res << 8) + right;
	return res;
}

void cmPowerSens::pollSensor(void) {
	s_lstCnl * cnlList = ((s_lstCnl *)modTbl[0].lstCnl);
	unsigned long now = getMillis();
	//uint16_t signalsPerKWh = cnlList->METER_CONSTANT_IR1; // ( cnlList->METER_CONSTANT_IR1  + (cnlList->METER_CONSTANT_IR2 >> 8)); // TODO: take turns per kwH from eeprom into consideration
	const uint16_t signalsPerKWh = toUint16(cnlList->METER_CONSTANT_IR1, cnlList->METER_CONSTANT_IR2); //(signalsPerKWh << 8 ) + cnlList->METER_CONSTANT_IR2;

	if (infraDetector.powerInputChanged() > 0) {

		/**
		 * sum up used energy
		 */
		// won't work for signalsPerKWh > 10000 but anyway the sensor wouldn't pick it up
		const uint32_t centiWPerSignal = 10000 / signalsPerKWh;
		//Serial << F("used another centiWPerSignal: ") << centiWPerSignal << '\n';
		//Serial << F("signalsPerKWh: ") << signalsPerKWh << '\n';
		addToEnergyCount(centiWPerSignal);
		// Serial << F("energyCount: ") << energyCount << '\n';

		/**
		 * calcuate power consumption
		 */
		unsigned long tsDiff = now - lastSignalTs;
		//Serial << F("power 1sig: ") <<  (1000.0 / signalsPerKwh) << '\n';
		//Serial << F("power tsDiff: ") <<  tsDiff << '\n';
		power = (1000.0 / signalsPerKWh) * (3600000.0 / tsDiff);
		//Serial << F("power: ") <<  power << '\n';
		lastSignalTs = now;
	}

	// report every x seconds or if more than 1kwh was used
	if (now > lastReportedTs + 180000
			|| energyCount > lastReportedEnergyCount + 1000) {
		sendPowerEvent(energyCount, power);
		lastReportedTs = now;
	}
}

void cmPowerSens::poll(void) {

	pollSensor();
	/*//updateState();																	// check if something is to be set on the PWM channel
	//sendState();																	// check if there is some status to send
	//
*/

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
inline void cmPowerSens::setToggle(void) {
/*	#ifdef CM_POWER_SENS_DBG
		dbg << F("TOGGLE_BLIND\n");
	#endif

	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (curState == AS_CM_JT_ON) {												// currently on, next status should be off
		nxtState = AS_CM_JT_OFF;

	} else if (nxtState == AS_CM_JT_OFF) {										// currently off, next status should be on
		nxtState = AS_CM_JT_ON;
	}*/
}

/**
 * @brief it's only for information purpose if channel config was changed (List0/1 or List3/4)
 */
void cmPowerSens::configCngEvent(void) {
	Serial.print("configCngEvent 1\n");

	//modTbl[1].

	//getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf)
	dbg << F("lstCnl: ") << _HEX(modTbl[0].lstCnl, 9) << '\n';
	dbg << F("lstCnl *: ") << ((int)modTbl[0].lstCnl) <<  '\n';
	dbg << F("lstCnl: ") << _HEX((uint8_t*)&lstCnl, 9) << '\n';
	dbg << F("lstCnl *: ") << ((int)&lstCnl) <<  '\n';
	//dbg << F("lstCnl *: ") << ((int)modTbl[0].lstPeer) << F(" lst: ") << modTbl[0].lst <<  '\n';

	s_lstCnl * cnlList = ((s_lstCnl *)modTbl[0].lstCnl);
	cnlList->METER_CONSTANT_LED1;
	dbg << F("meter type: ") << cnlList->METER_TYPE << F(", METER_CONSTANT_IR: ") << cnlList->METER_CONSTANT_IR1   << '.' << cnlList->METER_CONSTANT_IR2 << '\n';
	dbg << F("METER_CONSTANT_GAS: ") << cnlList->METER_CONSTANT_GAS1 << '.' << cnlList->METER_CONSTANT_GAS2 << F(", METER_CONSTANT_LED: ") << cnlList->METER_CONSTANT_LED1  << '.' << cnlList->METER_CONSTANT_LED2 << F(", METER_SENSIBILITY_IR: ") <<  convertMeterSensibility(cnlList->METER_SENSIBILITY_IR) << '\n';
	//fUpdateState(lstCnl.METER_TYPE, lstCnl.METER_CONSTANT_GAS2, lstCnl.METER_CONSTANT_IR2, lstCnl.METER_CONSTANT_LED2, lstCnl.METER_TYPE);
	Serial.print("configCngEvent 2\n");

	infraDetector.setLimit(convertMeterSensibility(cnlList->METER_SENSIBILITY_IR));
	//dbg << F("Channel config changed, lst1: ") << _HEX(((uint8_t*)&lstCnl), sizeof(s_lstCnl)) << '\n';

	/*modReferenceTimeTopBottom = GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_TOP_BOTTOM);
	modReferenceTimeBottomTop = GET_2_BYTE_VALUE(lstCnl.REFERENCE_RUNNING_TIME_BOTTOM_TOP);

	msgDelay = lstCnl.STATUSINFO_MINDELAY * 500;									// get message delay

	if (lstCnl.STATUSINFO_RANDOM) {
		// maybe we outsource this call to external module
		hm->initPseudoRandomNumberGenerator();
		msgDelay += (rand()%(uint16_t)(lstCnl.STATUSINFO_RANDOM * 1000));
	}*/

	if (!msgDelay) {
		msgDelay = 2000;
	}
}

uint8_t cmPowerSens::convertMeterSensibility(uint8_t sensibility){
	/*
	 * METER_SENSIBILITY_IR: 255 = -1
	 * METER_SENSIBILITY_IR: 157 = -99
	 * METER_SENSIBILITY_IR: 0	= 0
	 * METER_SENSIBILITY_IR: 99 = 00
	 * we convert it to 1-199 for easier handling
	*/
	if(sensibility > 100){
		return  -156 + sensibility;
	} else {
		return sensibility + 100;
	}
}

/**
 * @brief We received a status request.
 *        Appropriate answer is an InfoActuatorStatus message
 */
inline void cmPowerSens::pairStatusReq(void) {
	#ifdef CM_POWER_SENS_DBG
		dbg << F("statusRequest\n");
	#endif
	Serial.print("statusRequest\n");
	
	stateToSend = AS_CM_STATETOSEND_STATE;											// ACK should be send
	msgTmr.set(0);																	// immediately
}


/**********************************
 * predefined, no reason to touch *
 **********************************/

/**
 * @brief Register module in HM
 */
void cmPowerSens::regInHM(uint8_t cnl, uint8_t lst) {
	#ifdef CM_POWER_SENS_DBG
		dbg << F("regInHM cnl: ")  << cnl << F(" lst: ")  << lst << '\n';
	#endif
	RG::s_modTable *pModTbl = &modTbl[cnl];													// pointer to the respective line in the module table

	pModTbl->isActive = 1;
	pModTbl->mDlgt = myDelegate::from_function<CLASS_NAME, &CLASS_NAME::hmEventCol>(this);
	pModTbl->lstCnl = (uint8_t*)&lstCnl;
	pModTbl->lstPeer = (uint8_t*)&lstPeer;

	hm.ee.getList(cnl, 1, 0, (uint8_t*)&lstCnl);											// load list1 in the respective buffer
	regCnl = cnl;																			// stores the channel we are responsible fore
		
	/*hm.rg.regUserModuleInAS(
		cnl,
		lst,
		myDelegate::from_function<cmPowerSens, &cmPowerSens::hmEventCol>(this),
		(uint8_t*)&lstCnl,
		(uint8_t*)&lstPeer
	);

	regCnl = cnl;	*/																// stores the channel we are responsible fore
	infraDetector = InfraRedSignalDetector();
}

/**
 * @brief HM event controller
 */
void cmPowerSens::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	_delay_ms(100);
	unsigned long now = getMillis();
	if      ((by3 == 0x00) && (by10 == 0x00)) poll();
	//else if ((by3 == 0x00) && (by10 == 0x01)) setToggle();
	else if ((by3 == 0x00) && (by10 == 0x02)) firstStart();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	//else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	//else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	//else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	//else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else {
		#ifdef CM_POWER_SENS_DBG
			dbg << F("cmPowerSens::hmEventCol - unhandled event!: by3=") << by3 << F(" by10=") << by10 ;
			dbg << F("by11=") << by11 << F(" data: ") << _HEX(data, len) ;
		#endif
		return;
	}

}


inline void cmPowerSens::firstStart(void) {
	#ifdef CM_POWER_SENS_DBG
		dbg << F("firstStart\n");
	#endif
}
