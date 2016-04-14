/*
 * Todo's:
 * - fix AES handling
 * - Statemachine for the channel
 *
 */


//#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"															// device configuration file
#include <AS.h>

#define USE_ADRESS_SECTION      1

#define MOTOR_STOP              0
#define MOTOR_LEFT              1
#define MOTOR_RIGHT             2

#define TRAVEL_COUNT_MAX        32768											// ABS value of max travel count (max = 32768)

// function forward declaration
void mototPoll();

// set to 1 for reverse the motor direction
// ToDo: should be set via register
uint8_t  reverseMotorDir    = 1;

uint8_t  motorState         = 0;
uint8_t  motorStateLast     = 0;
uint8_t  motorLastDirection = 0;
uint8_t  motorDirLast       = MOTOR_LEFT;
uint8_t  endSwitchState     = 1;

int16_t  travelCount = 0;

int16_t  travelCountOld = 0;
uint32_t travelTimeStart = 0;
uint16_t travelTimeMax = 1000;													// max travel time without impulses
int16_t  travelMax = 0;
uint32_t impulseSwitchTime = 0;
uint32_t intervallTimeStart  = 0;
uint16_t sendStatusIntervall = 2000;											// time after status update is send while traveling

/**
 * @brief This is the Arduino Setup-Function
 *
 * TODO: maybe we move the main parts to own setup/init function?
 */
void setup() {
	// We disable the Watchdog first
	wdt_disable();

	/*
	 * Hardware setup: everything off
	 * Without this block the code freeze after at first spi communication
	 */
	EIMSK = 0;																	// disable external interrupts
	ADCSRA = 0;																	// ADC off
	power_all_disable();														// and everything else

	DDRB = DDRC = DDRD = 0x00;													// everything as input
	PORTB = PORTC = PORTD = 0x00;												// pullup's off
	/* ********************************************************************** */

	// TODO: Maybe we should enable timer0 and SPI internally?
	power_timer0_enable();
	power_spi_enable();															// enable only needed functions

	#ifdef SER_DBG
		dbgStart();																// serial setup
		dbg << F("Starting sketch for HM-LC-Bl1-FM_AES ... ("__DATE__" "__TIME__")\n\n");
		dbg << F(LIB_VERSION_STRING);
	#endif

	// get values from eeprom
	getEEPromBlock(0x03FF, 1, (void*)&motorLastDirection);						// restore motorLastDirection from byte 1024 in eeprom
	getEEPromBlock(0x03FC, 2, (void*)&travelCount);								// restore travelCount from byte 1022 in eeprom
	getEEPromBlock(0x03FB, 1, (void*)&initialPos);								// restore initialPos from byte 1021 in eeprom
//	dbg << F("motorLastDirection: ") << motorLastDirection << F(", travelCount: ") << travelCount << '\n';

	hm.init();																	// init the asksin framework

	#if USE_ADRESS_SECTION == 1
		//getDataFromAddressSection(DevType, 0,  ADDRESS_SECTION_START + 0, 2);	// get device type from bootloader section
		getDataFromAddressSection(HMSR, 0, ADDRESS_SECTION_START + 2, 10);		// get hmid from bootloader section
		getDataFromAddressSection(HMID, 0, ADDRESS_SECTION_START + 12, 3);		// get serial number from bootloader section

		#ifdef SER_DBG
			dbg << F("Get HMID and HMSR from Bootloader-Area: ") << "\n";
		#endif
	#endif

	#ifdef SER_DBG
		dbg << F("HMID: ")  << _HEX(HMID,3)        << "\n";
		dbg << F("HMSR: ")  << _HEX(HMSR,10)       << "\n\n";
		dbg << F("HMSR: ")  << _HEX(HMSR,10)       << "(" << (char*)HMSR << ")\n\n";
		dbg << F("MAID: ")  << _HEX(MAID,3)        << "\n";
//		dbg << F("HmKey: ") << _HEX(HMKEY, 16)     << "\n";
//		dbg << F("KeyId: ") << _HEX(hmKeyIndex, 1) << "\n";

		for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {							// check if AES activated for any channel
			if (hm.ee.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
				dbg << F("AES active for channel: ") << _HEXB(i) << '\n';
			}
		}
		dbg << '\n';
	#endif

	/*
	 * At this point you can write your own code
	 */
	motorInit();
}

