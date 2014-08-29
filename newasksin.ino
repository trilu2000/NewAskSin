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
	
	//uint8_t xTmp[4]  = {0x01,0x02,0x03, 0x05};
	//hm.ee.isPairValid(xTmp);

	//hm.ee.clearPeers();
	//hm.ee.isPeerValid(xTmp);

	//hm.ee.countFreeSlots(1);
	//hm.ee.getPeerIdx(1, xTmp);
	//hm.ee.addPeer(1, 1, xTmp);
	//hm.ee.remPeer(1, 1);
	//timer1.poll(1000);

	//hm.ee.clearPeers();
	
	// add peer
	//uint8_t buf[] = {0x01,0x02,0x03,0x04,0x05};
	//hm.ee.addPeer(1,buf);

	// display peer space
	//uint8_t xuf[20], cnt;
	//uint8_t slc = hm.ee.countPeerSlc(1);
	
	//for (uint8_t i = 0; i < slc; i++) {
	//	cnt = hm.ee.getPeerListSlc(1,i,xuf);
	//	dbg << slc << ": " << pHex(xuf,cnt) << '\n';
	//}
	hm.sendDEVICE_INFO();
	
}

void loop() {
	hm.poll();

	//_delay_ms(1000);
	//dbg << getMillis() << '\n';
	//dbg << '.';
	/* add main program code here */

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
		
		if (i % 2 == 0) hm.sndBuf[i/2] = inChar << 4;											// high byte
		else hm.sndBuf[i/2] |= inChar;															// low byte
		
		i++;
	}
}
