/*
 * Todo's:
 *
 * - Statemachine for the channel
 * - set motor reverse via register
 * - register for sendStatusIntervall
 */


#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"															// device configuration file
#include <AS.h>

#define USE_ADRESS_SECTION      1

#define MOTOR_STOP              0
#define MOTOR_LEFT              1
#define MOTOR_RIGHT             2
#define TRAVEL_TIME_MAX         1000											// max travel time without impulses
#define TRAVEL_COUNT_MAX        32768											// ABS value of max travel count (max = 32768)

// function forward declaration
void motorPoll(void);

// set to 1 for reverse the motor direction
uint8_t  reverseMotorDir    = 1;

uint8_t  motorState;
uint8_t  motorStateLast     = MOTOR_STOP;
uint8_t  motorDirLast       = MOTOR_LEFT;
uint8_t  endSwitchState     = 1;

int16_t  travelCountOld = 0;
uint32_t travelTimeStart = 0;
int16_t  travelMax = 0;
uint32_t impulseSwitchTime = 0;
uint32_t intervallTimeStart  = 0;

uint16_t sendStatusIntervall = 2000;											// time after status update is send while traveling

/**
 * @brief This is the Arduino Setup-Function
 *
 * TODO: maybe we move the main parts to own setup/init function?
 */
void setup(void) {
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
		dbg << F("Starting sketch for HM-LC-Bl1-FM_AES ... ("__DATE__" "__TIME__")\n");
		dbg << F(LIB_VERSION_STRING) << "\n";
	#endif

	getEEPromBlock(EEVARS_EEPROM_ADDR, 4, (void*)&eeVars);						// restore eeVars values from eeprom

	#ifdef SER_DBG
		dbg << F("Restore vars from EEProm: mLastDir: ") << eeVars.motorLastDirection;
		dbg << F(", travelCount: ") << eeVars.travelCount << '\n';
	#endif

	hm.init();																	// init the asksin framework

	#if USE_ADRESS_SECTION == 1
		//getDataFromAddressSection(DevType, 0,  ADDRESS_SECTION_START + 0, 2);	// get device type from bootloader section
		getDataFromAddressSection(HMSR, 0, ADDRESS_SECTION_START + 2, 10);		// get hmid from bootloader section
		getDataFromAddressSection(HMID, 0, ADDRESS_SECTION_START + 12, 3);		// get serial number from bootloader section

		#ifdef SER_DBG
			dbg << F("Get HMID and HMSR from Bootloader-Area\n");
		#endif
	#endif

	#ifdef SER_DBG
		dbg << F("HMID: ")  << _HEX(HMID,3)        << "\n";
		dbg << F("HMSR: ")  << _HEX(HMSR,10)       << " (";
		for (uint8_t i = 0; i < 10; i++) {dbg << (char)HMSR[i];}
		dbg << ")\n";

		dbg << F("MAID: ")  << _HEX(MAID,3)        << "\n";

//		dbg << F("HmKey: ") << _HEX(HMKEY, 16)     << "\n";
//		dbg << F("KeyId: ") << _HEX(hmKeyIndex, 1) << "\n";

		for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {							// check if AES activated for any channel
			if (hm.ee.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
				dbg << F("CH " ) << i << F(": AES is active\n");
			}
		}
		dbg << "\n";
	#endif

	/*
	 * At this point you can write your own code
	 */
	motorInit();
}

/**
 * @brief This is the Arduino Forever-Loop
 */
void loop(void) {
	hm.poll();																		// poll the asksin main loop

	/*
	 * At this point you can write your own code
	 */
	motorPoll();
}

/**
 * @brief Init the blind channel modul
 */
void initBlind(uint8_t channel) {
}

/**
 * @brief This function was called at every action
 */
