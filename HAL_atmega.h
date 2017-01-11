/*
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin vendor hardware abstraction layer  -----------------------------------------------------------------------------
*  This is the central place where all vendor hardware related functions are defined. 
*  HAL.h is the main template, all available functions are defined here, majority as external for sure
*  Based on the hardware, a vendor hal will be included HAL_<vendor>.h
*  Into the HAL_<vendor>.h there will be a CPU specific template included, HAL_<cpu>.h
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _HAL__ATMEGA_H
#define _HAL__ATMEGA_H

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>


/*-- struct for pin definition in HAL_<cpu>.h template 
* has to be defined before including the HAL_<cpu>.h template.
* needed for pin and interrupt functions 
*/
struct s_pin_def {
	uint8_t PINBIT;
	volatile uint8_t *DDREG;
	volatile uint8_t *PORTREG;
	volatile uint8_t *PINREG;
	uint8_t PCINR;
	uint8_t PCIBYTE;
	volatile uint8_t *PCICREG;
	volatile uint8_t *PCIMASK;
	uint8_t PCIEREG;
	uint8_t VEC;
};
//- -----------------------------------------------------------------------------------------------------------------------


#if defined(__AVR_ATmega328P__)
	#include "HAL_atmega_328P.h"
#elif defined(__AVR_ATmega_32U4__)
	#include "HAL_atmega_32U4.h"
#else
	#error "No HAL definition for current MCU available!"
#endif


/*************************************************************************************************************************/
/*************************************************************************************************************************/
/* - vendor and cpu specific functions --------------------------------------------------------------------------------- */
/*************************************************************************************************************************/
/*************************************************************************************************************************/

/*-- pin functions --------------------------------------------------------------------------------------------------------
* concept of pin functions is a central definition of pin and interrupt registers as a struct per pin. handover of pin 
* information is done by forwarding a pointer to the specific function and within the function all hardware related 
* setup and switching is done.
*/
void set_pin_output(const s_pin_def *ptr_pin);												// set a specific pin as output
void set_pin_input(const s_pin_def *ptr_pin);												// set the pin as input
void set_pin_high(const s_pin_def *ptr_pin);												// set high level on specific pin
void set_pin_low(const s_pin_def *ptr_pin);													// set a low level on a specific pin
void set_pin_toogle(const s_pin_def *ptr_pin);												// toogle the pin level
uint8_t get_pin_status(const s_pin_def *ptr_pin);											// detect a pin input if it is high or low
//- -----------------------------------------------------------------------------------------------------------------------


/*-- interrupt functions --------------------------------------------------------------------------------------------------
* based on the same concept as the pin functions. everything pin related is defined in a pin struct, handover of the pin 
* is done by forwarding a pointer to the struct. pin definition is done in HAL_<cpu>.h, functions are declared in HAL_<vendor>.h
*/
#define DEBOUNCE  5																			// debounce time for periodic check if an interrupt was raised
void register_PCINT(const s_pin_def *ptr_pin);												// function to register a pin interrupt
uint8_t check_PCINT(const s_pin_def *ptr_pin, uint8_t debounce);							// period check if a pin interrupt had happend
// pointer for callback defined in HAL_<vendor>.cpp											// defined in HAL_<vendor>.cpp, a call back function can be registered, debounce will not work while need a periodic check
inline void maintain_PCINT(uint8_t vec);													// internal function to handle pin change interrupts 
//- -----------------------------------------------------------------------------------------------------------------------


/*-- spi functions --------------------------------------------------------------------------------------------------------
* spi interface is again vendor related, we need two functions mainly for the communication class in asksin
* enable spi sets all relevant registers that the spi interface will work with the correct speed
* the sendbyte function for all communication related thing...
*/
void enable_spi(void);																		// configures the spi port
uint8_t spi_send_byte(uint8_t send_byte);													// send and receive via spi interface
//- -----------------------------------------------------------------------------------------------------------------------


/*-- eeprom functions -----------------------------------------------------------------------------------------------------
* to make the library more hardware independend all eeprom relevant functions are defined at one point
*/
void init_eeprom(void);																		// init the eeprom, can be enriched for a serial eeprom as well
void get_eeprom(uint16_t addr, uint8_t len, void *ptr);										// read a specific eeprom address
void set_eeprom(uint16_t addr, uint8_t len, void *ptr);										// write a block to a specific eeprom address
void clear_eeprom(uint16_t addr, uint16_t len);												// and clear the eeprom
//- -----------------------------------------------------------------------------------------------------------------------


/*-- timer functions ------------------------------------------------------------------------------------------------------
* as i don't want to depend on the arduino timer while it is not possible to add some time after the arduino was sleeping
* i defined a new timer. to keep it as flexible as possible you can configure the different timers in the arduino by handing
* over the number of the timer you want to utilize. as timer are very vendor related there is the need to have at least 
* timer 0 available for all different hardware.
*/
// https://github.com/zkemble/millis/blob/master/millis/
void init_millis_timer0(int16_t correct_ms = 0);											// initialize the respective timer
void init_millis_timer1(int16_t correct_ms = 0);
void init_millis_timer2(int16_t correct_ms = 0);
uint32_t get_millis(void);																	// get the current time in millis
void add_millis(uint32_t ms);																// add some time to the counter, mainly used while wake up from sleeping
//- -----------------------------------------------------------------------------------------------------------------------


/*-- battery measurement functions ----------------------------------------------------------------------------------------
* for arduino there are two options to measure the battery voltage - internal and external. internal can measure only a 
* directly connected battery, external can measure any voltage via a resistor network.
* the resistor network is connected with z1 to ground and the measure pin, z2 is connected to measure pin and VCC
* the external measurement function will enable both pins, measure the voltage and switch back the enable pin to input,
* otherwise the battery gets drained via the resistor network...
* http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
*/
uint8_t get_internal_voltage(void);															// internal measurement
uint8_t get_external_voltage(const s_pin_def *ptr_enable, const s_pin_def *ptr_measure, uint8_t z1, uint8_t z2); // external measurement
inline uint16_t get_adc_value(uint8_t reg_admux);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- power saving functions ----------------------------------------------------------------------------------------
* As power saving is very hardware and vendor related, we need a common scheme which is working similar on all
* supported hardware - needs to be reworked...
* http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
* http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode
*/
static uint16_t wdtSleep_TIME;																// variable to store the current mode, amount will be added after wakeup to the millis timer
void startWDG32ms(void);																	// set watchdog to 32 ms
void startWDG64ms(void);																	// 64 ms
void startWDG250ms(void);																	// 256 ms
void startWDG8000ms(void);																	// 8192 ms
void setSleep(void);																		// set the cpu in sleep mode

void startWDG();																			// start watchdog timer
void stopWDG();																				// stop the watchdog timer
void setSleepMode();																		// set the sleep mode, documentation to follow
//- -----------------------------------------------------------------------------------------------------------------------


#endif 
