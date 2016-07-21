#define SER_DBG																				// serial debug messages

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																		// configuration sheet
#include <AS.h>																				// ask sin framework



//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {

	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	EIMSK = 0;																				// disable external interrupts
	ADCSRA = 0;																				// ADC off
	power_all_disable();																	// and everything else
	
	DDRB = DDRC = DDRD = 0x00;																// everything as input
	PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	// enable only what is really needed

	#ifdef SER_DBG																			// some debug
		dbgStart();																			// serial setup
		dbg << F("HM_PB_6_WM55\n");	
		dbg << F(LIB_VERSION_STRING);
		_delay_ms (50);																		// ...and some information
	#endif

	// - AskSin related ---------------------------------------
	hm.init();																				// init the asksin framework
	sei();																					// enable interrupts


	// - user related -----------------------------------------
	registerPCINT(PIN_C0);																	// register the pin change interrupt for hw keys
	registerPCINT(PIN_C1);
	registerPCINT(PIN_C2);
	registerPCINT(PIN_C3);
	registerPCINT(PIN_C4);
	registerPCINT(PIN_C5);


	#ifdef SER_DBG
		dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");	// some debug
	#endif

	// show channel table content
	dbg << "channel table:\n";
	dbg << "line\tcnl\tlst\tsIdx\tsLen\thide\tpAddr\n";
	for (uint8_t i = 0; i < (sizeof(cnlTbl)/sizeof(EE::s_cnlTbl)); i++) {
		// cnl, lst, sIdx, sLen, hide, pAddr 
		dbg << i << "\t" << cnlTbl[i].cnl << "\t" << cnlTbl[i].lst << "\t" << cnlTbl[i].sIdx << "\t" << cnlTbl[i].sLen << "\t" << cnlTbl[i].vis << "\t" << cnlTbl[i].pAddr << "\n";
	}
}




void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	
	// - user related -----------------------------------------
	
}



//- user functions --------------------------------------------------------------------------------------------------------
/**
* @brief Callback function for pin change interrupt. Has to be enabled in hardware.h by #define PCINT_CALLBACK.
* Will be called every time a registered pin change interrupt had happened. Declaration of callback is done in
* HAL_extern.h automatically.
* @param   vec     Indicates the vector were the interrupt was raised
* @param   pin     Indicates the pin byte (1,2,4,8,16,32,64,128)
* @param   flag    Indicates the value of the port pin (1 = high, 0 = low)
*/
void pci_callback(uint8_t vec, uint8_t pin, uint8_t flag) {
	//dbg << "cb, vec:" << vec << ", pin:" << pin << ", flag:" << flag << '\n';
}



//- predefined functions --------------------------------------------------------------------------------------------------
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
