//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin hardware definition --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _HARDWARE_h
	#define _HARDWARE_h

	#include <HAL/HAL.h>

	//- define hardware -------------------------------------------------------------------------------------------------------
	//- LED's
	#if defined(__AVR_ATmega328P__)
		#define ledRedDDR     DDRD												// define led port and remaining pin
		#define ledRedPort    PORTD
		#define ledRedPin     PORTD4

		#define ledGrnDDR     DDRD
		#define ledGrnPort    PORTD
		#define ledGrnPin     PORTD4

		#define ledActiveLow  0													// leds against GND = 0, VCC = 1
	#else
		#error "Error: LEDS not defined for your hardware in hardware.h!"

	#endif

	//- configuration key
	#if defined(__AVR_ATmega328P__)
		#define confKeyDDR    DDRB												// define config key port and remaining pin
		#define confKeyPort   PORTB
		#define confKeyPin    PORTB0

		#define confKeyPCICR  PCICR												// interrupt register
		#define confKeyPCIE   PCIE0												// pin change interrupt port bit
		#define confKeyPCMSK  PCMSK0											// interrupt mask
		#define confKeyINT    PCINT0											// pin interrupt
	#else
		#error "Error: Configuration key not defined for your hardware in hardware.h!"

	#endif

	//- wake up pin
	#if defined(__AVR_ATmega32U4__)
		#define wakeupDDR     DDRE												// define wake up pin port and remaining pin
		#define wakeupPRT     PORTE
		#define wakeupPNR     PINE
		#define wakeupPIN     PINE2
	#endif
	//- -----------------------------------------------------------------------------------------------------------------------


	//- battery measurement functions -----------------------------------------------------------------------------------------
	#define BATTERY_FACTOR    17												// see excel table

	//- external battery measurement
	#if defined(__AVR_ATmega328P__)
		#define battEnblDDR   DDRD												// define battery measurement enable pin, has to be low to start measuring
		#define battEnblPort  PORTD
		#define battEnblPin   PORTD7

		#define battMeasDDR   DDRC												// define battery measure pin, where ADC gets the measurement
		#define battMeasPort  PORTC
		#define battMeasPin   PORTC1
	#else
		#error "Error: External battery measurement not defined for your hardware in hardware.h!"
	#endif

	void    initExtBattMeasurement(void);
	void    switchExtBattMeasurement(uint8_t stat);
	//- -----------------------------------------------------------------------------------------------------------------------
	//- some macros for debugging ---------------------------------------------------------------------------------------------
	//- -----------------------------------------------------------------------------------------------------------------------

#endif

