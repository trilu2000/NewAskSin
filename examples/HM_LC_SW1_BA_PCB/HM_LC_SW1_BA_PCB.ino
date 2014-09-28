#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																		// configuration sheet
#include <Dummy.h>


//- load modules ----------------------------------------------------------------------------------------------------------
AS hm;																						// stage the asksin framework
Dummy dummy;																				// stage a dummy module

waitTimer xTmr;

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	#ifdef SER_DBG
	dbgStart();																				// serial setup
	Serial << F("Main\n");																	// ...and some information
	#endif
	
	// - Hardware setup ---------------------------------------
	// led's - D4 and D6
	pinOutput(DDRD,4);																		// init the led pins
	pinOutput(DDRD,6);
	
	// config key pin - D8
	pinInput(DDRB,0);																		// init the config key pin
	setInHigh(PORTB,0);
	regPCIE(PCIE0);																			// set the pin change interrupt
	regPCINT(PCMSK0,PCINT0);																// description is in hal.h

	
	// - AskSin related ---------------------------------------
	// init the homematic framework and register user modules
	hm.init();																				// init the asksin framework
	hm.confButton.config(2,0,0);															// configure the config button, mode, pci byte and pci bit
	dummy.regInHM(1, 3, &hm);																// register dummy module on channel 1, with a list3 and introduce asksin instance
	
	hm.ld.init(2, &hm);																		// set the led
	hm.ld.set(welcome);																		// show something
	
	hm.pw.setMode(0);																		// set power management mode

	// - User related -----------------------------------------

	
	sei();																					// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	//hm.poll();																				// poll the homematic main loop
	if (xTmr.done()) {
		xTmr.set(250);
		dbg << hm.cc.detectBurst() << ' ';
		hm.cc.setIdle();
	}
	// - User related -----------------------------------------

}


//- user functions --------------------------------------------------------------------------------------------------------
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
/*void serialEvent() {
while (Serial.available()) {
uint8_t inChar = (uint8_t)Serial.read();											// read a byte
if ((inChar>47) && (inChar<58))
hm.ld.rmb((ledStat)(inChar-48));

}
}*/
