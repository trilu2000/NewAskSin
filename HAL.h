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

	//- include and typedef for delegate ------------------------------------------------------------------------ 
	#include "Fastdelegate.h"																		// include the fastdelegate library

	using namespace fastdelegate;
	typedef FastDelegate2<uint8_t, uint8_t> s_pcint_dlgt;											// void function_name(uint8_t pcint_pin, uint8_t pcint_mode)

	/**
	* @brief Structure to handle information raised by the interrupt function
	*
	* @param *PINR  Pointer to PIN register, to read the PIN status
	* @param prev   To remember on the previus status of the port, to identify which PIN was raising the interrupt
	* @param mask   Mask byte to clean out bits which are not registered for interrupt detection
	*/
	struct  s_pcint_vector_byte {
		volatile uint8_t *PINR;																		// pointer to the port where pin status can be read
		uint8_t prev;
		uint8_t mask;
	};
	extern volatile s_pcint_vector_byte pcint_vector_byte[];										// size of the table depending on avr type in the cpp file

	/**
	* @brief Structure to handle the registered interrupt requests
	*
	* @param pin        Stores the interrupt pin
	* @param debounce   Debounce flag, callback will be handled differently
	* @param active     Flag is set while interrupt was detected and we need to debounce
	* @param ie         Interrupt vector, needed for calculation of the pin change interrupt number
	* @param time       Here we store the time stamp for debouncing functionallity
	* @param dlgt       Delegate, holds the callback information
	*/
	struct s_pcint_table {
		uint8_t pin;
		uint8_t debounce;
		uint8_t active;
		uint8_t ie;
		uint32_t time;
		s_pcint_dlgt dlgt;
	};
	extern s_pcint_table pcint_table[];



	// old struct to catch pin change interrupts
	struct  s_pcINT {
		uint8_t cur;
		uint8_t prev;
		uint32_t time;
	} static volatile pcInt[3];


	#define regPCIE(PORT)         (PCICR |= _BV(PORT))
	#define regPCINT(MASK,PORT)   (MASK  |= _BV(PORT))

	extern void    initPCINT(void);
	extern uint8_t chkPCINT(uint8_t port, uint8_t pin, uint8_t debounce);
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
