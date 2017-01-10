//- debug functionallity --------------------------------------------------------------------------------------------------
#include "00_debug-flag.h"


//- load library's --------------------------------------------------------------------------------------------------------
#include <as_main.h>																		// ask sin framework
#include "register.h"																		// configuration sheet
#include "wait_timer.h"

waitTimer xtimer;


//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	EIMSK = 0;																				// disable external interrupts
	ADCSRA = 0;																				// ADC off
	power_all_disable();																	// and everything else

	//DDRB = DDRC = DDRD = 0x00;															// everything as input
	//PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	DBG_START(SER, F("HM_PB_6_WM55\n"));
	DBG(SER, F(LIB_VERSION_STRING));

	// - AskSin related ---------------------------------------
	hm.init();																				// init the asksin framework

	// - user related -----------------------------------------
	//pci_ptr = &pci_callback;																// register pin change interrupt callback function
	sei();																					// enable interrupts

	// some test
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop

	// - user related -----------------------------------------
//	if (xtimer.done()) {
//		dbg << "r:" << get_external_voltage(&pin_D7, &pin_C6, 10, 45) << '\n';
//		xtimer.set(3000);
//	}
}


//- user functions --------------------------------------------------------------------------------------------------------
void cmRemote::initRemote(uint8_t channel) {
	// setting the essentials for the respective remote channel if necassary
	DBG(RE, F("initRemote: "), channel, '\n');

}


/* 
*  @brief this is the registered callback function for the pin change interrupt. there are 3 parameters as a hand over to identify the 
*  pin which has raised the interrupt and if it was a falling or raising edge. 
*  registering of this function has to be done in init by writing the callback function address into the callback pointer 'pci_ptr = &pci_callback;'
*  <vec> returns the port which had raised the interrupt. as vec is depending on the cpu type, you will find the definition in HAL_<vendor>_<type>. 
*  <pin> is the byte value of the pin which has raised the interrupt. 1 = pin0, 2 = pin1, 4 = pin2, 8 = pin3, 16 = pin4, etc... 
*  <flag> to indentify a falling or raising edge. 0 = falling, value above 0 = raising 
*/
void pci_callback(uint8_t vec, uint8_t pin, uint8_t flag) {
	dbg << F("v:") << vec << F(", p:") << pin << F(", f:") << flag << '\n';
}




