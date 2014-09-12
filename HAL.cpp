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
void initHW(void) {
	// AVR 328 uses three port addresses, PortB (digital pin 8 to 13), PortC (analog input pins), PortD (digital pins 0 to 7)

	// - define the output pins -----------------------------------------------
	// PortB PB2, PB3, PB4, PB5 already in use for SPI
	// PortD PD0 and PD1 in use for serial port, PD2 in use for GDO0 pin
	DDRB  |= _BV(7) | _BV(6) | _BV(1);
	DDRC  |= _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2) | _BV(1) | _BV(0);
	DDRD  |= _BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3);

	// - define the input pins ------------------------------------------------
	// to get the pin state use "if (PINB & _BV(0))"
	DDRB  &= ~_BV(0);																	// config key depends on PORTB0
	PORTB |= _BV(0);																	// set PORTB0 to high
		
	// - define the pin interrupts --------------------------------------------
	// The PCIEx bits in the PCICR registers enable External Interrupts and tells the MCU to check PCMSKx on a pin 
	// change state.
	// bit           7       6       5       4       3       2       1       0
	// PCICR         -       -       -       -       -     PCIE2   PCIE1   PCIE0
	// Read/Write    R       R       R       R       R      R/W     R/W     R/W
	// Initial Value 0       0       0       0       0       0       0       0	

	// Pin Change Mask Register determines which pins cause the PCINTX interrupt to be triggered.
	// bit           7       6       5       4       3       2       1       0
	// PCMSK0	  PCINT7  PCINT6  PCINT5  PCINT4  PCINT3  PCINT2  PCINT1  PCINT0
	// PCMSK1	     -    PCINT14 PCINT13 PCINT12 PCINT11 PCINT10 PCINT9  PCINT8
	// PCMSK2	  PCINT23 PCINT22 PCINT21 PCINT20 PCINT19 PCINT18 PCINT17 PCINT16
	// Read/Write   R/W     R/W     R/W     R/W     R/W     R/W     R/W     R/W
	// Initial Value 0       0       0       0       0       0       0       0

	// Pin Change Interrupt Flag Register - When a pin changes states (HIGH to LOW, or LOW to HIGH) and the 
	// corresponding PCINTx bit in the PCMSKx register is HIGH the corresponding PCIFx bit in the PCIFR register 
	// is set to HIGH and the MCU jumps to the corresponding Interrupt vector.
	// bit           7       6       5       4       3       2       1       0
	// PCIFR         -       -       -       -       -     PCIF2   PCIF1   PCIF0
}
ISR (PCINT0_vect) {
}
ISR (PCINT1_vect) {
}
ISR (PCINT2_vect) {
}

