//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
#include "HAL.h"


//- some macros for debugging ---------------------------------------------------------------------------------------------
void dbgStart(void) {
	if (!(UCSR0B & (1<<RXEN0))) dbg.begin(57600);
}


//- eeprom functions ------------------------------------------------------------------------------------------------------
void initEEProm(void) {
	// place the code to init a i2c eeprom
}
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	eeprom_read_block((void*)ptr,(const void*)addr,len);								// AVR GCC standard function
}
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	eeprom_write_block((const void*)ptr,(void*)addr,len);								// AVR GCC standard function
}
void clearEEPromBlock(uint16_t addr, uint16_t len) {
	uint8_t tB=0;
	for (uint16_t l = 0; l < len; l++) {												// step through the bytes of eeprom
		setEEPromBlock(addr+l,1,(void*)&tB);
	}
}


//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
static volatile uint8_t gdo0 = 0;
void    ccInitHw(void) {
	CC1100_IN_DDR &= ~_BV (CC1100_IN_PIN);												// GDO0 input
	CC1100_CS_DDR |= _BV (CC1100_CS_PIN);												// CS output

	SPI_DDR |= _BV (SPI_MOSI) | _BV (SPI_SCLK);											// MOSI, SCK output
	SPI_DDR &= ~_BV (SPI_MISO);															// MISO input

	SPCR = _BV(SPE) | _BV(MSTR);														// SPI enable, master, speed = CLK/4

    EICRA |= _BV(ISC01);																// set INT0 to trigger on falling edge
}
uint8_t ccSendByte(uint8_t data) {
	SPDR = data;																		// send byte
	while (!(SPSR & _BV(SPIF)));														// wait until transfer finished
	return SPDR;
}
uint8_t ccGetGDO0() {
	if (gdo0 == 1) {
		gdo0 = 0;
		return 1;
	}
	return gdo0;
}
void    ccSetGDO0() {
	gdo0 = 1;
}
ISR(INT0_vect) {
	//_disableGDO0Int;
	gdo0 = 1;
	//_enableGDO0Int;
}


//- timer functions -------------------------------------------------------------------------------------------------------
static volatile millis_t milliseconds;
void     initMillis() {
	SET_TCCRA();
	SET_TCCRB();
	REG_TIMSK = _BV(BIT_OCIE);
	REG_OCR = ((F_CPU / PRESCALER) / 1000);
}
millis_t getMillis() {
	millis_t ms;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ms = milliseconds;
	}
	return ms;
}
void     addMillis(millis_t ms) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		milliseconds += ms;
	}
}
ISR(ISR_VECT) {
	++milliseconds;
}


//- pin related functions -------------------------------------------------------------------------------------------------
struct s_pcINT {
	uint8_t cur = 0xff;
	uint8_t prev = 0xff;
	uint32_t time;
} static volatile pcInt[3];
uint8_t chkPCINT(uint8_t port, uint8_t pin) {
	// returns 0 or 1 for pin level and
	// 2 for falling and 3 for rising edge
	
	// old and new bit is similar, return current status
	if ( (pcInt[port].cur & _BV(pin)) == (pcInt[port].prev & _BV(pin)) ) return pcInt[port].cur & _BV(pin);

	// check for debounce time, if still running return previous status
	if (pcInt[port].time > getMillis()) return pcInt[port].prev & _BV(pin);
	
	// detect rising or falling edge
	if (pcInt[port].cur & _BV(pin)) {
		pcInt[port].prev |= _BV(pin);													// set bit bit in prev
		return 3;
	} else {
		pcInt[port].prev &= ~_BV(pin);													// clear bit in prev
		return 2;
	}
}

#define debounce 5
ISR (PCINT0_vect) {
	pcInt[0].cur = PINB;
	pcInt[0].time = getMillis()+debounce;
}
ISR (PCINT1_vect) {
	pcInt[1].cur = PINC;
	pcInt[1].time = getMillis()+debounce;
}
ISR (PCINT2_vect) {
	pcInt[2].cur = PIND;
	pcInt[2].time = getMillis()+debounce;
}


//- power management functions --------------------------------------------------------------------------------------------
//static volatile uint8_t wdtSleep;
static uint16_t wdtSleepTime;

void    startWDG32ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP0);
	wdtSleepTime = 32;
}
void    startWDG250ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP2);
	wdtSleepTime = 256;
}
void    startWDG8000ms(void) {
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDIE) | (1<<WDP3) | (1<<WDP0);
	wdtSleepTime = 8192;
}
void    setSleep(void) {
	//dbg << ',';																		// some debug
	//_delay_ms(10);																	// delay is necessary to get it printed on the console before device sleeps
	//_delay_ms(100);

	// some power savings by switching off some CPU functionality
	ADCSRA = 0;																			// disable ADC
	uint8_t xPrr = PRR;																	// save content of Power Reduction Register
	PRR = 0xFF;																			// turn off various modules

	sleep_enable();																		// enable sleep
	MCUCR = (1<<BODS)|(1<<BODSE);														// turn off brown-out enable in software
	MCUCR = (1<<BODS);																	// must be done right before sleep

	sleep_cpu();																		// goto sleep
	// sleeping now
	// --------------------------------------------------------------------------------------------------------------------
	// wakeup will be here
	sleep_disable();																	// first thing after waking from sleep, disable sleep...

	PRR = xPrr;																			// restore power management
	//dbg << '.';																		// some debug
}

ISR(WDT_vect) {
	// nothing to do, only for waking up
	milliseconds += wdtSleepTime;
}

//- -----------------------------------------------------------------------------------------------------------------------
