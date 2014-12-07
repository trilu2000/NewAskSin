//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin hardware definition --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _HARDWARE_h
#define _HARDWARE_h

#include <HAL.h>


//- define hardware -------------------------------------------------------------------------------------------------------
//- LED's
#if defined(__AVR_ATmega32U4__)
	#define ledRedDDR     DDRB																// define led port and remaining pin
	#define ledRedPort    PORTB
	#define ledRedPin     PORTB7

	#define ledGrnDDR     DDRC
	#define ledGrnPort    PORTC
	#define ledGrnPin     PORTC7

	#define ledActiveLow  1																	// leds against GND = 0, VCC = 1

#else
	#define ledRedDDR     DDRD   //DDRB														// define led port and remaining pin
	#define ledRedPort    PORTD  //PORTB
	#define ledRedPin     PORTD6 //PORTB7

	#define ledGrnDDR     DDRD   //DDRC
	#define ledGrnPort    PORTD  //PORTC
	#define ledGrnPin     PORTD4 //PORTC7

	#define ledActiveLow  0																	// leds against GND = 0, VCC = 1
#endif

//- configuration key
#if defined(__AVR_ATmega32U4__)
	#define confKeyDDR    DDRB																// define config key port and remaining pin
	#define confKeyPort   PORTB
	#define confKeyPin    PORTB6

	#define confKeyPCICR  PCICR																// interrupt register
	#define confKeyPCIE   PCIE0																// pin change interrupt port bit
	#define confKeyPCMSK  PCMSK0															// interrupt mask
	#define confKeyINT    PCINT6															// pin interrupt

#else
	#define confKeyDDR    DDRB																// define config key port and remaining pin
	#define confKeyPort   PORTB
	#define confKeyPin    PORTB0

	#define confKeyPCICR  PCICR																// interrupt register
	#define confKeyPCIE   PCIE0																// pin change interrupt port bit
	#define confKeyPCMSK  PCMSK0															// interrupt mask
	#define confKeyINT    PCINT0															// pin interrupt

#endif
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
//- -----------------------------------------------------------------------------------------------------------------------


//- power management functions --------------------------------------------------------------------------------------------
// http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
// http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode

//- -----------------------------------------------------------------------------------------------------------------------


//- battery measurement functions -----------------------------------------------------------------------------------------
// http://jeelabs.org/2013/05/17/zero-powe-battery-measurement/

#define BATTERY_NUM_MESS_ADC              20												// real measures to get the best average measure
#define BATTERY_DUMMY_NUM_MESS_ADC        40												// dummy measures to get the ADC working
#define AVR_BANDGAP_VOLTAGE               1100UL											// band gap reference for Atmega328p
#define BATTERY_FACTOR                    124												// see excel table

//- external battery measurement
#if defined(__AVR_ATmega32U4__)
	#define battEnblDDR   DDRF																// define battery measurement enable pin, has to be low to start measuring
	#define battEnblPort  PORTF
	#define battEnblPin   PORTF4

	#define battMeasDDR   DDRF																// define battery measure pin, where ADC gets the measurement
	#define battMeasPort  PORTF
	#define battMeasPin   PORTF7

#else
	#define battEnblDDR   DDRB																// define battery measurement enable pin, has to be low to start measuring
	#define battEnblPort  PORTB
	#define battEnblPin   PORTB4

	#define battMeasDDR   DDRB																// define battery measure pin, where ADC gets the measurement
	#define battMeasPort  PORTB
	#define battMeasPin   PORTB7

#endif
//- -----------------------------------------------------------------------------------------------------------------------


//- some macros for debugging ---------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#endif