void blindUpdateState(uint8_t channel, uint8_t state, uint32_t rrttb) {			// rrttb = REFERENCE_RUNNING_TIME_BOTTOM_TOP
	travelMax = (uint16_t)(rrttb / 1000);
	travelMax = (travelMax == 0) ? 200 : travelMax;								// if travelMax = 0 then set travelMax to 200

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
		dbg << F("Ch: ") << channel << F(", Stat: ") << state;
		dbg << F(", mStat: ") << motorState << F(", mStatLast: ") << motorStateLast;
		dbg << F(", mDirLast: ") << motorDirLast << F(", travelCount: ") << eeVars.travelCount;
		dbg << F(", travelCountMax: ") << TRAVEL_COUNT_MAX << '\n';
	#endif
}

void motorInit(void) {
	// initialize the impulse input 1
	setPinHigh(SW_IMPULSE_PORT, SW_IMPULSE_PIN);								// set pullup for impulse input 1
	pinInput(SW_IMPULSE_DDR, SW_IMPULSE_PIN);									// set impulse input 1 to input
	regPCIE(SW_IMPULSE_PCIE);													// set the pin change interrupt
	regPCINT(SW_IMPULSE_PCMSK, SW_IMPULSE_INT);									// description is in hal.h

	// initialize the switch input
	setPinHigh(SW_END_PORT, SW_END_PIN);										// set pullup for end switch input
	pinInput(SW_END_DDR, SW_END_PIN);											// end switch input to input
	regPCIE(SW_END_PCIE);														// set the pin change interrupt
	regPCINT(SW_END_PCMSK, SW_END_INT);											// description is in hal.h

	// initialize the motor control pins
	pinOutput(MOTOR_CTRL1_DDR, MOTOR_CTRL1_PIN);								// MOTOR_CTRL1 to output
	pinOutput(MOTOR_CTRL2_DDR, MOTOR_CTRL2_PIN);								// MOTOR_CTRL2 to output

	eeVars.motorLastDirection = MOTOR_STOP;
	motorState = MOTOR_STOP;
	motorStateLast = MOTOR_STOP;
	motorDirLast = MOTOR_STOP;
	endSwitchState = 1;
	travelMax = 0;

	travelCountOld = eeVars.travelCount;
}

void sendPosition(void) {
	uint8_t pos = calcPosition();
	cmMyBlind[0].setSendState(pos);												// send position
}

void motorPoll(void) {
	if (endSwitchState == 0 && motorState == MOTOR_LEFT) {						// end switch reached
		motorState = MOTOR_STOP;
		eeVars.travelCount = 0;

		#ifdef SER_DBG
			dbg << F("Endswitch reached: ") << endSwitchState << "\n";
		#endif
	}

	if (eeVars.travelCount >= travelMax && motorState == MOTOR_RIGHT) { 		// travel distance reached
		motorState = MOTOR_STOP;

		#ifdef SER_DBG
			dbg << F("Travel distance reached, travelMax: ") << travelMax;
			dbg << ", travelCount: " << eeVars.travelCount << "\n";
		#endif
	}

	if (motorState != MOTOR_STOP) {
		setPinHigh(LED_GRN_PORT, LED_GRN_PIN);									// set green LED on at traveling

		if ( (getMillis() - travelTimeStart) > TRAVEL_TIME_MAX ) {				// travel impulse missing
			motorState = MOTOR_STOP;

			#ifdef SER_DBG
				uint32_t tts = (getMillis() - travelTimeStart);
				dbg << F("Travelimpulse missing: ") << tts << ", " << travelTimeStart << "\n";
			#endif
		}
	}

	if (motorState != motorStateLast) {
		if        (motorState == MOTOR_STOP) {
			setPinLow(LED_GRN_PORT, LED_GRN_PIN);								// set green LED off at stop
			motorStopBrake();

		} else if (motorState == MOTOR_LEFT) {
			eeVars.motorLastDirection = MOTOR_LEFT;
			if (reverseMotorDir) {
				motorRight();
			} else {
				motorLeft();
			}

		} else if (motorState == MOTOR_RIGHT && eeVars.travelCount < travelMax) {
			eeVars.motorLastDirection = MOTOR_RIGHT;
			if (reverseMotorDir) {
				motorLeft();
			} else {
				motorRight();
			}
		}
	}

	if (eeVars.travelCount != travelCountOld) {
		// reset travelTimeStart
		travelTimeStart = getMillis();
		travelCountOld = eeVars.travelCount;

		#ifdef SER_DBG
			dbg << F("TravelCnt: ") << eeVars.travelCount << "\n";
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
			motorStop();														// release motor brake
		}
	}
}