/**
 * @brief This is the Arduino Forever-Loop
 */
void loop() {
	hm.poll();																		// poll the asksin main loop

	/*
	 * At this point you can write your own code
	 */
	mototPoll();
}

/**
 * @brief Init the blind channel modul
 */
void initBlind(uint8_t channel) {
	digitalWrite(A2, HIGH);
	pinInput(SW_IMPULSE_DDR, SW_IMPULSE_PIN);									// set channel to input
	regPCIE(SW_IMPULSE_PCIE);													// set the pin change interrupt
	regPCINT(SW_IMPULSE_PCMSK, SW_IMPULSE_INT);									// description is in hal.h

	digitalWrite(A3, HIGH);
	pinInput(SW_END_DDR, SW_END_PIN);											// set channel to input
	regPCIE(SW_END_PCIE);														// set the pin change interrupt
	regPCINT(SW_END_PCMSK, SW_END_INT);											// description is in hal.h
}

/**
 * @brief This function was called at every action
 */
void blindUpdateState(uint8_t channel, uint8_t state, uint32_t rrttb) {			// rrttb = REFERENCE_RUNNING_TIME_BOTTOM_TOP
	travelMax = (uint16_t)(rrttb / 1000);

	if ((state == 200 && motorStateLast == MOTOR_LEFT) || (state == 0 && motorStateLast == MOTOR_RIGHT)) {
		motorState = MOTOR_STOP;

	} else if (state == 200) {
		motorState = MOTOR_LEFT;
		travelTimeStart = getMillis();
		motorDirLast = MOTOR_LEFT;

	} else if (state == 0) {
		motorState = MOTOR_RIGHT;
		travelTimeStart = getMillis();
		motorDirLast = MOTOR_LEFT;

	} else if (state == 201) {							// toggle
		if (motorState > MOTOR_STOP) {
			motorState = MOTOR_STOP;

		} else if (motorDirLast == MOTOR_RIGHT) {
			motorState = MOTOR_LEFT;
			motorDirLast = MOTOR_LEFT;
			travelTimeStart = getMillis();

		} else if (motorDirLast == MOTOR_LEFT) {
			motorState = MOTOR_RIGHT;
			motorDirLast = MOTOR_RIGHT;
			travelTimeStart = getMillis();
		}

	} else if (state == 255) {							// stop
		motorState = MOTOR_STOP;
	}

	intervallTimeStart = getMillis();

	#ifdef SER_DBG
		dbg << F("Ch: ") << channel << F(", State: ") << state << ", motorState: " << motorState << ", motorDirLast: " << motorDirLast << '\n';
	#endif
}

void motorInit() {
	/** Debug start */
	pinMode(A0, OUTPUT);
	pinMode(A1, OUTPUT);

	motorState = MOTOR_STOP;
	motorStateLast = MOTOR_STOP;
	motorLastDirection = MOTOR_STOP;
	travelCountOld = travelCount;
}

void sendPosition() {
	uint8_t pos = (uint8_t)(((travelCount > 0 ? (int32_t)travelCount : 0) * 200 ) / travelMax);
	pos = (pos > 200) ? 200 : 200 - pos;
	cmMyBlind[0].setSendState(pos);												// send position
}

