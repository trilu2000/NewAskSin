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

	#include <avr/sleep.h>
	#include <avr/power.h>
	#include <avr/wdt.h>
	#include <util/delay.h>
	#include <util/atomic.h>
	#include <avr/eeprom.h>

	#include "Print.h"


	//- MCU dependent HAL definitions -----------------------------------------------------------------------------------------
	#if defined(__AVR_ATmega328P__)
		#include "HAL_atmega328P.h"
	#elif defined(__AVR_ATmega32U4__)
		#include "HAL_atmega32U4.h"
	#else
		#error "No HAL definition for current MCU available!"
	#endif
	//- -----------------------------------------------------------------------------------------------------------------------

	static uint16_t wdtSleep_TIME;

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
	//- -----------------------------------------------------------------------------------------------------------------------

	//- some macros and definitions -------------------------------------------------------------------------------------------
	#define _pgmB(x) pgm_read_byte(&x)											// short hand for PROGMEM read
	#define _pgmW(x) pgm_read_word(&x)

	#define pinOutput(PORT,PIN)  ((PORT) |=  _BV(PIN))							// pin functions
	#define pinInput(PORT,PIN)   ((PORT) &= ~_BV(PIN))
	#define setPinHigh(PORT,PIN) ((PORT) |=  _BV(PIN))
	#define setPinLow(PORT,PIN)  ((PORT) &= ~_BV(PIN))
	#define setPinCng(PORT,PIN)  ((PORT) ^= _BV(PIN))
	#define getPin(PORT,PIN)     ((PORT) &  _BV(PIN))
	//- -----------------------------------------------------------------------------------------------------------------------


	//- timer functions -------------------------------------------------------------------------------------------------------
	typedef uint32_t tMillis;
	extern void    initMillis(void);
	extern tMillis getMillis(void);
	extern void    addMillis(tMillis ms);
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

	extern void    dbgStart(void);
	//- -----------------------------------------------------------------------------------------------------------------------


	//- pin related functions -------------------------------------------------------------------------------------------------
	void    initLeds(void);												// initialize leds
	void    ledRed(uint8_t stat);										// function in main sketch to drive leds
	extern void    ledGrn(uint8_t stat);										// stat could be 0 for off, 1 for on, 2 for toggle

	extern void    initConfKey(void);											// init the config key, function in user sketch

	extern void    initWakeupPin(void);											// init the wakeup pin
	extern uint8_t checkWakeupPin(void);										// we could setup a pin which avoid sleep mode
	//- -----------------------------------------------------------------------------------------------------------------------

	//- pin interrupts --------------------------------------------------------------------------------------------------------
	// http://www.protostack.com/blog/2010/09/external-interrupts-on-an-atmega168/

	#define regPCIE(PORT)         (PCICR |= _BV(PORT))
	#define regPCINT(MASK,PORT)   (MASK  |= _BV(PORT))

	extern void    initPCINT(void);
	extern uint8_t chkPCINT(uint8_t port, uint8_t pin);
	//- -----------------------------------------------------------------------------------------------------------------------


	//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
	extern void    ccInitHw(void);
	extern uint8_t ccSendByte(uint8_t data);
	extern uint8_t ccGetGDO0(void);

	extern void    enableGDO0Int(void);
	extern void    disableGDO0Int(void);

	extern void    waitMiso(void);
	extern void    ccSelect(void);
	extern void    ccDeselect(void);
	//- -----------------------------------------------------------------------------------------------------------------------


	//- eeprom functions ------------------------------------------------------------------------------------------------------
	extern void    initEEProm(void);
	extern void    getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
	extern void    setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
	extern void    clearEEPromBlock(uint16_t addr, uint16_t len);
	//- -----------------------------------------------------------------------------------------------------------------------


	//- power management functions --------------------------------------------------------------------------------------------
	extern void    startWDG32ms(void);
	extern void    startWDG250ms(void);
	extern void    startWDG8000ms(void);
	extern void    setSleep(void);

	extern void    startWDG();
	extern void    stopWDG();
	extern void    setSleepMode();
	//- -----------------------------------------------------------------------------------------------------------------------


	//- battery measurement functions -----------------------------------------------------------------------------------------
	// http://jeelabs.org/2013/05/17/zero-powe-battery-measurement/
	#define BAT_NUM_MESS_ADC                  20								// real measures to get the best average measure
	#define BAT_DUMMY_NUM_MESS_ADC            40								// dummy measures to get the ADC working

	extern uint16_t getAdcValue(uint8_t adcmux);
	uint8_t  getBatteryVoltage(void);
	//- -----------------------------------------------------------------------------------------------------------------------

#endif 
