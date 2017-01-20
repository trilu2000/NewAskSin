//- load library's --------------------------------------------------------------------------------------------------------
#include <newasksin.h>																		// ask sin framework
#include "waittimer.h"

LED *led = new LED(&pin_F4, &pin_F5);														// replace pins with your hw setup


void setup() {

/* intent of this sketch is to test the led functionallity but also the pin functionallity defined in HAL.h
*  to run the led modul you have to ensure that the timer is working and initialized */
	init_millis_timer0();																	// initialize and start the timer

	_delay_ms(400);																			// some delay untill the usb port is settled
	Serial.begin(57600);
	dbg << F("\n\nthis is a test sketch of the Newasksin library to test the LED functionallity...\n\n");
	dbg << F("we start with a welcome message \n");
	led->init();																			// init the hardware
	led->set(LED_STAT::WELCOME);															// set a message

}

void loop() {
	led->poll();																			// poll the module

}