void mototPoll() {
	if (endSwitchState == 0 && motorState == MOTOR_LEFT) {						// end switch reached
		motorState = MOTOR_STOP;
		travelCount = 0;
//		dbg << F("end switch reached: ") << endSwitchState << '\n';
	}

	if (travelCount >= travelMax && motorState == MOTOR_RIGHT) { 				// travel distance reached
		motorState = MOTOR_STOP;

		#ifdef SER_DBG
			dbg << F("travel distance reached, travelMax: ") << travelMax << '\n';
		#endif
	}

	if (motorState != MOTOR_STOP) {
		digitalWrite(5, HIGH);													// set green LED on at traveling
																				// led switched off by asksin module automaticly

		if ( (getMillis() - travelTimeStart) > travelTimeMax ) {				// travel impulse missing
			motorState = MOTOR_STOP;

//			uint32_t tts = (getMillis() - travelTimeStart);
//			dbg << F("travel imp missing: ") << tts << ", " << travelTimeStart << '\n';
		}
	}

	if (motorState != motorStateLast) {
		if        (motorState == MOTOR_STOP) {
//			motorStop();
			motorBreak();

		} else if (motorState == MOTOR_LEFT && travelCount >= 0) {
			motorLastDirection = MOTOR_LEFT;
			if (reverseMotorDir) {
				motorRight();
			} else {
				motorLeft();
			}

		} else if (motorState == MOTOR_RIGHT && travelCount < travelMax) {
			motorLastDirection = MOTOR_RIGHT;
			if (reverseMotorDir) {
				motorLeft();
			} else {
				motorRight();
			}
		}
	}

	if (travelCount != travelCountOld) {
		// reset travelTimeStart
		travelTimeStart = getMillis();
		travelCountOld = travelCount;

		#ifdef SER_DBG
			dbg << F("travelCnt: ") << travelCount << '\n';
		#endif
	}

	if (motorState != motorStateLast) {
		motorStateLast = motorState;
	}

	if ( intervallTimeStart > 0 && (getMillis() - intervallTimeStart) > sendStatusIntervall ) {
		sendPosition();
		if (motorState != MOTOR_STOP) {
			intervallTimeStart = getMillis();									// reset send delay every time if motor is running
		} else {
			intervallTimeStart = 0;
			motorStop();														// release motor break
		}
	}
}

void motorRight() {
	motorStop();
	digitalWrite(A0, 0);
	digitalWrite(A1, 1);
}

void motorLeft() {
	motorStop();
	digitalWrite(A0, 1);
	digitalWrite(A1, 0);
}

void motorStop() {
	digitalWrite(A0, 0);
	digitalWrite(A1, 0);

	// store to  eeprom
	setEEPromBlock(0x03FF, 1, (void*)&motorLastDirection);						// save motorLastDirection to byte 1024 in eeprom
	setEEPromBlock(0x03FC, 2, (void*)&travelCount);								// save travelCount to byte 1022 in eeprom



	// duplicate code. please fix
	uint8_t pos = (uint8_t)(((travelCount > 0 ? (int32_t)travelCount : 0) * 200 ) / travelMax);
	pos = (pos > 200) ? 200 : 200 - pos;
	setEEPromBlock(0x03FB, 2, (void*)&pos);										// save last pos to byte 1022 in eeprom

	_delay_ms(100);
}

void motorBreak() {
	motorStop();
	digitalWrite(A0, 1);
	digitalWrite(A1, 1);
}

// own PCINT1_vec
ISR (PCINT1_vect) {

	uint8_t impulseSwitch = (PINC & _BV(SW_IMPULSE_PIN));

	endSwitchState = (PINC & _BV(SW_END_PIN));									// here we need no debounceing

//	dbg << F("impulseSwitch: ") << impulseSwitch << ", endSwitchState: " << endSwitchState << ", PINC: " << PINC << '\n';

	if (!impulseSwitch && (getMillis() - impulseSwitchTime > DEBOUNCE) ) {		// trigger on release impulse contact
		impulseSwitchTime = getMillis();

		if ( (
			(motorState == MOTOR_LEFT || (motorState == MOTOR_STOP && motorLastDirection == MOTOR_LEFT) ) &&
			travelCount > -TRAVEL_COUNT_MAX) ) {

			travelCount--;

		} else if ( (
			(motorState == MOTOR_RIGHT || (motorState == MOTOR_STOP && motorLastDirection == MOTOR_RIGHT) ) &&
			travelCount < TRAVEL_COUNT_MAX) ) {

			travelCount++;
		}
	}

	/*
	if (irOn) {
		uint8_t a = pcInt[PORTC].cur & _BV(PINC2);
		dbg << "i2:" << pcInt[1].cur << "," << a << "\n";

		if ((pcInt[1].cur & _BV(PINC2)) && irState == 0) {
			irState = 1;
			irCount++;
		} else if ((pcInt[1].cur & _BV(PINC2) == 0) && irState == 1) {
			irState = 0;
			irCount++;
		}
	}
*/
}

#if USE_ADRESS_SECTION == 1
	void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen) {
		for (unsigned char i = 0; i < dataLen; i++) {
			buffer[(i + bufferStartAddress)] = pgm_read_byte(sectionAddress + i);
		}
	}
#endif
