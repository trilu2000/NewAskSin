//- debug functionallity --------------------------------------------------------------------------------------------------
#include "00_debug-flag.h"


//- load library's --------------------------------------------------------------------------------------------------------
#include <newasksin.h>																		// ask sin framework
#include "register.h"																		// configuration sheet
#include "waittimer.h"

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------
	power_all_disable();																	// and everything else
	EIMSK = 0;																				// disable external interrupts
	ADCSRA = 0;																				// ADC off

	//DDRB = DDRC = DDRD = 0x00;															// everything as input
	//PORTB = PORTC = PORTD = 0x00;															// pullup's off

	DBG_START(SER, F("HM_PB_6_WM55\n"));
	DBG(SER, F(LIB_VERSION_STRING));

	// - AskSin related ---------------------------------------
	init_millis_timer0();																	// init timer0
	hm->init();																				// init the asksin framework

	sei();																					// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	hm->poll();																				// poll the homematic main loop
	
	// - user related -----------------------------------------
	
}


//- user functions --------------------------------------------------------------------------------------------------------
void CM_DIMMER::init_dimmer(uint8_t virtual_group, uint8_t virtual_channel, uint8_t channel) {
// setting the relay pin as output, could be done also by pinMode(3, OUTPUT)
	//DBG(SER, F("initDim- vrt_grp: "), *virtual_group, F(", vrt_cnl: "), *virtual_channel, F(", cnl: "), *channel, '\n');
	
	set_pin_output(&pin_D3);																// init the dimmer pin
	power_timer2_enable();																	// enable the timer2 in power management

	TCCR2A = _BV(COM2B1) | _BV(WGM20);
	TCCR2B = _BV(CS20);																		// configure the PWM for the respective output pin, no prescaler
	OCR2B = 0;
}

void CM_DIMMER::switch_dimmer(uint8_t virtual_group, uint8_t virtual_channel, uint8_t channel, uint8_t status) {
// switching the relay, could be done also by digitalWrite(3,HIGH or LOW)
	//DBG(SER, F("switchDim: "), *virtual_group, *channel, ", ", *status, '\n' );

	uint16_t x = status * 255;
	x /= 200;																				// status = 0 to 200, but PWM needs 255 as maximum
	OCR2B = x;																				// set the PWM value to the pin

	//if (status) setPinHigh(PORTD,3);														// here you could switch on an additional power supply
	//else setPinLow(PORTD,3);
}




