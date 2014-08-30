//#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																	// configuration sheet


void setup() {
	#ifdef SER_DBG
	Serial.begin(57600);																// serial setup
	Serial << F("Starting sketch...\n");												// ...and some information
	#endif
	
	//uint8_t xHMID[3]  = {0x01,0x02,0x03};
	//uint8_t xHMSR[10] = {'t','l','u','1','0','0','1','2','3','4'};
	//setEEPromBlock(2,3,(void*)&xHMID);
	//setEEPromBlock(5,10,(void*)&xHMSR);
	
	hm.init();

	//hm.ee.testModul();
	
	
}

void loop() {
	hm.poll();

}

void serialEvent() {
	static uint8_t i = 0;																		// it is a high byte next time
	while (Serial.available()) {
		uint8_t inChar = (uint8_t)Serial.read();												// read a byte
		if (inChar == '\n') {																	// send to receive routine
			i = 0;
			hm.sndActive = 1;
		}
		
		if      ((inChar>96) && (inChar<103)) inChar-=87;										// a - f
		else if ((inChar>64) && (inChar<71))  inChar-=55;										// A - F
		else if ((inChar>47) && (inChar<58))  inChar-=48;										// 0 - 9
		else continue;
		
		if (i % 2 == 0) hm.sn.buf[i/2] = inChar << 4;											// high byte
		else hm.sn.buf[i/2] |= inChar;															// low byte
		
		i++;
	}
}
