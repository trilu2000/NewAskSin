#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>
#include "register.h"																		// configuration sheet
#include <THSensor.h>


//- load modules ----------------------------------------------------------------------------------------------------------
AS hm;																						// stage the asksin framework
THSensor thsens;																			// stage a dummy module

//waitTimer xTmr;

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	#ifdef SER_DBG
	dbgStart();																				// serial setup
	dbg << F("Main\n");																		// ...and some information
	#endif
	
	_delay_ms(100);
	_delay_ms(100);
	_delay_ms(100);
	_delay_ms(100);
	_delay_ms(100);
	dbg << F("Main\n");																		// ...and some information

	// - Hardware setup ---------------------------------------
	initMillis();																			// milli timer start
	initPCINT();																			// init the pin change interrupts
	ccInitHw();
		
	// everything off
	//ADCSRA = 0;																				// ADC off
	//power_all_disable();																	// and everything else
	
	//DDRB = DDRC = DDRD = 0x00;																// everything as input
	//PORTB = PORTC = PORTD = 0x00;															// pullup's off

	//power_spi_enable();																		// enable only needed functions
	//power_timer0_enable();
	//power_usart0_enable();

	// led's - D4 and D6
	//pinOutput(DDRD,4);																		// init the led pins
	//pinOutput(DDRD,6);
	
	// config key pin - D8
	//pinInput(DDRB,0);																		// init the config key pin
	//setPinHigh(PORTB,0);
	//initPCINT();																			// some sanity on interrupts	
	//regPCIE(PCIE0);																			// set the pin change interrupt
	//regPCINT(PCMSK0,PCINT0);																// description is in hal.h

	// battery measurement, ADC pin PC0, enable pin PD7
	//pinInput(DDRC, 0);																		// set the ADC pin as input
	//setPinLow(PORTC, 0);																	// switch off pull up
	// enable pin is set via macro in HAL.h

	
	// - AskSin related ---------------------------------------
	// init the homematic framework and register user modules
	hm.init();																				// init the asksin framework
	//hm.confButton.config(1,0,0);															// configure the config button, mode, pci byte and pci bit
	
	//hm.ld.init(2, &hm);																		// set the led
	//hm.ld.set(welcome);																		// show something
	
	//hm.pw.setMode(0);																		// set power management mode
	//hm.bt.set(1, 27, 3600000);		// 3600000 = 1h											// set battery check

	//thsens.regInHM(1, 4, &hm);																// register sensor module on channel 1, with a list4 and introduce asksin instance
	//thsens.config(&initTH1, &measureTH1, NULL);
	
	// - user related -----------------------------------------
	uint8_t x[5] = {0x01,0x11,0x02,0x22,0x03};
	dbg << "a:" << _HEX(x,5) << _TIME << '\n';
	dbg << "b:" << _HEXB(0xff) << '\n';
	sei();																					// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	
	//if (getMillis()%500 ==0) {
	//	dbg << getMillis() << '\n';
	//}

	// - user related -----------------------------------------

}


//- user functions --------------------------------------------------------------------------------------------------------
void initTH1() {
//	dbg << "init th1\n";
	
}
void measureTH1() {
//	dbg << "measure th1\n";

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
/*void serialEvent() {
while (Serial.available()) {
uint8_t inChar = (uint8_t)Serial.read();											// read a byte
if ((inChar>47) && (inChar<58))
hm.ld.rmb((ledStat)(inChar-48));
}
}*/
