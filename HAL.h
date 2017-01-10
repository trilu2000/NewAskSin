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


//#include "macros.h"


//- MCU dependent HAL definitions -----------------------------------------------------------------------------------------
#if defined(__AVR__)
	#include "HAL_atmega.h"
#else
	#error "No HAL definition for current MCU available!"
#endif




/*-- pin functions --------------------------------------------------------------------------------------------------------
* all pins defined as a struct, holding all information regarding port, pin, ddr, etc.
* as we support different arduino hw i tried to make it as flexible as possible. everything is defined in seperate 
* hw specific files. the struct and pin manipulation function is defined in HAL_atmega.h because it is similar for all
* ATMEL hardware, the pin structs are defined in HAL_atmega_<model> while different for each cpu type. here we reference
* only on the functions defined in HAL_<type>_<model>.
*/
extern void set_pin_output(const s_pin_def *ptr_pin);
extern void set_pin_input(const s_pin_def *ptr_pin);
extern void set_pin_high(const s_pin_def *ptr_pin);
extern void set_pin_low(const s_pin_def *ptr_pin);
extern void set_pin_toogle(const s_pin_def *ptr_pin);
extern uint8_t get_pin_status(const s_pin_def *ptr_pin);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- interrupt functions --------------------------------------------------------------------------------------------------
* interrupts again are very hardware supplier related, therefor we define her some external functions which needs to be 
* defined in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
* you can also use the arduino standard timer for a specific hardware by interlinking the function call to getmillis()
*/
extern void register_PCINT(const s_pin_def *ptr_pin);
extern uint8_t check_PCINT(const s_pin_def *ptr_pin);
extern void(*pci_ptr)(uint8_t vec, uint8_t pin, uint8_t flag);
extern void maintain_PCINT(uint8_t vec);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- spi functions --------------------------------------------------------------------------------------------------------
* spi is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
*/
extern void enable_spi(void);
extern uint8_t spi_send_byte(uint8_t send_byte);


/*-- eeprom functions -----------------------------------------------------------------------------------------------------
* eeprom is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
*/
extern void init_eeprom(void);
extern void get_eeprom(uint16_t addr, uint8_t len, void *ptr);
extern void set_eeprom(uint16_t addr, uint8_t len, void *ptr);
extern void clear_eeprom(uint16_t addr, uint16_t len);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- timer functions ------------------------------------------------------------------------------------------------------
* timer is very hardware supplier related, therefor we define her some external functions which needs to be defined
* in the hardware specific HAL file. for ATMEL it is defined in HAL_atmega.h.
* you can also use the arduino standard timer for a specific hardware by interlinking the function call to getmillis()
*/
extern void init_millis(void);
extern uint32_t get_millis(void);
extern void add_millis(uint32_t ms);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- battery measurement functions ----------------------------------------------------------------------------------------
* battery measurements is done in two ways, internal measurement based on atmel specs, or external measurement via a voltage 
* divider with one pin to enable and another to measure the adc value. both is hardware and vendor related, you will find the
* code definition in HAL_<vendor>.h
*/
extern uint8_t get_internal_voltage(void);
extern void init_external_voltage(const s_pin_def *ptr_enable, const s_pin_def *ptr_measure);
extern uint8_t get_external_voltage(const s_pin_def *ptr_enable, const s_pin_def *ptr_measure, uint8_t z1, uint8_t z2);
//- -----------------------------------------------------------------------------------------------------------------------



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



//- randum number functions -----------------------------------------------------------------------------------------------
void get_random(uint8_t *buf);
//- -----------------------------------------------------------------------------------------------------------------------


//- some macros and definitions -------------------------------------------------------------------------------------------
#define _PGM_BYTE(x) pgm_read_byte(&x)										// short hand for PROGMEM read
#define _PGM_WORD(x) pgm_read_word(&x)
//- -----------------------------------------------------------------------------------------------------------------------


//- conversation for message enum -----------------------------------------------------------------------------------------
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




static uint16_t wdtSleep_TIME;





	//- needed for 32u4 to prevent sleep, while USB didn't work in sleep ------------------------------------------------------
//	extern void    initWakeupPin(void);															// init the wakeup pin
//	extern uint8_t checkWakeupPin(void);														// we can setup a pin which avoid sleep mode
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
//	#define BAT_NUM_MESS_ADC                  20								// real measures to get the best average measure
//	#define BAT_DUMMY_NUM_MESS_ADC            40								// dummy measures to get the ADC working

//	extern uint16_t getAdcValue(uint8_t adcmux);
//	uint8_t  getBatteryVoltage(void);
	//- -----------------------------------------------------------------------------------------------------------------------



#endif 
