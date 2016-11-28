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

//#define EXT_BATTERY_MEASUREMENT												// comment out to use internal battery measurement
	#define BATTERY_FACTOR             17										// see excel table
	#define DEBOUNCE                   5

	#if defined(__AVR_ATmega328P__)
		//- hardware specific general setup
		#define PCINT_CALLBACK													// enables the pin change interrupt callback in user sketch

		//- cc1100 hardware CS and GDO0 definitions -------------------------------------------------------------------
		#define CC_CS                  PIN_B2									// chip select
		#define CC_GDO0                PIN_D2									// GDO0 pin, identify data received on falling edge
		#define CC_MISO                PIN_B4									// SPI port MISO
		#define CC_MOSI                PIN_B3									// SPI port MOSI
		#define CC_SCLK                PIN_B5									// SPI port Clock


		//- LED's definition ------------------------------------------------------------------------------------------
		#define LED_RED                PIN_D6									// define the red led pin port
		#define LED_GRN                PIN_D4									// define the green led		
		#define LED_ACTIVE_LOW         0										// leds connected to GND = 0, VCC = 1


		//- configuration key  ----------------------------------------------------------------------------------------
		#define CONFIG_KEY             PIN_B0									// define the config key pin port


		//- battery external measurement functions --------------------------------------------------------------------
		#define BATT_ENABLE            PIN_D7									// define battery measurement enable pin, has to be low to start measuring
		#define BATT_MEASURE           PIN_C1									// define battery measure pin, where ADC gets the measurement


	#elif defined(__AVR_ATmega32U4__)
		//- cc1100 hardware CS and GDO0 definitions -------------------------------------------------------------------
		#define CC_CS                  PIN_B4									// chip select
		#define CC_GDO0                PIN_B5									// GDO0 pin, identify data received on falling edge
		#define CC_MISO                PIN_B3									// SPI port MISO
		#define CC_MOSI                PIN_B2									// SPI port MOSI
		#define CC_SCLK                PIN_B1									// SPI port Clock


		//- LED's definition ------------------------------------------------------------------------------------------
		#define LED_RED                PIN_B7									// define the red led pin port
		#define LED_GRN                PIN_C7									// define the green led		
		#define LED_ACTIVE_LOW         1										// leds connected to GND = 0, VCC = 1


		//- configuration key  ----------------------------------------------------------------------------------------
		#define CONFIG_KEY             PIN_B6									// define the config key pin port


		//- battery external measurement functions --------------------------------------------------------------------
		#define BATT_ENABLE            PIN_F4									// define battery measurement enable pin, has to be low to start measuring
		#define BATT_MEASURE           PIN_F7									// define battery measure pin, where ADC gets the measurement


	#else
		#error "Error: cc1100 CS and GDO0 not defined for your hardware in hardware.h!"
	#endif
	//- ---------------------------------------------------------------------------------------------------------------


	//- wake up pin ---------------------------------------------------------------------------------------------------
	#if defined(__AVR_ATmega32U4__)
		#define WAKE_UP_DDR            DDRE										// define wake up pin port and remaining pin
		#define WAKE_UP_PORT           PORTE
		#define WAKE_UP_PNR            PINE
		#define WAKE_UP_PIN            PINE2
	#endif
	//- ---------------------------------------------------------------------------------------------------------------

#endif