void motorRight(void) {
	// Make shure motor is stopped and delay some ms to prevent destroying the H-Bridge
	motorStop();

	setPinHigh(MOTOR_CTRL1_PORT, MOTOR_CTRL1_PIN);
	setPinLow (MOTOR_CTRL2_PORT, MOTOR_CTRL2_PIN);
}

void motorLeft(void) {
	// Make shure motor is stopped and delay some ms to prevent destroying the H-Bridge
	motorStop();

	setPinLow (MOTOR_CTRL1_PORT, MOTOR_CTRL1_PIN);
	setPinHigh(MOTOR_CTRL2_PORT, MOTOR_CTRL2_PIN);
}

void motorStop(void) {
	setPinLow (MOTOR_CTRL1_PORT, MOTOR_CTRL1_PIN);
	setPinLow (MOTOR_CTRL2_PORT, MOTOR_CTRL2_PIN);

	// Without delay H-Bridge may be destroyed
	_delay_ms(100);
}

void motorStopBrake(void) {
	// Make shure motor is stopped and delay some ms to prevent destroying the H-Bridge
	motorStop();

	setPinHigh(MOTOR_CTRL1_PORT, MOTOR_CTRL1_PIN);
	setPinHigh(MOTOR_CTRL2_PORT, MOTOR_CTRL2_PIN);

	eeVars.initialPos = calcPosition();
	setEEPromBlock(EEVARS_EEPROM_ADDR, 4, (void*)&eeVars);						// save eeVars to eeprom
}

uint8_t calcPosition(void) {
	uint8_t position = (uint8_t)( ((eeVars.travelCount > 0 ? (int32_t)eeVars.travelCount : 0) * 200 ) / travelMax );
	position = (position > 200) ? 200 : (200 - position);
	return position;
}

// own PCINT1_vec
ISR (PCINT1_vect) {
	uint8_t impulseSwitch = (PINC & _BV(SW_IMPULSE_PIN));

	endSwitchState = (PINC & _BV(SW_END_PIN));									// here we need no debounceing

	//dbg << F("impulseSwitch: ") << impulseSwitch;
	//dbg << F(", endSwitchState: ") << endSwitchState;
	//dbg << F(", PINC: ") << PINC << "\n";

	if (!impulseSwitch && (getMillis() - impulseSwitchTime > DEBOUNCE) ) {		// trigger on release impulse contact
		impulseSwitchTime = getMillis();

		if ( (
			(motorState == MOTOR_LEFT || (motorState == MOTOR_STOP && eeVars.motorLastDirection == MOTOR_LEFT) ) &&
			eeVars.travelCount > -TRAVEL_COUNT_MAX) ) {

			if (endSwitchState) {												// negative count only in end switch not active
				eeVars.travelCount--;
			}

		} else if ( (
			(motorState == MOTOR_RIGHT || (motorState == MOTOR_STOP && eeVars.motorLastDirection == MOTOR_RIGHT) ) &&
			eeVars.travelCount < TRAVEL_COUNT_MAX) ) {

			eeVars.travelCount++;
		}
	}
}

#if USE_ADRESS_SECTION == 1
	void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen) {
		for (unsigned char i = 0; i < dataLen; i++) {
			buffer[(i + bufferStartAddress)] = pgm_read_byte(sectionAddress + i);
		}
	}
#endif
