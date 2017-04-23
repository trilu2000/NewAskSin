#if defined(__AVR__)


#include "HAL.h"

#include <util/atomic.h>
#include <util/delay.h>


/*-- pin functions --------------------------------------------------------------------------------------------------------
* concept of pin functions is a central definition of pin and interrupt registers as a struct per pin. handover of pin
* information is done by forwarding a pointer to the specific function and within the function all hardware related
* setup and switching is done.
*/

/* set a specific pin as output */
void set_pin_output(uint8_t pin_def) {
	uint8_t bit = digitalPinToBitMask(pin_def);
	uint8_t port = digitalPinToPort(pin_def);
	volatile uint8_t *reg;

	reg = portModeRegister(port);
	*reg |= bit;
}
/* set the pin as input */
void set_pin_input(uint8_t pin_def) {
	uint8_t bit = digitalPinToBitMask(pin_def);
	uint8_t port = digitalPinToPort(pin_def);
	volatile uint8_t *reg;

	reg = portModeRegister(port);
	*reg &= ~bit;
}

/* set high level on specific pin */
void set_pin_high(uint8_t pin_def) {
	uint8_t bit = digitalPinToBitMask(pin_def);
	uint8_t port = digitalPinToPort(pin_def);

	volatile uint8_t *out;
	out = portOutputRegister(port);
	*out |= bit;
}
/* set a low level on a specific pin */
void set_pin_low(uint8_t pin_def) {
	uint8_t bit = digitalPinToBitMask(pin_def);
	uint8_t port = digitalPinToPort(pin_def);

	volatile uint8_t *out;
	out = portOutputRegister(port);
	*out &= ~bit;
}
/* detect a pin input if it is high or low */
uint8_t get_pin_status(uint8_t pin_def) {
	uint8_t bit = digitalPinToBitMask(pin_def);
	uint8_t port = digitalPinToPort(pin_def);

	if (*portInputRegister(port) & bit) return HIGH;
	return LOW;
}
//- -----------------------------------------------------------------------------------------------------------------------



/*-- interrupt functions --------------------------------------------------------------------------------------------------
* based on the same concept as the pin functions. everything pin related is defined in a pin struct, handover of the pin
* is done by forwarding a pointer to the struct. pin definition is done in HAL_<cpu>.h, functions are declared in HAL_<vendor>.h
*/
struct  s_pcint_vector {
	volatile uint8_t *PINREG;
	uint8_t curr;
	uint8_t prev;
	uint8_t mask;
	uint32_t time;
};
volatile s_pcint_vector pcint_vector[pc_interrupt_vectors];									// define a struct for pc int processing

/* function to register a pin interrupt */
void register_PCINT(uint8_t def_pin) {
	set_pin_input(def_pin);																	// set the pin as input
	set_pin_high(def_pin);																	// key is connected against ground, set it high to detect changes

	// need to get vectore 0 - 2, depends on cpu
	uint8_t vec = digitalPinToPCICRbit(def_pin);											// needed for interrupt handling and to sort out the port
	uint8_t port = digitalPinToPort(def_pin);												// need the pin port to get further information as port register
	if (port == NOT_A_PIN) return;															// return while port was not found

	pcint_vector[vec].PINREG = portInputRegister(port);										// remember the input register
	pcint_vector[vec].curr |= get_pin_status(def_pin);										// remember current status of the port bit
	pcint_vector[vec].prev = pcint_vector[vec].curr;										// and set it as previous while we check for changes

	pcint_vector[vec].mask |= digitalPinToBitMask(def_pin);									// set the pin bit in the bitmask

	*digitalPinToPCICR(def_pin) |= _BV(digitalPinToPCICRbit(def_pin));						// pci functions
	*digitalPinToPCMSK(def_pin) |= _BV(digitalPinToPCMSKbit(def_pin));						// make the pci active
}

/* period check if a pin interrupt had happend */
uint8_t check_PCINT(uint8_t def_pin, uint8_t debounce) {
	// need to get vectore 0 - 3, depends on cpu
	uint8_t vec = digitalPinToPCICRbit(def_pin);											// needed for interrupt handling and to sort out the port
	uint8_t bit = digitalPinToBitMask(def_pin);												// get the specific bit for the asked pin

	uint8_t status = pcint_vector[vec].curr & bit ? 1 : 0;									// evaluate the pin status
	uint8_t prev = pcint_vector[vec].prev & bit ? 1 : 0;									// evaluate the previous pin status

	if (status == prev) return status;														// check if something had changed since last time
	if (debounce && ((get_millis() - pcint_vector[vec].time) < DEBOUNCE)) return status;	// seems there is a change, check if debounce is necassary and done

	pcint_vector[vec].prev ^= bit;															// if we are here, there was a change and debounce check was passed, remember for next time

	if (status) return 3;																	// pin is 1, old was 0
	else return 2;																			// pin is 0, old was 1
}

