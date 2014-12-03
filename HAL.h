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

#define pinOutput(PORT,PIN)  ((PORT) |=  _BV(PIN))											// pin functions
#define pinInput(PORT,PIN)   ((PORT) &= ~_BV(PIN))
#define setPinHigh(PORT,PIN) ((PORT) |=  _BV(PIN))
#define setPinLow(PORT,PIN)  ((PORT) &= ~_BV(PIN))
#define setPinCng(PORT,PIN)  ((PORT) ^= _BV(PIN))
#define getPin(PORT,PIN)     ((PORT) &  _BV(PIN))
//- -----------------------------------------------------------------------------------------------------------------------

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

typedef uint32_t tMillis;
void    initMillis(void);
tMillis getMillis(void);
void    addMillis(tMillis ms);
//- -----------------------------------------------------------------------------------------------------------------------

//- some macros for debugging ---------------------------------------------------------------------------------------------
// http://aeroquad.googlecode.com/svn/branches/pyjamasam/WIFIReceiver/Streaming.h
#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

#define hiHexB(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)
#define loHexB(x)  char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)

struct _HEXB {
	uint8_t val;
	_HEXB(uint8_t v): val(v) {}
};
inline Print &operator <<(Print &obj, const _HEXB &arg) { obj.print(hiHexB(arg.val)); obj.print(loHexB(arg.val)); return obj; }

struct _HEX {
	uint8_t *val;
	uint8_t len;
	_HEX(uint8_t *v, uint8_t l): val(v), len(l) {}
};
inline Print &operator <<(Print &obj, const _HEX &arg) { for(uint8_t i=0;i<arg.len;i++)	{ obj.print(hiHexB(arg.val[i])); obj.print(loHexB(arg.val[i])); if(i+1<arg.len) obj.print(' '); }; return obj; }

enum _eTIME { _TIME };
inline Print &operator <<(Print &obj, _eTIME arg) { obj.print('('); obj.print(getMillis()); obj.print(')'); return obj; }

void dbgStart(void);
//- -----------------------------------------------------------------------------------------------------------------------

//- pin interrupts --------------------------------------------------------------------------------------------------------
// http://www.protostack.com/blog/2010/09/external-interrupts-on-an-atmega168/

#define debounce 5
#define regPCIE(PORT)         (PCICR |= _BV(PORT))
#define regPCINT(MASK,PORT)   (MASK  |= _BV(PORT))

void    initPCINT(void);
uint8_t chkPCINT(uint8_t port, uint8_t pin);
//- -----------------------------------------------------------------------------------------------------------------------

//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
#if defined(__AVR_ATmega32U4__)
	#define SPI_PORT		PORTB															// SPI port definition
	#define SPI_DDR			DDRB
	#define SPI_MISO		PORTB3
	#define SPI_MOSI		PORTB2
	#define SPI_SCLK        PORTB1

	#define CC_CS_PORT		PORTB															// SPI chip select definition
	#define CC_CS_DDR		DDRB
	#define CC_CS_PIN		PORTB4

	#define CC_GDO0_DDR     DDRB															// GDO0 pin, signals received data
	#define CC_GDO0_PORT    PINB
	#define CC_GDO0_PIN     PORTB5

	#define CC_GDO0_PCICR   PCICR															// GDO0 interrupt register
	#define CC_GDO0_PCIE    PCIE0
	#define CC_GDO0_PCMSK   PCMSK0															// GDO0 interrupt mask
	#define CC_GDO0_INT     PCINT5															// pin interrupt
	
#else
	#define SPI_PORT		PORTB															// SPI port definition
	#define SPI_DDR			DDRB
	#define SPI_MISO		PORTB4
	#define SPI_MOSI		PORTB3
	#define SPI_SCLK        PORTB5

	#define CC_CS_PORT		PORTB															// SPI chip select definition
	#define CC_CS_DDR		DDRB
	#define CC_CS_PIN		PORTB2

	#define CC_GDO0_DDR     DDRD															// GDO0 pin, signals received data
	#define CC_GDO0_PORT    PIND
	#define CC_GDO0_PIN     PORTD2

	#define CC_GDO0_PCICR   PCICR															// GDO0 interrupt register
	#define CC_GDO0_PCIE    PCIE2
	#define CC_GDO0_PCMSK   PCMSK2															// GDO0 interrupt mask
	#define CC_GDO0_INT     PCINT18															// pin interrupt

#endif

#define _waitMiso        while(SPI_PORT &   _BV(SPI_MISO))									// wait until SPI MISO line goes low
#define _ccDeselect      setPinHigh( CC_CS_PORT, CC_CS_PIN)
#define _ccSelect        setPinLow(  CC_CS_PORT, CC_CS_PIN)

#define _disableGDO0Int  CC_GDO0_PCMSK &= ~_BV(CC_GDO0_INT);								// disables and enables interrupt
#define _enableGDO0Int   CC_GDO0_PCMSK |=  _BV(CC_GDO0_INT);

void    ccInitHw(void);
uint8_t ccSendByte(uint8_t data);
uint8_t ccGetGDO0(void);
//- -----------------------------------------------------------------------------------------------------------------------

//- eeprom functions ------------------------------------------------------------------------------------------------------
void initEEProm(void);
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void clearEEPromBlock(uint16_t addr, uint16_t len);
//- -----------------------------------------------------------------------------------------------------------------------

//- pin related functions -------------------------------------------------------------------------------------------------
// AVR 328 uses three port addresses, PortB (digital pin 8 to 13), PortC (analog input pins), PortD (digital pins 0 to 7)

#define led0_on()       ledRed(1)
#define led0_off()      ledRed(0)
#define led0_cng()      ledRed(2)
#define led1_on()       ledGrn(1)
#define led2_off()      ledGrn(0)
#define led3_cng()      ledGrn(2)

extern void initLeds(void);																	// initialize leds
extern void ledRed(uint8_t stat);															// function in main sketch to drive leds
extern void ledGrn(uint8_t stat);															// stat could be 0 for off, 1 for on, 2 for toggle
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
//- -----------------------------------------------------------------------------------------------------------------------

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
