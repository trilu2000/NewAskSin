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
	DBG(SER, F("initDim- vrt_grp: "), virtual_group, F(", vrt_cnl: "), virtual_channel, F(", cnl: "), channel, '\n');
	
	power_timer2_enable();																	// enable the timer2 in power management

	set_pin_output(&pin_D3);																// init the relay pins
	TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);							// COM2A1 and COM2B1, set the compare output mode to clear OC2A and OC2B on compare match; WGM21 and WGM20, fast PWM (mode 3 in datasheet).
	TCCR2B = _BV(WGM22) | _BV(CS22);														// fast PWM mode 7 —  WGM22, reset counter on OCR2A value instead of the default 255; CS20, set the prescaler divisor to 1
	OCR2A = 64;																				// OCR2A holds the top value of our counter, so it acts as a divisor to the clock. when our counter reaches this, it resets.
	OCR2B = 0;

	//TCCR2B |= (1 << CS21);																	// configure the PWM for the respective output pin
	//OCR2B = 0x00;
	//TCCR2A |= 1 << COM2B1;
}

void CM_DIMMER::switch_dimmer(uint8_t virtual_group, uint8_t virtual_channel, uint8_t channel, uint8_t status) {
// switching the relay, could be done also by digitalWrite(3,HIGH or LOW)
	//DBG(SER, F("switchDim: "), cnl_group, channel, ", ", status, '\n' );

	uint16_t x = status * 255;
	x /= 200;																				// status = 0 to 200, but PWM needs 255 as maximum
	OCR2B = x;																				// set the PWM value to the pin

	//if (status) setPinHigh(PORTD,3);														// here you could switch on an additional power supply
	//else setPinLow(PORTD,3);
}




