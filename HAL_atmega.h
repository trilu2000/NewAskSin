#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>


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


#if defined(__AVR_ATmega328P__)
	#include "HAL_atmega_328P.h"
#elif defined(__AVR_ATmega_32U4__)
	#include "HAL_atmega_32U4.h"
#else
	#error "No HAL definition for current MCU available!"
#endif


//-- pin functions --------------------------------------------------------------------------------------------------------
void set_pin_output(const s_pin_def *ptr_pin);
void set_pin_input(const s_pin_def *ptr_pin);
void set_pin_high(const s_pin_def *ptr_pin);
void set_pin_low(const s_pin_def *ptr_pin);
void set_pin_toogle(const s_pin_def *ptr_pin);
uint8_t get_pin_status(const s_pin_def *ptr_pin);
//- -----------------------------------------------------------------------------------------------------------------------


//-- interrupt functions --------------------------------------------------------------------------------------------------
#define DEBOUNCE  5
void register_PCINT(const s_pin_def *ptr_pin);
uint8_t check_PCINT(const s_pin_def *ptr_pin, uint8_t debounce);
void maintain_PCINT(uint8_t vec);
//- -----------------------------------------------------------------------------------------------------------------------


//-- spi functions --------------------------------------------------------------------------------------------------------
void enable_spi(void);
uint8_t spi_send_byte(uint8_t send_byte);
//- -----------------------------------------------------------------------------------------------------------------------


//-- eeprom functions -----------------------------------------------------------------------------------------------------
void init_eeprom(void);
void get_eeprom(uint16_t addr, uint8_t len, void *ptr);
void set_eeprom(uint16_t addr, uint8_t len, void *ptr);
void clear_eeprom(uint16_t addr, uint16_t len);
//- -----------------------------------------------------------------------------------------------------------------------


//-- timer functions ------------------------------------------------------------------------------------------------------
// https://github.com/zkemble/millis/blob/master/millis/
#define TIMER2
void init_millis(void);
uint32_t get_millis(void);
void add_millis(uint32_t ms);
//- -----------------------------------------------------------------------------------------------------------------------
