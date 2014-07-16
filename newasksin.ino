//#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																	// configuration sheet


void setup() {
	#ifdef SER_DBG
	Serial.begin(57600);																	// serial setup
	Serial << F("Starting sketch...\n");													// ...and some information
	#endif
	
	//uint8_t xHMID[3]  = {0x01,0x02,0x03};
	//uint8_t xHMSR[10] = {'t','l','u','1','0','0','1','2','3','4'};
	//setEEPromBlock(2,3,(void*)&xHMID);
	//setEEPromBlock(5,10,(void*)&xHMSR);

	hm.init();
	hm.ee.printRegDef();


	//_delay_ms(1000);
	


}

void loop() {

  /* add main program code here */

}
