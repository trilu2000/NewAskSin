//- load library's --------------------------------------------------------------------------------------------------------
#include <newasksin.h>																		// ask sin framework
#include "waittimer.h"

waitTimer xtimer;
uint8_t loop_counter = 0;


void setup() {

/* intent of this sketch is to test the timer functionallity, defined in HAL.h
*  depending on your hardware and the respective definition you can use several timers in the newasksin library.
*  please set the value below to the number of the timer you want to test */
	uint8_t timer_nr = 1;

	Serial.begin(57600);
	dbg << F("\n\nthis is a test sketch of the Newasksin library to test the timer functionallity...\n\n");
	dbg << F("start timer ") << timer_nr << F(" and wait 5 sec. \n");
	init_millis_timer0(timer_nr);															// init the timer
	_delay_ms(5000);
	dbg << F("after 5000ms delay: ") << get_millis() << '\n';

	dbg << F("now we are starting a quick check of the accuracy of the timer. \n");
	dbg << F("we start the timer and show every second a new dot, after 60 seconds an x\n");
	dbg << F("you can check the time with an external stop watch...\n");
	_delay_ms(1000);

}

void loop() {

	if (loop_counter == 60) {
		dbg << "x " << get_millis() << '\n';
		loop_counter = 0;
	} 
	
	if (xtimer.done()) {
		dbg << '.';
		loop_counter++;
		xtimer.set(1000);
	}

}