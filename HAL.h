/*
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin hardware abstraction layer  ------------------------------------------------------------------------------------
*  This is the central place where all hardware related functions are defined. Some are based on Arduino like debug print,
*  but the majority is strongly based on the hardware vendor and cpu type. Therefor the HAL.h has a modular concept:
*  HAL.h is the main template, all available functions are defined here, majority as external for sure
*  Based on the hardware, a vendor hal will be included HAL_<vendor>.h
*  Into the HAL_<vendor>.h there will be a CPU specific template included, HAL_<cpu>.h
* - -----------------------------------------------------------------------------------------------------------------------
*/


#ifndef _HAL_H
#define _HAL_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <stdint.h>




//-- MCU dependent HAL definitions ----------------------------------------------------------------------------------------
#if defined(__AVR__)
	//#include "HAL_atmega.h"
	
	#if defined(__AVR_ATmega328P__)
		#include "HAL_atmega_328P.h"
	#elif defined(__AVR_ATmega32U4__)
		#include "HAL_atmega_32U4.h"
	#else
		#error "No HAL definition for current MCU available!"
	#endif
#else
	#error "No HAL definition for current MCU available!"
#endif




/*************************************************************************************************************************/
/*************************************************************************************************************************/
/* - vendor and cpu specific functions --------------------------------------------------------------------------------- */
/*************************************************************************************************************************/
/*************************************************************************************************************************/

/*-- pin functions --------------------------------------------------------------------------------------------------------
* all pins defined as a struct, holding all information regarding port, pin, ddr, etc.
* as we support different arduino hw i tried to make it as flexible as possible. everything is defined in seperate 
* hw specific files. the struct and pin manipulation function is defined in HAL_atmega.h because it is similar for all
* ATMEL hardware, the pin structs are defined in HAL_atmega_<model> while different for each cpu type. here we reference
* only on the functions defined in HAL_<type>_<model>.
*/
void set_pin_output(uint8_t pin_def);
void set_pin_input(uint8_t pin_def);
void set_pin_high(uint8_t pin_def);
void set_pin_low(uint8_t pin_def);
uint8_t get_pin_status(uint8_t pin_def);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- interrupt functions --------------------------------------------------------------------------------------------------
* interrupts again are very hardware supplier related, therefor we define her some external functions which needs to be 
* defined in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
* you can also use the arduino standard timer for a specific hardware by interlinking the function call to getmillis()
*/
#define DEBOUNCE  5																			// debounce time for periodic check if an interrupt was raised
static void(*pci_ptr)(uint8_t vec, uint8_t pin, uint8_t flag);
void register_PCINT(uint8_t pin_def);
uint8_t check_PCINT(uint8_t pin_def, uint8_t debounce);
void maintain_PCINT(uint8_t vec);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- spi functions --------------------------------------------------------------------------------------------------------
* spi is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
*/
void enable_spi(void);
uint8_t spi_send_byte(uint8_t send_byte);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- eeprom functions -----------------------------------------------------------------------------------------------------
* eeprom is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
*/
void init_eeprom(void);
void get_eeprom(uint16_t addr, uint8_t len, void *ptr);
void set_eeprom(uint16_t addr, uint8_t len, void *ptr);
void clear_eeprom(uint16_t addr, uint16_t len);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- timer functions ------------------------------------------------------------------------------------------------------
* timer is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
* you can also use the arduino standard timer for a specific hardware by interlinking the function call to getmillis()
*/
#ifdef hasTimer0
void init_millis_timer0(int16_t correct_ms = 0);
#endif
#ifdef hasTimer1
void init_millis_timer1(int16_t correct_ms = 0);
#endif
#ifdef hasTimer2
void init_millis_timer2(int16_t correct_ms = 0);
#endif
#ifdef hasTimer3
void init_millis_timer3(int16_t correct_ms = 0);
#endif
uint32_t get_millis(void);
void add_millis(uint32_t ms);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- battery measurement functions ----------------------------------------------------------------------------------------
* battery measurements is done in two ways, internal measurement based on atmel specs, or external measurement via a voltage 
* divider with one pin to enable and another to measure the adc value. both is hardware and vendor related, you will find the
* code definition in HAL_<vendor>.h
*/
uint8_t get_internal_voltage(void);
//void init_external_voltage(uint8_t pin_enable, uint8_t pin_measure);
uint8_t get_external_voltage(const uint8_t pin_enable, uint8_t pin_measure, uint8_t z1, uint8_t z2);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- power saving functions ----------------------------------------------------------------------------------------
* As power saving is very hardware and vendor related, you will find the code definition in HAL_<vendor>.h
*/
void startWDG32ms(void);
void startWDG64ms(void);
void startWDG256ms(void);
void startWDG8192ms(void);
void setSleep(void);

