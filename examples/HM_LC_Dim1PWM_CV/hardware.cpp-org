#include "hardware.h"


//- some macros for debugging ---------------------------------------------------------------------------------------------
void dbgStart(void) {

#if defined(__AVR_ATmega32U4__)
	pinOutput(DDRB, PINB0);																	// pin output, otherwise USB will not work
	power_usb_enable();																		// enable the USB port

	if (!(UCSR1B & (1<<RXEN1))) {
		dbg.begin(57600);																	// check if serial was already set
		//while(!dbg);																		// wait until serial has connected
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
	}
#else
	power_usart0_enable();																	// serial port for debugging
	if (!(UCSR0B & (1<<RXEN0))) dbg.begin(57600);											// check if serial was already set
#endif
}
//- -----------------------------------------------------------------------------------------------------------------------


//- pin related functions -------------------------------------------------------------------------------------------------
void    initLeds(void) {
	pinOutput(ledRedDDR,ledRedPin);															// set the led pins in port
	pinOutput(ledGrnDDR,ledGrnPin);
	if (ledActiveLow) {
		setPinHigh(ledRedPort, ledRedPin);
		setPinHigh(ledGrnPort, ledGrnPin);
	}
}
void    ledRed(uint8_t stat) {
	stat ^= ledActiveLow;
	if      (stat == 1) setPinHigh(ledRedPort, ledRedPin);
	else if (stat == 0) setPinLow(ledRedPort, ledRedPin);
	else                setPinCng(ledRedPort, ledRedPin);
}
void    ledGrn(uint8_t stat) {
	stat ^= ledActiveLow;
	if      (stat == 1) setPinHigh(ledGrnPort, ledGrnPin);
	else if (stat == 0) setPinLow(ledGrnPort, ledGrnPin);
	else                setPinCng(ledGrnPort, ledGrnPin);
}

void    initConfKey(void) {
	// set port pin and register pin interrupt
	pinInput(confKeyDDR, confKeyPin);														// init the config key pin
	setPinHigh(confKeyPort,confKeyPin);

	initPCINT();																			// some sanity on interrupts
	regPCIE(confKeyPCIE);																	// set the pin change interrupt
	regPCINT(confKeyPCMSK,confKeyINT);														// description is in hal.h
}

void    initWakeupPin(void) {
	#if defined(wakeupDDR)

	pinInput(wakeupDDR, wakeupPIN);															// set pin as input
	setPinHigh(wakeupPRT, wakeupPIN);														// enable internal pull up

	#endif
}

uint8_t checkWakeupPin(void) {
	// to enable the USB port for upload, configure PE2 as input and check if it is 0, this will avoid sleep mode and enable program upload via serial
	#if defined(wakeupDDR)

	if (getPin(wakeupPNR, wakeupPIN)) return 1;												// return pin is active
	#endif

	return 0;																				// normal operation
}
//- -----------------------------------------------------------------------------------------------------------------------


//- pin interrupts --------------------------------------------------------------------------------------------------------
#define debounce 5

struct  s_pcINT {
	uint8_t cur;
	uint8_t prev;
	uint32_t time;
} static volatile pcInt[3];
void    initPCINT(void) {
	memset((uint8_t*)pcInt, 0x00, sizeof(pcInt));
	//dbg << "a: " << pcInt[2].cur << '\n';
}
uint8_t chkPCINT(uint8_t port, uint8_t pin) {
	// returns pin status while no interrupt had happened for the pin, 2 for falling and 3 for rising edge

	uint8_t cur  = pcInt[port].cur  & _BV(pin);
	uint8_t prev = pcInt[port].prev & _BV(pin);

	if ((cur == prev) || ( (getMillis() - pcInt[port].time) < debounce )) {																		// old and new bit is similar, or debounce time is running
		return (pcInt[port].cur & _BV(pin))?1:0;
	}
	
	//if ( (getMillis() - pcInt[port].time) < debounce ) {
	//	dbg << "xxxx\n";
	//} 

	// detect rising or falling edge
	//dbg << pcInt[port].cur << ' ' << pcInt[port].prev << ' ';
	if (cur) {																				// pin is 1
		pcInt[port].prev |= _BV(pin);														// set bit bit in prev
		//dbg << "y3\n";
		return 3;
	} else {																				// pin is 0
		//dbg << "y2\n";
		pcInt[port].prev &= ~_BV(pin);														// clear bit in prev
		return 2;
	}
}

