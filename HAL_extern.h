//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
void    ccInitHw(void) {
	pinOutput( CC_CS_DDR, CC_CS_PIN );											// set chip select as output
	pinOutput( SPI_DDR, SPI_MOSI );												// set MOSI as output
	pinInput ( SPI_DDR, SPI_MISO );												// set MISO as input
	pinOutput( SPI_DDR, SPI_SCLK );												// set SCK as output
	pinInput ( CC_GDO0_DDR, CC_GDO0_PIN );										// set GDO0 as input

	SPCR = _BV(SPE) | _BV(MSTR);												// SPI enable, master, speed = CLK/4

	CC_GDO0_PCICR |= _BV(CC_GDO0_PCIE);											// set interrupt in mask active
}
uint8_t ccSendByte(uint8_t data) {
	SPDR = data;																// send byte
	while (!(SPSR & _BV(SPIF))); 												// wait until transfer finished
	return SPDR;
}
uint8_t ccGetGDO0() {
	uint8_t x = chkPCINT(CC_GDO0_PCIE, CC_GDO0_INT);
	//if (x>1) dbg << "x:" << x << '\n';

	if (x == 2 ) return 1;														// falling edge detected
	else return 0;
}

void    enableGDO0Int(void) {
	//dbg << "enable int\n";
	CC_GDO0_PCMSK |=  _BV(CC_GDO0_INT);
}
void    disableGDO0Int(void) {
	//dbg << "disable int\n";
	CC_GDO0_PCMSK &= ~_BV(CC_GDO0_INT);
}

void    waitMiso(void) {
	while(SPI_PORT &   _BV(SPI_MISO));
}
void    ccSelect(void) {
	setPinLow( CC_CS_PORT, CC_CS_PIN);
}
void    ccDeselect(void) {
	setPinHigh( CC_CS_PORT, CC_CS_PIN);
}
//- -----------------------------------------------------------------------------------------------------------------------


//- status led related functions -------------------------------------------------------------------------------------------------
void    initLeds(void) {
	pinOutput(LED_RED_DDR, LED_RED_PIN);										// set the led pins in port
	pinOutput(LED_GRN_DDR, LED_GRN_PIN);
	if (LED_ACTIVE_LOW) {
		setPinHigh(LED_RED_PORT, LED_RED_PIN);
		setPinHigh(LED_GRN_PORT, LED_GRN_PIN);
	}
}
void    ledRed(uint8_t stat) {
	stat ^= LED_ACTIVE_LOW;
	if      (stat == 1) setPinHigh(LED_RED_PORT, LED_RED_PIN);
	else if (stat == 0) setPinLow(LED_RED_PORT, LED_RED_PIN);
	else                setPinCng(LED_RED_PORT, LED_RED_PIN);
}
void    ledGrn(uint8_t stat) {
	stat ^= LED_ACTIVE_LOW;
	if      (stat == 1) setPinHigh(LED_GRN_PORT, LED_GRN_PIN);
	else if (stat == 0) setPinLow(LED_GRN_PORT, LED_GRN_PIN);
	else                setPinCng(LED_GRN_PORT, LED_GRN_PIN);
}



struct  s_pcINT {
	uint8_t cur;
	uint8_t prev;
	uint32_t time;
} static volatile pcInt[3];

//- pin related functions -------------------------------------------------------------------------------------------------

void    initPCINT(void) {
	memset((uint8_t*)pcInt, 0x00, sizeof(pcInt));
	//dbg << "a: " << pcInt[2].cur << '\n';
}
uint8_t chkPCINT(uint8_t port, uint8_t pin) {
	// returns pin status while no interrupt had happened for the pin, 2 for falling and 3 for rising edge

	uint8_t cur  = pcInt[port].cur  & _BV(pin);
	uint8_t prev = pcInt[port].prev & _BV(pin);

	if ((cur == prev) || ( (getMillis() - pcInt[port].time) < DEBOUNCE )) {		// old and new bit is similar, or DEBOUNCE time is running
		return (pcInt[port].cur & _BV(pin)) ? 1 : 0;
	}

	//if ( (getMillis() - pcInt[port].time) < DEBOUNCE ) {
	//	dbg << "xxxx\n";
	//}

	// detect rising or falling edge
	//dbg << pcInt[port].cur << ' ' << pcInt[port].prev << ' ';
	if (cur) {																	// pin is 1
		pcInt[port].prev |= _BV(pin);											// set bit bit in prev
		//dbg << "y3\n";
		return 3;
	} else {																	// pin is 0
		//dbg << "y2\n";
		pcInt[port].prev &= ~_BV(pin);											// clear bit in prev
		return 2;
	}
}


