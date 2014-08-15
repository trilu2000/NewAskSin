//#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																	// configuration sheet

MilliTimer timer1;

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
	
	//uint8_t xTmp[4]  = {0x01,0x02,0x03, 0x05};
	//hm.ee.isPairValid(xTmp);

	//hm.ee.clearPeers();
	//hm.ee.isPeerValid(xTmp);

	//hm.ee.countFreeSlots(1);
	//hm.ee.getPeerIdx(1, xTmp);
	//hm.ee.addPeer(1, 1, xTmp);
	//hm.ee.remPeer(1, 1);
	initMillis();																		// start the millis counter
	//timer1.poll(1000);
}

void loop() {
	hm.poll();
	if (timer1.poll(1000)) dbg << getMillis() << '\n';

	//_delay_ms(1000);
	//dbg << getMillis() << '\n';
	//dbg << '.';
	/* add main program code here */

}
