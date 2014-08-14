//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _HAL_H
#define _HAL_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

//#include <avr/power.h>
//#include <avr/sleep.h>
//#include <avr/wdt.h>


//- some macros and definitions -------------------------------------------------------------------------------------------
#define _pgmB(x) pgm_read_byte(&x)															// short hand for PROGMEM read
#define _pgmW(x) pgm_read_word(&x)


//- some macros for debugging ---------------------------------------------------------------------------------------------
#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }
#define pHexB(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)<< char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)
#define pHex(x,y) char(0);for(uint8_t i=0;i<y;i++)dbg<<pHexB(x[i])<<char(i+1<y?32:0);dbg<<char(0)
#define pLine     char(0);for(uint8_t i=0;i<40;i++)dbg<<'-';dbg<<'\n'


//- eeprom functions ------------------------------------------------------------------------------------------------------
void initEEProm(void);
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void clearEEPromBlock(uint16_t addr, uint16_t len);


//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PORTB2
#define SPI_MISO		PORTB4
#define SPI_MOSI		PORTB3
#define SPI_SCLK        PORTB5

#define CC1100_CS_DDR   SPI_DDR
#define CC1100_CS_PORT  SPI_PORT
#define CC1100_CS_PIN   SPI_SS

#define CC1100_IN_DDR   DDRD
#define CC1100_IN_PORT  PIND
#define CC1100_IN_PIN   PORTD2

#define CC1100_INT      INT0
#define CC1100_INTVECT  INT0_vect

#define _setBit(PORT, BITNUM)  ((PORT) |= (1<<(BITNUM)))
#define _clrBit(PORT, BITNUM)  ((PORT) &= ~(1<<(BITNUM)))
#define _tglBit(PORT, BITNUM)  ((PORT) ^= (1<<(BITNUM)))
#define _chkBit(PORT, BITNUM)  ((PORT) & (1<<(BITNUM)))

#define _waitMiso   while(_chkBit(SPI_PORT,SPI_MISO))										// wait until SPI MISO line goes low
#define _ccDeselect  _setBit( CC1100_CS_PORT, CC1100_CS_PIN )
#define _ccSelect    _clrBit( CC1100_CS_PORT, CC1100_CS_PIN )

#define _disableGDO0Int  EIMSK &= ~(1<<INT0);
#define _enableGDO0Int   EIMSK |= (1<<INT0);

void    ccInitHw(void);
uint8_t ccSendByte(uint8_t data);
uint8_t ccGDO0(void);

//- timer functions -------------------------------------------------------------------------------------------------------
#define REG_TCCRA		TCCR0A
#define REG_TCCRB		TCCR0B
#define REG_TIMSK		TIMSK0
#define REG_OCR			OCR0A
#define BIT_OCIE		OCIE0A
#define BIT_WGM			WGM01
#define CLOCKSEL        (_BV(CS01)|_BV(CS00))
#define PRESCALER       64
#define ISR_VECT		TIMER0_COMPA_vect

#define SET_TCCRA()	    (REG_TCCRA = _BV(BIT_WGM))
#define SET_TCCRB()	    (REG_TCCRB = CLOCKSEL)

typedef unsigned long millis_t;
void millis_init(void);
millis_t millis_get(void);
void millis_add(millis_t ms);


#endif 