ISR (PCINT0_vect) {
	pcInt[0].prev = pcInt[0].cur;
	pcInt[0].cur = PINB;
	pcInt[0].time = getMillis();
	//dbg << '.';
	//dbg << pcInt[0].cur << ' ' << pcInt[0].prev << '\n';
}
ISR (PCINT1_vect) {
	pcInt[1].prev = pcInt[1].cur;
	pcInt[1].cur = PINC;
	pcInt[1].time = getMillis();
	//dbg << ',';
}
ISR (PCINT2_vect) {
	pcInt[2].prev = pcInt[2].cur;
	pcInt[2].cur = PIND;
	pcInt[2].time = getMillis();
	//dbg << ';';
}
//- -----------------------------------------------------------------------------------------------------------------------


//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
void    ccInitHw(void) {

	pinInput(  CC_GDO0_DDR, CC_GDO0_PIN );													// set GDO0 as input
	pinOutput( CC_CS_DDR, CC_CS_PIN );														// set chip select as output

	pinOutput( SPI_DDR, SPI_MOSI );															// set MOSI as output
	pinOutput( SPI_DDR, SPI_SCLK );															// set SCK as output
	pinInput(  SPI_DDR, SPI_MISO );															// set MISO as input

	SPCR = _BV(SPE) | _BV(MSTR);// | _BV(SPR0);// | _BV(SPR1); 								// SPI enable, master, speed = CLK/4
	//SPSR &= ~_BV(SPI2X);
	
	CC_GDO0_PCICR |= _BV(CC_GDO0_PCIE);														// set interrupt in mask active
	//CC_GDO0_PCMSK |=  _BV(CC_GDO0_INT);
}
uint8_t ccSendByte(uint8_t data) {
	SPDR = data;																			// send byte
	while (!(SPSR & _BV(SPIF))); 															// wait until transfer finished
	return SPDR;
}
uint8_t ccGetGDO0() {
	uint8_t x = chkPCINT(CC_GDO0_PCIE, CC_GDO0_INT);
	//if (x>1) dbg << "x:" << x << '\n';
	
	if (x == 2 ) return 1;																	// falling edge detected
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
	setPinLow(  CC_CS_PORT, CC_CS_PIN);
}
void    ccDeselect(void) {
	setPinHigh( CC_CS_PORT, CC_CS_PIN);
}
//- -----------------------------------------------------------------------------------------------------------------------


//- timer functions -------------------------------------------------------------------------------------------------------
static volatile tMillis milliseconds;
void    initMillis() {
	SET_TCCRA();
	SET_TCCRB();
	REG_TIMSK = _BV(BIT_OCIE);
	REG_OCR = ((F_CPU / PRESCALER) / 1000);
}
tMillis getMillis() {
	tMillis ms;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ms = milliseconds;
	}
	return ms;
}
void    addMillis(tMillis ms) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		milliseconds += ms;
	}
}
ISR(ISR_VECT) {
	++milliseconds;
}
//- -----------------------------------------------------------------------------------------------------------------------


//- eeprom functions ------------------------------------------------------------------------------------------------------
void    initEEProm(void) {
	// place the code to init a i2c eeprom
}
void    getEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	eeprom_read_block((void*)ptr,(const void*)addr,len);									// AVR GCC standard function
}
void    setEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	eeprom_write_block((const void*)ptr,(void*)addr,len);									// AVR GCC standard function
}
void    clearEEPromBlock(uint16_t addr, uint16_t len) {
	uint8_t tB=0;
	for (uint16_t l = 0; l < len; l++) {													// step through the bytes of eeprom
		setEEPromBlock(addr+l,1,(void*)&tB);
	}
}
//- -----------------------------------------------------------------------------------------------------------------------


//- power management functions --------------------------------------------------------------------------------------------
// http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
// http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode

static uint16_t wdtSleep_TIME;

