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
	
	initHW();																				// initialize the hardware
	led1_on();
	// control led
	//pinMode(3,OUTPUT);
	//digitalWrite(3,1);

	hm.init();
	dummy.regInHM(1, 3, &hm);
	
}

void loop() {
	hm.poll();
	if (PINB & _BV(0)) led0_on();
	else led0_off();
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
