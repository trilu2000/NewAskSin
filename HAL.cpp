//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------


#include "HAL.h"
#include "00_debug-flag.h"




//- randum number functions -----------------------------------------------------------------------------------------------
void get_random(uint8_t *buf) {
	/* not random, but most likely, as real random takes 200 byte more */

	uint32_t time = get_millis();												// get current time 
	memcpy(buf, (uint8_t*)&time, 4);											// copy the time into the array
	memcpy(buf + 2, (uint8_t*)&time, 4);
	for (uint8_t i = 0; i < 6; i++) {											// do some xors and byte shift
		if (i) buf[i] ^= buf[i - 1];
		else buf[0] ^= 0x35;
		if (buf[i] & 1) buf[i] = (buf[i] >> 1) ^ 0xA0;
		else buf[i] = (buf[i] >> 1);
	}
}
//- -----------------------------------------------------------------------------------------------------------------------



// todo: customize baudrate
// remove mcu dependencies

//- -----------------------------------------------------------------------------------------------------------------------


//- power management functions --------------------------------------------------------------------------------------------
// http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
// http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode
void    startWDG32ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP0);
	wdtSleep_TIME = 32;
}
void    startWDG64ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP1);
	wdtSleep_TIME = 64;
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
	add_millis(wdtSleep_TIME);
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



//- -----------------------------------------------------------------------------------------------------------------------