/************************************
 *** Config key related functions ***
 ************************************/

/**
 * Initialize the config key.
 * Set port pin and register pin interrupt.
 */
void    initConfKey(void) {
	// set port pin and register pin interrupt
	pinInput(CONFIG_KEY_DDR, CONFIG_KEY_PIN);									// init the config key pin
	setPinHigh(CONFIG_KEY_PORT,CONFIG_KEY_PIN);

	initPCINT();																// some sanity on interrupts
	regPCIE(CONFIG_KEY_PCIE);													// set the pin change interrupt
	regPCINT(CONFIG_KEY_PCMSK, CONFIG_KEY_INT);									// description is in hal.h

	//dbg << "pb:" << PINB  << " pc:" << PINC  << " pd:" << PIND << "\n";
	//dbg << "ckDDR:" << CONFIG_KEY_DDR << "ckPort:" << CONFIG_KEY_PORT  << " ckpin:" << CONFIG_KEY_PIN << "\n";

	pcInt[0].cur = PINB;
	pcInt[1].cur = PINC;
	pcInt[2].cur = PIND;
}

//- -----------------------------------------------------------------------------------------------------------------------
ISR (PCINT0_vect) {
	pcInt[0].prev = pcInt[0].cur;
	pcInt[0].cur = PINB;
	pcInt[0].time = getMillis();
	//dbg << "i1:" << PINB  << "\n";
}
ISR (PCINT1_vect) {
	pcInt[1].prev = pcInt[1].cur;
	pcInt[1].cur = PINC;
	pcInt[1].time = getMillis();
	//dbg << "i2:" << PINC << "\n";
}
ISR (PCINT2_vect) {
	pcInt[2].prev = pcInt[2].cur;
	pcInt[2].cur = PIND;
	pcInt[2].time = getMillis();
	//dbg << "i3:" << PIND  << "\n";
}
//- -----------------------------------------------------------------------------------------------------------------------


/*************************************
 *** Battery measurement functions ***
 *************************************/
void    initExtBattMeasurement(void);
void    switchExtBattMeasurement(uint8_t stat);

/**
 * get the voltage off battery
 */
uint8_t  getBatteryVoltage(void) {
	#if defined EXT_BATTERY_MEASUREMENT
		initExtBattMeasurement();
		switchExtBattMeasurement(1);
	#endif

	#if defined EXT_BATTERY_MEASUREMENT
		uint16_t adcValue = getAdcValue(										// Voltage Reference = Internal 1.1V; Input Channel = external battery measure pin
			(1 << REFS1) | (1 << REFS0) | BATT_MEASURE_PIN
		);

		adcValue = adcValue * AVR_BANDGAP_VOLTAGE / 1023 / BATTERY_FACTOR;		// calculate battery voltage in V/10
		switchExtBattMeasurement(0);
	#else
		uint16_t adcValue = getAdcValue(										// Voltage Reference = AVCC with external capacitor at AREF pin; Input Channel = 1.1V (V BG)
			(0 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0)
		);

		adcValue = AVR_BANDGAP_VOLTAGE * 1023 / adcValue / 100;					// calculate battery voltage in V/10
	#endif

	return (uint8_t)adcValue;
}

/**
 * Initialize battery measurement pin for external battery measurement
 */
void    initExtBattMeasurement(void) {
	pinInput(BATT_MEASURE_DDR, BATT_MEASURE_PIN);								// set the ADC pin as input
	pinInput(BATT_ENABLE_DDR, BATT_ENABLE_PIN);									// set the measurement enable pin as input, otherwise we waste energy over the resistor network against VCC
}

/**
 * activate / deactivate battery measurement pin
 * @param	stat	1: activate, 0: deactivate
 */
void    switchExtBattMeasurement(uint8_t stat) {
	if (stat == 1) {
		pinOutput(BATT_ENABLE_DDR, BATT_ENABLE_PIN);							// set pin as out put
		setPinLow(BATT_ENABLE_PORT, BATT_ENABLE_PIN);							// set low to measure the resistor network
		setPinLow(BATT_MEASURE_PORT, BATT_MEASURE_PIN);
	} else {
		pinInput(BATT_ENABLE_DDR, BATT_ENABLE_PIN);

		// todo: check
		setPinHigh(BATT_MEASURE_PORT, BATT_MEASURE_PIN);						// switch on pull up, otherwise we waste energy over the resistor network against VCC
	}
}
