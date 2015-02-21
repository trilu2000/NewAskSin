//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin hardware definition ----------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -------------------------------------------------------------------------------------------------------------------

#include <HAL.h>

#ifndef _HARDWARE_h
	#define _HARDWARE_h

	#define EXT_BATTERY_MEASUREMENT  1
	#define BATTERY_FACTOR           17											// see excel table
	#define debounce                 5
	#define LED_ACTIVE_LOW           0											// leds connected to GND = 0, VCC = 1


	#if defined(__AVR_ATmega328P__)
		//- cc1100 hardware CS and GDO0 definitions -------------------------------------------------------------------
		#define CC_CS_DDR            DDRB;										// SPI chip select definition
		#define CC_CS_PORT           PORTB;
		#define CC_CS_PIN            PORTB2;

		#define CC_GDO0_DDR          DDRD;										// GDO0 pin, signals received data
		#define CC_GDO0_PIN          PORTB2;

		#define CC_GDO0_PCICR        PCICR										// GDO0 interrupt register
		#define CC_GDO0_PCIE         PCIE2
		#define CC_GDO0_PCMSK        PCMSK2										// GDO0 interrupt mask
		#define CC_GDO0_INT          PCINT18									// pin interrupt

		//- LED's definition ------------------------------------------------------------------------------------------
		#define LED_RED_DDR          DDRD										// define led port and remaining pin
		#define LED_RED_PORT         PORTD
		#define LED_RED_PIN          PORTD4

		#define LED_GRN_DDR          DDRD
		#define LED_GRN_PORT         PORTD
		#define LED_GRN_PIN          PORTD4

		//- configuration key  ----------------------------------------------------------------------------------------
		#define confKeyDDR           DDRB										// define config key port and remaining pin
		#define confKeyPort          PORTB
		#define confKeyPin           PORTB0

		#define confKeyPCICR         PCICR										// interrupt register
		#define confKeyPCIE          PCIE0										// pin change interrupt port bit
		#define confKeyPCMSK         PCMSK0										// interrupt mask
		#define confKeyINT           PCINT0										// pin interrupt

		//- battery external measurement functions -----------------------------------------------------------------------------------------
		#define BATT_ENABLE_DDR      DDRD										// define battery measurement enable pin, has to be low to start measuring
		#define BATT_ENABLE_PORT     PORTD
		#define BATT_ENABLE_PIN      PORTD7

		#define BATT_MEASURE_DDR     DDRC										// define battery measure pin, where ADC gets the measurement
		#define BATT_MEASURE_PORT    PORTC
		#define BATT_MEASURE_PIN     PORTC1

	#elif defined(__AVR_ATmega32U4__)
		//- cc1100 hardware CS and GDO0 definitions -------------------------------------------------------------------
		#define CC_CS_DDR            DDRB;										// SPI chip select definition
		#define CC_CS_PORT           PORTB;
		#define CC_CS_PIN            PORTB4;

		#define CC_GDO0_DDR          DDRB;										// GDO0 pin, signals received data
		#define CC_GDO0_PIN          PORTB5;

		#define CC_GDO0_PCICR        PCICR										// GDO0 interrupt register
		#define CC_GDO0_PCIE         PCIE0
		#define CC_GDO0_PCMSK        PCMSK0										// GDO0 interrupt mask
		#define CC_GDO0_INT          PCINT5										// pin interrupt

		//- LED's definition ------------------------------------------------------------------------------------------
		#define ledRedDDR            DDRD										// define led port and remaining pin
		#define ledRedPort          PORTD
		#define ledRedPin           PORTD4

		#define ledGrnDDR           DDRD
		#define ledGrnPort          PORTD
		#define ledGrnPin           PORTD4


		//- configuration key  ----------------------------------------------------------------------------------------
		#define confKeyDDR          DDRB										// define config key port and remaining pin
		#define confKeyPort         PORTB
		#define confKeyPin          PORTB0

		#define confKeyPCICR        PCICR										// interrupt register
		#define confKeyPCIE         PCIE0										// pin change interrupt port bit
		#define confKeyPCMSK        PCMSK0										// interrupt mask
		#define confKeyINT          PCINT0										// pin interrupt

		//- battery external measurement functions --------------------------------------------------------------------
		#define BATT_ENABLE_DDR     DDRD										// define battery measurement enable pin, has to be low to start measuring
		#define BATT_ENABLE_PORT    PORTD
		#define BATT_ENABLE_PIN     PORTD7

		#define BATT_MEASURE_DDR    DDRC										// define battery measure pin, where ADC gets the measurement
		#define BATT_MEASURE_PORT   PORTC
		#define BATT_MEASURE_PIN    PORTC1

	#else
		#error "Error: cc1100 CS and GDO0 not defined for your hardware in hardware.h!"
	#endif
	//- ---------------------------------------------------------------------------------------------------------------


	//- wake up pin ---------------------------------------------------------------------------------------------------
	#if defined(__AVR_ATmega32U4__)
		#define wakeupDDR           DDRE										// define wake up pin port and remaining pin
		#define wakeupPRT           PORTE
		#define wakeupPNR           PINE
		#define wakeupPIN           PINE2
	#endif
	//- ---------------------------------------------------------------------------------------------------------------


	//- function prototypes -------------------------------------------------------------------------------------------
	void    initExtBattMeasurement(void);
	void    switchExtBattMeasurement(uint8_t stat);

#endif

