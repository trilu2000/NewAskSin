#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																		// configuration sheet
#include <Dummy.h>

AS hm;																						// stage the asksin framework
Dummy dummy;																				// stage a dummy module


void setup() {
	#ifdef SER_DBG
	dbgStart();																				// serial setup
	Serial << F("Main\n");																	// ...and some information
	#endif
	
	// led's - D4 and D6
	pinOutput(DDRD,4);																		// init the led pins 
	pinOutput(DDRD,6);
	
	// config key pin - D8
	pinInput(DDRB,0);																		// init the config key pin
	setInHigh(PORTB,0);
	regPCIE(PCIE0);																			// set the pin change interrupt
	regPCINT(PCMSK0,PCINT0);																// description is in hal.h
	
	// init the homematic framework and register user modules
	hm.init();
	dummy.regInHM(1, 3, &hm);
	
	sei();																					// enable interrupts
}

void loop() {
	hm.poll();																				// poll the homematic main loop


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
