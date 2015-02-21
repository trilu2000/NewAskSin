#include "HAL.h"

//- some macros for debugging ---------------------------------------------------------------------------------------------

// todo: customize baudrate
// remove mcu dependencies
void dbgStart(void) {
	power_serial_enable();														// enable the debuging port

	if (!(UCSR & (1<<RXEN))) {													// check if serial was already set
		dbg.begin(57600);
		_delay_ms(500);
	}
}
//- -----------------------------------------------------------------------------------------------------------------------


//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------




//- power management functions --------------------------------------------------------------------------------------------
// http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
// http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode
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
	//dbg << ',';																// some debug
	//_delay_ms(10);															// delay is necessary to get it printed on the console before device sleeps
	//_delay_ms(100);

	// some power savings by switching off some CPU functionality
	ADCSRA = 0;																	// disable ADC
	backupPwrRegs();															// save content of power reduction register and set it to all off

	sleep_enable();																// enable sleep
	offBrownOut();																// turn off brown out detection

	sleep_cpu();																// goto sleep
	// sleeping now
	// --------------------------------------------------------------------------------------------------------------------
	// wakeup will be here
	sleep_disable();															// first thing after waking from sleep, disable sleep...
	recoverPwrRegs();															// recover the power reduction register settings
	//dbg << '.';																// some debug
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


//- battery measurement functions -----------------------------------------------------------------------------------------
uint16_t getAdcValue(uint8_t adcmux) {
	uint16_t adcValue = 0;

	#if defined(__AVR_ATmega32U4__)												// save content of Power Reduction Register
		uint8_t tmpPRR0 = PRR0;
		uint8_t tmpPRR1 = PRR1;
	#else
		uint8_t tmpPRR = PRR;
	#endif
	power_adc_enable();

	ADMUX = adcmux;																// start ADC
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);							// Enable ADC and set ADC pre scaler

	for (uint8_t i = 0; i < BAT_NUM_MESS_ADC + BAT_DUMMY_NUM_MESS_ADC; i++) {	// take samples in a round
		ADCSRA |= (1 << ADSC);													// start conversion
		while (ADCSRA & (1 << ADSC)) {}											// wait for conversion complete

		if (i >= BAT_DUMMY_NUM_MESS_ADC) {										// we discard the first dummy measurements
			adcValue += ADCW;
		}
	}

	ADCSRA &= ~(1 << ADEN);														// ADC disable
	adcValue = adcValue / BAT_NUM_MESS_ADC;										// divide adcValue by amount of measurements

	#if defined(__AVR_ATmega32U4__)												// restore power management
		PRR0 = tmpPRR0;
		PRR1 = tmpPRR1;
	#else
		PRR = tmpPRR;
	#endif

	ADCSRA = 0;																	// ADC off

	//dbg << "x:" << adcValue << '\n';

	return adcValue;															// return the measured value
}
//- -----------------------------------------------------------------------------------------------------------------------