void    startWDG32ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP0);
	wdtSleep_TIME = 32;
}
void    startWDG250ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP2);
	wdtSleep_TIME = 256;
}
void    startWDG8000ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP3) | (1<<WDP0);
	wdtSleep_TIME = 8192;
}
void    setSleep(void) {
	//dbg << ',';																			// some debug
	//_delay_ms(10);																		// delay is necessary to get it printed on the console before device sleeps
	//_delay_ms(100);

	// some power savings by switching off some CPU functionality
	ADCSRA = 0;																				// disable ADC
	backupPwrRegs();																		// save content of power reduction register and set it to all off
	
	sleep_enable();																			// enable sleep
	offBrownOut();																			// turn off brown out detection
	
	sleep_cpu();																			// goto sleep
	// sleeping now
	// --------------------------------------------------------------------------------------------------------------------
	// wakeup will be here
	sleep_disable();																		// first thing after waking from sleep, disable sleep...
	recoverPwrRegs();																		// recover the power reduction register settings
	//dbg << '.';																			// some debug
}

void    startWDG() {
	WDTCSR = (1<<WDIE);
}
void    stopWDG() {
	WDTCSR &= ~(1<<WDIE);
}
void    setSleepMode() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

ISR(WDT_vect) {
	// nothing to do, only for waking up
	addMillis(wdtSleep_TIME);
}
//- -----------------------------------------------------------------------------------------------------------------------


//- battery measurement functions -----------------------------------------------------------------------------------------
uint16_t getAdcValue(uint8_t admux) {
	uint16_t adcValue = 0;
	
	#if defined(__AVR_ATmega32U4__)															// save content of Power Reduction Register
		uint8_t tmpPRR0 = PRR0;
		uint8_t tmpPRR1 = PRR1;
	#else
		uint8_t tmpPRR = PRR;
	#endif
	power_adc_enable();

	ADMUX = (admux);																		// start ADC
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);										// Enable ADC and set ADC pre scaler

	for (uint8_t i = 0; i < BATTERY_NUM_MESS_ADC + BATTERY_DUMMY_NUM_MESS_ADC; i++) {		// take samples in a round
		ADCSRA |= (1 << ADSC);																// start conversion
		while (ADCSRA & (1 << ADSC)) {}														// wait for conversion complete

		if (i >= BATTERY_DUMMY_NUM_MESS_ADC) {												// we discard the first dummy measurements
			adcValue += ADCW;
		}
	}

	ADCSRA &= ~(1 << ADEN);																	// ADC disable
	adcValue = adcValue / BATTERY_NUM_MESS_ADC;												// divide adcValue by amount of measurements

	#if defined(__AVR_ATmega32U4__)															// restore power management
		PRR0 = tmpPRR0;
		PRR1 = tmpPRR1;
	#else
		PRR = tmpPRR;
	#endif

	ADCSRA = 0;																				// ADC off
	return adcValue;																		// return the measured value
}
uint8_t getBatteryVoltageInternal(void) {

	uint32_t adcValue = (uint32_t)getAdcValue(
	#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
		_BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1)
	#else
		_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1)
	#endif
	);

	//dbg << "x:" << adcValue << '\n';
	adcValue = AVR_BANDGAP_VOLTAGE * 1023 / adcValue / 100;									// calculate battery voltage in V/10
	return (uint8_t)adcValue;
}
uint8_t getBatteryVoltageExternal(void) {
	/*enableBattery();																		// set pin to low, to make it active
	
	uint32_t adcValue = (uint32_t)getAdcValue(												// ask the ADC
	//	(1 << REFS1) | (1 << REFS0) | PORTF7 //,														// Voltage Reference = Internal 1.1V Voltage Reference
	0																					// pin 0 on PORTC
	);

	disableBattery();																		// measurement pin to input to save battery
	//dbg << "x:" << adcValue << '\n';
	adcValue *= BATTERY_FACTOR; adcValue /= 1000;											// calculate the V/10 and return
	return (uint8_t)adcValue;*/
}

void    initExtBattMeasurement(void) {
	pinInput(battMeasDDR, battMeasPin);														// set the ADC pin as input
	setPinHigh(battMeasPort, battMeasPin);													// switch on pull up, otherwise we waste energy over the resistor network against VCC
	pinInput(battEnblDDR, battEnblPin);														// set the measurement enable pin as input, otherwise we waste energy over the resistor network against VCC
}
void    switchExtBattMeasurement(uint8_t stat) {
	if (stat) {
		pinOutput(battEnblDDR, battEnblPin);												// set pin as out put
		setPinLow(battEnblPort, battEnblPin);												// set low to measure the resistor network
	} else pinInput(battEnblDDR, battEnblPin);
}
//- -----------------------------------------------------------------------------------------------------------------------
