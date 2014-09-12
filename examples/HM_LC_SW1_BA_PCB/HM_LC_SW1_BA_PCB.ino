#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																		// configuration sheet
#include <Dummy.h>

AS hm;
Dummy dummy;


void setup() {
	#ifdef SER_DBG
	dbgStart();																				// serial setup
	Serial << F("Starting sketch...\n");													// ...and some information
	#endif
	
	pinOutput(DDRD,4);																		// init the led pins 
	pinOutput(DDRD,6);
	
	pinInput(DDRB,0);																		// init the config key pin
	setInHigh(PORTB,0);
	regPCIE(PCIE0);																			// set the pin change interrupt
	regPCINT(PCMSK0,PCINT0);																// description is in hal.h
	
	led1_on();

	hm.init();
	dummy.regInHM(1, 3, &hm);
	
	sei();	
}

void loop() {
	hm.poll();

	uint8_t chkKey = chkPCINT(0, 0);														// check input pin
	if      (chkKey == 2) led0_on();														// set led accordingly
	else if (chkKey == 1) led0_off();

}

void serialEvent() {
	#ifdef SER_DBG
	
	static uint8_t i = 0;																	// it is a high byte next time
	while (Serial.available()) {
		uint8_t inChar = (uint8_t)Serial.read();											// read a byte
		if (inChar == '\n') {																// send to receive routine
			i = 0;
			hm.sn.active = 1;
		}
		
		if      ((inChar>96) && (inChar<103)) inChar-=87;									// a - f
		else if ((inChar>64) && (inChar<71))  inChar-=55;									// A - F
		else if ((inChar>47) && (inChar<58))  inChar-=48;									// 0 - 9
		else continue;
		
		if (i % 2 == 0) hm.sn.buf[i/2] = inChar << 4;										// high byte
		else hm.sn.buf[i/2] |= inChar;														// low byte
		
		i++;
	}
	#endif
}
