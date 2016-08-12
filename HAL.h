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

	#include "macros.h"
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


	//- timer functions -------------------------------------------------------------------------------------------------------
	#define HAS_OWN_MILLIS_TIMER
	typedef uint32_t tMillis;
	extern void    initMillis(void);
	extern tMillis getMillis(void);
	extern void    addMillis(tMillis ms);
	//- -----------------------------------------------------------------------------------------------------------------------



	/**	- ---------------------------------------------------------------------------------------------------------------------
	*
	* @brief Template and some functions for debugging over serial interface
	* http://aeroquad.googlecode.com/svn/branches/pyjamasam/WIFIReceiver/Streaming.h
	*/

	// template to enable dbg << "some text" << '\n' 
	template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

	struct _HEXB {
		uint8_t val;
		_HEXB(uint8_t v) : val(v) {}
	};
	inline Print &operator <<(Print &obj, const _HEXB &arg) { obj.print(_HI_HEX_BITS(arg.val)); obj.print(_LO_HEX_BITS(arg.val)); return obj; }

	struct _HEX {
		uint8_t *val;
		uint8_t len;
		_HEX(uint8_t *v, uint8_t l) : val(v), len(l) {}
	};
	inline Print &operator <<(Print &obj, const _HEX &arg) { for (uint8_t i = 0; i<arg.len; i++) { obj.print(_HI_HEX_BITS(arg.val[i])); obj.print(_LO_HEX_BITS(arg.val[i])); if (i + 1<arg.len) obj.print(' '); }; return obj; }

	enum _eTIME { _TIME };
	inline Print &operator <<(Print &obj, _eTIME arg) { obj.print('('); obj.print(getMillis()); obj.print(')'); return obj; }

	//extern void    dbgStart(void);
	void    dbgStart(void);
	//- -----------------------------------------------------------------------------------------------------------------------



	//- cc1100 hardware functions ---------------------------------------------------------------------------------------------
	// all functions can be found in HAL_extern.h file, this are only the forward declarations to overcome the problem
	// of handing over #defines from user folder to library folder in Arduino. No pin change interrupt neccasary any more.

	extern void    ccInitHw(void);																// init all hardware pins related to the cc1101 module
	extern uint8_t ccSendByte(uint8_t data);													// SPI send byte function
	extern uint8_t ccGetGDO0(void);																// detects falling edge while received data
	extern void    ccSelect(void);																// chip select and wait till ready
	extern void    ccDeselect(void);															// chip deselect
	//- -----------------------------------------------------------------------------------------------------------------------

	//- led related functions -------------------------------------------------------------------------------------------------
	struct s_blPat {							// struct for defining the blink pattern
		uint8_t len;							// length of pattern string
		uint8_t dur;							// how often the pattern has to be repeated, 0 for endless
		uint8_t led0;							// red
		uint8_t led1;							// green, if you like orange, set led0 and led1 to one
		uint8_t pat[6];							// the pattern it self, pattern starts always with the on time, followed by off time.
	};											// time is given in 10ms steps
	extern const struct s_blPat blPat[];
	extern void    initLeds(void);																// initialize leds
	extern void    ledRed(uint8_t stat);														// function in main sketch to drive leds
	extern void    ledGrn(uint8_t stat);														// stat could be 0 for off, 1 for on, 2 for toggle
	//- -----------------------------------------------------------------------------------------------------------------------

	//- config key related functions ------------------------------------------------------------------------------------------
	extern void    initConfKey(void);															// init the config key, function in user sketch
	extern uint8_t checkConfKey(void);															// checks the conf key if there had something happened
	//- -----------------------------------------------------------------------------------------------------------------------

	//- needed for 32u4 to prevent sleep, while USB didn't work in sleep ------------------------------------------------------
	extern void    initWakeupPin(void);															// init the wakeup pin
	extern uint8_t checkWakeupPin(void);														// we can setup a pin which avoid sleep mode
	//- -----------------------------------------------------------------------------------------------------------------------



	//- pin interrupts --------------------------------------------------------------------------------------------------------
	/**
	* @brief Structure to handle information raised by the interrupt function
	*
	* @param *PINR  Pointer to PIN register, to read the PIN status
	* @param prev   To remember on the previus status of the port, to identify which PIN was raising the interrupt
	* @param mask   Mask byte to clean out bits which are not registered for interrupt detection
	*/
	struct  s_pcint_vector_byte {
		volatile uint8_t *PINR;																		// pointer to the port where pin status can be read
		uint8_t curr;
		uint8_t prev;
		uint8_t mask;
		uint32_t time;
	};
	extern volatile s_pcint_vector_byte pcint_vector_byte[];										// size of the table depending on avr type in the cpp file
	extern void    registerPCINT(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC);
	extern uint8_t checkPCINT(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC, uint8_t debounce);
	extern uint8_t checkPCINT(uint8_t port, uint8_t pin, uint8_t debounce);							// function to poll if an interrupt had happened, gives also status of pin
	extern void    maintainPCINT(uint8_t vec);														// collects all interrupt vectors and maintains the callback address for external pin change interrupt handling
	//- -----------------------------------------------------------------------------------------------------------------------



	//- eeprom functions ------------------------------------------------------------------------------------------------------
	void    initEEProm(void);
	void    getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
	void    setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
	void    clearEEPromBlock(uint16_t addr, uint16_t len);
	//- -----------------------------------------------------------------------------------------------------------------------


	//- power management functions --------------------------------------------------------------------------------------------
	extern void    startWDG32ms(void);
	extern void    startWDG64ms(void);
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
