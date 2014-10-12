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

#include <stdint.h>
#include <stdlib.h>

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>


//- some macros and definitions -------------------------------------------------------------------------------------------
#define _pgmB(x) pgm_read_byte(&x)															// short hand for PROGMEM read
#define _pgmW(x) pgm_read_word(&x)


//- some macros for debugging ---------------------------------------------------------------------------------------------
#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }
#define pHexB(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)<< char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)
#define pHex(x,y) char(0);for(uint8_t i=0;i<y;i++)dbg<<pHexB(x[i])<<char(i+1<y?32:0);dbg<<char(0)
#define pLine     char(0);for(uint8_t i=0;i<40;i++)dbg<<'-';dbg<<'\n'
#define pTime     char(0);dbg<<'('<<getMillis()<<')'

void dbgStart(void);


//- eeprom functions ------------------------------------------------------------------------------------------------------
void initEEProm(void);
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void clearEEPromBlock(uint16_t addr, uint16_t len);


//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
#define SPI_PORT		PORTB																// SPI port definition
#define SPI_DDR			DDRB
#define SPI_SS			PORTB2
#define SPI_MISO		PORTB4
#define SPI_MOSI		PORTB3
#define SPI_SCLK        PORTB5

#define CC1100_CS_DDR   SPI_DDR
#define CC1100_CS_PORT  SPI_PORT
#define CC1100_CS_PIN   SPI_SS

#define CC1100_IN_DDR   DDRD																// GDO0 pin, signals received data 
#define CC1100_IN_PORT  PIND
#define CC1100_IN_PIN   PORTD2

#define _waitMiso        while(SPI_PORT &   _BV(SPI_MISO))									// wait until SPI MISO line goes low
#define _ccDeselect      CC1100_CS_PORT |=  _BV(CC1100_CS_PIN) 
#define _ccSelect        CC1100_CS_PORT &= ~_BV(CC1100_CS_PIN) 

#define _disableGDO0Int  EIMSK &= ~_BV(INT0);												// disables and enables interrupt
#define _enableGDO0Int   EIMSK |=  _BV(INT0);

void    ccInitHw(void);
uint8_t ccSendByte(uint8_t data);
uint8_t ccGetGDO0(void);
void    ccSetGDO0(void);


//- timer functions -------------------------------------------------------------------------------------------------------
// https://github.com/zkemble/millis/blob/master/millis/
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

typedef uint32_t millis_t;
void     initMillis(void);
millis_t getMillis(void);
void     addMillis(millis_t ms);


//- pin related functions -------------------------------------------------------------------------------------------------
// AVR 328 uses three port addresses, PortB (digital pin 8 to 13), PortC (analog input pins), PortD (digital pins 0 to 7)
#define led0_on()		PORTD |=  _BV(6)
#define led0_off()		PORTD &= ~_BV(6)
#define led0_cng()		PORTD ^=  _BV(6)

#define led1_on()		PORTD |=  _BV(4)
#define led1_off()		PORTD &= ~_BV(4)
#define led1_cng()		PORTD ^=  _BV(4)

#define pinOutput(PORT,PIN)  ((PORT) |=  _BV(PIN))
#define pinInput(PORT,PIN)   ((PORT) &= ~_BV(PIN))
#define setPinHigh(PORT,PIN) ((PORT) |=  _BV(PIN))
#define setPinLow(PORT,PIN)  ((PORT) &= ~_BV(PIN))
#define getPin(PORT,PIN)     ((PORT) &  _BV(PIN))

// define the pin interrupts
// http://www.protostack.com/blog/2010/09/external-interrupts-on-an-atmega168/

#define regPCIE(PORT)         (PCICR |= _BV(PORT))
#define regPCINT(MASK,PORT)   (MASK  |= _BV(PORT))

uint8_t chkPCINT(uint8_t port, uint8_t pin);
//- -----------------------------------------------------------------------------------------------------------------------


//- power management functions --------------------------------------------------------------------------------------------
// http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
// http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode

#define startWDG()            WDTCSR = (1<<WDIE)     
#define stopWDG()             WDTCSR &= ~(1<<WDIE)
#define setSleepMode()        set_sleep_mode(SLEEP_MODE_PWR_DOWN)

void startWDG32ms(void);
void startWDG250ms(void);
void startWDG8000ms(void);
void setSleep(void);


//- battery measurement functions -----------------------------------------------------------------------------------------
// http://jeelabs.org/2013/05/17/zero-powe-battery-measurement/

#define BATTERY_NUM_MESS_ADC              20												// real measures to get the best average measure
#define BATTERY_DUMMY_NUM_MESS_ADC        40												// dummy measures to get the ADC working
#define AVR_BANDGAP_VOLTAGE               1100UL											// band gap reference for Atmega328p
#define BATTERY_FACTOR                    121												// see excel table

#define BATTERY_MODE_BANDGAP_MESSUREMENT  1
#define BATTERY_MODE_EXTERNAL_MESSUREMENT 2

#define enableBattery()       pinOutput(DDRD, 7); setPinLow(PORTD, 7)																// to low status, so measurement could be taken
#define disableBattery()      pinInput(DDRD, 7);

uint16_t getAdcValue(uint8_t voltageReference, uint8_t inputChannel);
uint8_t  getBatteryVoltageInternal(void);
uint8_t  getBatteryVoltageExternal(void);

//- -----------------------------------------------------------------------------------------------------------------------

#endif 