/* internal function to handle pin change interrupts */
void maintain_PCINT(uint8_t vec) {
	pcint_vector[vec].curr = *pcint_vector[vec].PINREG & pcint_vector[vec].mask;			// read the pin port and mask out only pins registered
	pcint_vector[vec].time = get_millis();													// store the time, if debounce is asked for

	if (pci_ptr) {
		uint8_t pin_int = pcint_vector[vec].curr ^ pcint_vector[vec].prev;					// evaluate the pin which raised the interrupt
		pci_ptr(vec, pin_int, pcint_vector[vec].curr & pin_int);							// callback the interrupt function in user sketch
	}
}

/* interrupt vectors to catch pin change interrupts */
#ifdef PCIE0
ISR(PCINT0_vect) {
	maintain_PCINT(0);
}
#endif

#ifdef PCIE1
ISR(PCINT1_vect) {
	maintain_PCINT(1);
}
#endif

#ifdef PCIE2
ISR(PCINT2_vect) {
	maintain_PCINT(2);
}
#endif

#ifdef PCIE3
ISR(PCINT3_vect) {
	maintain_PCINT(3);
}
#endif
//- -----------------------------------------------------------------------------------------------------------------------



/*-- spi functions --------------------------------------------------------------------------------------------------------
* spi interface is again vendor related, we need two functions mainly for the communication class in asksin
* enable spi sets all relevant registers that the spi interface will work with the correct speed
* the sendbyte function for all communication related thing...
*/

/* configures the spi port */
void enable_spi(void) {
	power_spi_enable();																		// enable only needed functions
	SPCR = _BV(SPE) | _BV(MSTR);															// SPI enable, master, speed = CLK/4
}
/* send and receive via spi interface */
uint8_t spi_send_byte(uint8_t send_byte) {
	SPDR = send_byte;																		// send byte
	while (!(SPSR & _BV(SPIF))); 															// wait until transfer finished
	return SPDR;																			// return the data register
}
//- -----------------------------------------------------------------------------------------------------------------------


/*-- eeprom functions -----------------------------------------------------------------------------------------------------
* to make the library more hardware independend all eeprom relevant functions are defined at one point
*/

/* init the eeprom, can be enriched for a serial eeprom as well */
void init_eeprom(void) {
	// place the code to init a i2c eeprom
}

/* read a specific eeprom address */
void get_eeprom(uint16_t addr, uint8_t len, void *ptr) {
	eeprom_read_block((void*)ptr, (const void*)addr, len);									// AVR GCC standard function
}

/* write a block to a specific eeprom address */
void set_eeprom(uint16_t addr, uint8_t len, void *ptr) {
	/* update is much faster, while writes only when needed; needs some byte more space
	* but otherwise we run in timing issues */
	eeprom_update_block((const void*)ptr, (void*)addr, len);								// AVR GCC standard function
}

/* and clear the eeprom */
void clear_eeprom(uint16_t addr, uint16_t len) {
	uint8_t tB = 0;
	if (!len) return;
	for (uint16_t l = 0; l < len; l++) {													// step through the bytes of eeprom
		set_eeprom(addr + l, 1, (void*)&tB);
	}
}
//- -----------------------------------------------------------------------------------------------------------------------


/*-- timer functions ------------------------------------------------------------------------------------------------------
* as i don't want to depend on the arduino timer while it is not possible to add some time after the arduino was sleeping
* i defined a new timer. to keep it as flexible as possible you can configure the different timers in the arduino by handing
* over the number of the timer you want to utilize. as timer are very vendor related there is the need to have at least
* timer 0 available for all different hardware.
*/
// https://github.com/zkemble/millis/blob/master/millis/
static volatile uint32_t milliseconds;
static volatile uint8_t timer = 255;

void add_millis(uint32_t ms) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		milliseconds += ms;
	}
}

uint32_t get_millis(void) {
	uint32_t ms;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ms = milliseconds;
	}
	return ms;
}

#ifdef TIMER0_COMPA_vect
ISR(TIMER0_COMPA_vect) {
	if (timer == 0) ++milliseconds;
}
#endif

#ifdef TIMER1_COMPA_vect
ISR(TIMER1_COMPA_vect) {
	if (timer == 1) ++milliseconds;
}
#endif

#ifdef TIMER2_COMPA_vect
ISR(TIMER2_COMPA_vect) {
	if (timer == 2) ++milliseconds;
}
#endif

#ifdef TIMER3_COMPA_vect
ISR(TIMER3_COMPA_vect) {
	if (timer == 3) ++milliseconds;
}
#endif

//- -----------------------------------------------------------------------------------------------------------------------