void startWDG(void);
void stopWDG(void);
void setSleepMode(void);
//- -----------------------------------------------------------------------------------------------------------------------

uint16_t freeRam();




/*************************************************************************************************************************/
/*************************************************************************************************************************/
/* - arduino compatible functions -------------------------------------------------------------------------------------- */
/*************************************************************************************************************************/
/*************************************************************************************************************************/

/*-- serial print functions -----------------------------------------------------------------------------------------------
* template and some functions for debugging over serial interface
* based on arduino serial class, so should work with all hardware served in arduino
* http://aeroquad.googlecode.com/svn/branches/pyjamasam/WIFIReceiver/Streaming.h
*/
#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

const char num2char[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A', 'B', 'C', 'D', 'E', 'F', };
struct _HEX {
	uint8_t *val;
	uint8_t len;
	_HEX(uint8_t v) : val(&v), len(1) {}
	_HEX(uint8_t *v, uint8_t l = 1) : val(v), len(l) {}
};
inline Print &operator <<(Print &obj, const _HEX &arg) { 
	for (uint8_t i = 0; i<arg.len; i++) {
		if (i) obj.print(' ');
		obj.print(num2char[arg.val[i] >> 4]);
		obj.print(num2char[arg.val[i] & 0xF]);
	}
	return obj; 
}

enum _eTIME { _TIME };
inline Print &operator <<(Print &obj, _eTIME arg) { obj.print('('); obj.print(get_millis()); obj.print(')'); return obj; }
//- -----------------------------------------------------------------------------------------------------------------------


/*-- randum number functions ----------------------------------------------------------------------------------------------
* Random number is needed for AES encryption, here we are generating a fake random number by xoring the timer.
*/
void get_random(uint8_t *buf, uint32_t x = get_millis());
//- -----------------------------------------------------------------------------------------------------------------------


/*-- progmem macros -------------------------------------------------------------------------------------------------------
* not sure if it is realy vendor independend, to be checked later...
*/
#define _PGM_BYTE(x) pgm_read_byte(&x)	
#define _PGM_WORD(x) pgm_read_word(&x)
//- -----------------------------------------------------------------------------------------------------------------------


/*-- conversation for message enum ----------------------------------------------------------------------------------------
* macro to overcome the endian problem while converting a long number into a byte array
*/
#define BIG_ENDIAN ((1 >> 1 == 0) ? 0 : 1)
#if BIG_ENDIAN
#define BY03(x)   ( (uint8_t) (( (uint32_t)(x) >> 0) ) )
#define BY10(x)   ( (uint8_t) (( (uint32_t)(x) >> 8) ) )
#define BY11(x)   ( (uint8_t) (( (uint32_t)(x) >> 16) ) )
#define MLEN(x)   ( (uint8_t) (( (uint32_t)(x) >> 24) ) )
#else
#define BY03(x)   ( (uint8_t) (( (uint32_t)(x) >> 24) ) )
#define BY10(x)   ( (uint8_t) (( (uint32_t)(x) >> 16) ) )
#define BY11(x)   ( (uint8_t) (( (uint32_t)(x) >> 8) ) )
#define MLEN(x)   ( (uint8_t) (( (uint32_t)(x) >> 0) ) )
#endif
//- -----------------------------------------------------------------------------------------------------------------------


#endif 
