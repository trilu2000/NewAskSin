//- debug functionallity --------------------------------------------------------------------------------------------------
#include "00_debug-flag.h"


//- load library's --------------------------------------------------------------------------------------------------------
#include <newasksin.h>																		// ask sin framework
#include "register.h"																		// configuration sheet



//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------
	power_all_disable();																	// and everything else

	// - AskSin related ---------------------------------------
	DBG_START(SER, F("HM_LC_Dim1PWM\n"));
	DBG(SER, freeRam(), ' ', F(LIB_VERSION_STRING));
	init_millis_timer0();																	// init timer0
	hm.init();																				// init the asksin framework

	sei();																					// enable interrupts
	//dbg << "timer: " << timer_max << '\n';
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop

	// - user related -----------------------------------------
	
}


//- user functions --------------------------------------------------------------------------------------------------------
void CM_DIMMER::init_dimmer(uint8_t virtual_group, uint8_t virtual_channel, uint8_t channel) {
// setting the relay pin as output, could be done also by pinMode(3, OUTPUT)
	DBG(SER, F("initDim- vrt_grp: "), virtual_group, F(", vrt_cnl: "), virtual_channel, F(", cnl: "), channel, '\n');

// frequency calc = 8.000.000 / (2x ICR, 200) = 20.000 Hz

// atmega32u4 setting (pinC6, OC3A)
	set_pin_output(pinC6);																		// init the dimmer pin, OC3A
	power_timer3_enable();																		// enable the timer1 in power management

	ICR3 = 200;																					// 200 is the default max amount
	OCR3A = 200;																					// we start with off
	TCCR3A = _BV(COM3A1) | _BV(WGM31);															// output on OCR1, non inverted
	TCCR3B = _BV(WGM33) | _BV(CS30);															// mode 10, PWM, Phase Correct ICR1, prescaler to 1

// atmega328 setting
	//set_pin_output(pinB1);																	// init the dimmer pin, OCR1A
	//power_timer1_enable();																	// enable the timer1 in power management

	//ICR1 = 200;																				// 200 is the default max amount
	//OCR1A = 0;																				// we start with off
	//TCCR1A = _BV(COM1A1) | _BV(WGM11);														// Clear OC1A/OC1B on Compare Match when upcounting. Set OC1A / OC1B on Compare Match when downcounting. output on OCR1, non inverted
	//TCCR1B = _BV(WGM13)  | _BV(CS10);															// mode 10, PWM, Phase Correct ICR1, prescaler to 1
}

void CM_DIMMER::switch_dimmer(uint8_t virtual_group, uint8_t virtual_channel, uint8_t channel, uint8_t status, uint8_t pwm_multi) {
// switching the relay, could be done also by digitalWrite(3,HIGH or LOW)
	//DBG(SER, F("switchDim: "), virtual_group, channel, ", ", status, '\n' );
// atmega32u4 setting (pinC6, OC3A)



// atmega328 setting
	//ICR1 = 200 * pwm_multi;																	// pwm multiplier to set the pwm frequency
	//OCR1A = status * pwm_multi;																// set the PWM value to the pin
}




