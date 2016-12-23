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

#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>


#include "macros.h"


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
extern uint8_t check_PCINT(uint8_t port, uint8_t pin, uint8_t debounce);
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





//- randum number functions -----------------------------------------------------------------------------------------------
void get_random(uint8_t *buf);
//- -----------------------------------------------------------------------------------------------------------------------



	static uint16_t wdtSleep_TIME;





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