/*-- battery measurement functions ----------------------------------------------------------------------------------------
* for arduino there are two options to measure the battery voltage - internal and external. internal can measure only a
* directly connected battery, external can measure any voltage via a resistor network.
* the resistor network is connected with z1 to ground and the measure pin, z2 is connected to measure pin and VCC
* the external measurement function will enable both pins, measure the voltage and switch back the enable pin to input,
* otherwise the battery gets drained via the resistor network...
* http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
*/
#define BAT_NUM_MESS_ADC                  20												// real measures to get the best average measure
uint16_t get_adc_value(uint8_t reg_admux);													// helper function

/* internal measurement */
uint8_t get_internal_voltage(void) {
	uint16_t result = get_adc_value(admux_internal);										// get the adc value on base of the predefined adc register setup
	result = 11253L / result;																// calculate Vcc (in mV); 11253 = 1.1*1023*10 (*10 while we want to get 10mv)
	return (uint8_t)result;																	// Vcc in millivolts
}

/* external measurement */
uint8_t get_external_voltage(uint8_t pin_enable, uint8_t pin_measure, uint8_t z1, uint8_t z2) {
	/* set the pins to enable measurement */
	set_pin_output(pin_enable);																// set the enable pin as output
	set_pin_low(pin_enable);																// and to gnd, while measurement goes from VCC over the resistor network to GND
	set_pin_input(pin_measure);																// set the ADC pin as input

	/* call the adc get function to get the adc value, do some mathematics on the result */
	uint32_t result = get_adc_value(admux_external | digitalPinToBitMask(pin_measure));		// get the adc value on base of the predefined adc register setup
	result = ((result * ref_v_external) / 102) / z1;										// calculate vcc between gnd and measurement pin 
	result = result * (z1 + z2) / 100;														// interpolate result to vcc 

	/* finally, we set both pins as input again to waste no energy over the resistor network to VCC */
	set_pin_input(pin_enable);	
	set_pin_input(pin_measure);	

	return result;																			// Vcc in millivolts
}

uint16_t get_adc_value(uint8_t reg_admux) {
	uint16_t adcValue = 0;
	/* enable and set adc */
	/* enable and set adc */
	power_adc_enable();																		// start adc 

	ADMUX = reg_admux;																		// set adc
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);										// enable ADC and set ADC pre scaler
	_delay_ms(2);

	/* measure the adc */
	for (uint8_t i = 0; i < BAT_NUM_MESS_ADC; i++) {										// take samples in a round
		ADCSRA |= (1 << ADSC);																// start conversion
		while (ADCSRA & (1 << ADSC))														// wait for conversion complete
			;
		adcValue += ADCW;
	}

	ADCSRA &= ~(1 << ADEN);																	// ADC disable
	adcValue = adcValue / BAT_NUM_MESS_ADC;													// divide adcValue by amount of measurements

	power_adc_disable();																	// stop adc
	return adcValue;
}
//- -----------------------------------------------------------------------------------------------------------------------


/*-- power saving functions ----------------------------------------------------------------------------------------
* As power saving is very hardware and vendor related, we need a common scheme which is working similar on all
* supported hardware - needs to be reworked...
* http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
* http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode
*/
static uint16_t wdtSleep_TIME;																// variable to store the current mode, amount will be added after wakeup to the millis timer

void startWDG32ms(void) {
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDIE) | (1 << WDP0);
	wdtSleep_TIME = 32;
}
void startWDG64ms(void) {
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDIE) | (1 << WDP1);
	wdtSleep_TIME = 64;
}
void startWDG256ms(void) {
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDIE) | (1 << WDP2);
	wdtSleep_TIME = 256;
}
void startWDG8192ms(void) {
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
	wdtSleep_TIME = 8192;
}

void setSleep(void) {
	// some power savings by switching off some CPU functionality
	ADCSRA = 0;																				// disable ADC
	backupPwrRegs();																		// save content of power reduction register and set it to all off

	sleep_enable();																			// enable sleep
	offBrownOut();																			// turn off brown out detection

	sleep_cpu();																			// goto sleep
	// sleeping now
	// --------------------------------------------------------------------------------------------------------------------
	// wakeup will be here
	sleep_disable();																		// first thing after waking from sleep, disable sleep...
	recoverPwrRegs();																		// recover the power reduction register settings
	//dbg << '.';																			// some debug
}

void    startWDG() {
	WDTCSR = (1 << WDIE);
}
void    stopWDG() {
	WDTCSR &= ~(1 << WDIE);
}
void    setSleepMode() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

ISR(WDT_vect) {
	add_millis(wdtSleep_TIME);																// nothing to do, only for waking up
}
//- -----------------------------------------------------------------------------------------------------------------------


uint16_t freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

#endif