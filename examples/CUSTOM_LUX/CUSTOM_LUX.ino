#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>
#include "register.h"																		// configuration sheet
#include <THSensor.h>

//- define hardware -------------------------------------------------------------------------------------------------------
//- LED's 
#define ledRedDDR     DDRB																	// define led port and remaining pin
#define ledRedPort    PORTB
#define ledRedPin     PORTB7

#define ledGrnDDR     DDRC
#define ledGrnPort    PORTC
#define ledGrnPin     PORTC7

#define ledActiveLow  1																		// leds against GND = 0, VCC = 1

//- configuration key
#define confKeyDDR    DDRB																	// define config key port and remaining pin
#define confKeyPort   PORTB
#define confKeyPin    PORTB0

#define confKeyPCICR  PCICR																	// interrupt register
#define confKeyPCIE   PCIE0																	// pin change interrupt port bit
#define confKeyPCMSK  PCMSK0																// interrupt mask
#define confKeyINT    PCINT0																// pin interrupt

//- external battery measurement
#define externalMeasurement 1																// set to 1 to enable external battery measurement, to switch off or measure internally set it to 0

#define battEnblDDR   DDRF																	// define battery measurement enable pin, has to be low to start measuring
#define battEnblPort  PORTF
#define battEnblPin   PORTF4

#define battMeasDDR   DDRF																	// define battery measure pin, where ADC gets the measurement
#define battMeasPort  PORTF
#define battMeasPin   PORTF7
// external measurement to be reworked


//- load modules ----------------------------------------------------------------------------------------------------------
AS hm;																						// stage the asksin framework
THSensor thsens;																			// stage a dummy module

waitTimer xt;

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	#ifdef SER_DBG
	dbgStart();																				// serial setup
	dbg << F("Main\n");																		// ...and some information
	#endif
	
	// - everything off ---------------------------------------
	//ADCSRA = 0;																				// ADC off
	//power_all_disable();																	// and everything else
	
	//DDRB = DDRC = DDRD = 0x00;																// everything as input
	//PORTB = PORTC = PORTD = 0x00;															// pullup's off

	//power_spi_enable();																		// enable only needed functions
	//power_timer0_enable();
	//power_usart0_enable();


	// - Hardware setup ---------------------------------------
	initMillis();																			// milli timer start
	initPCINT();																			// initialize the pin change interrupts
	ccInitHw();																				// initialize transceiver hardware
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts
	initExtBattMeasurement();																// initialize the external battery measurement
	
	
	// - AskSin related ---------------------------------------
	// init the homematic framework and register user modules
	hm.init();																				// init the asksin framework
	hm.confButton.config(1,0,0);															// configure the config button, mode, pci byte and pci bit
	
	hm.ld.init(2, &hm);																		// set the led
	hm.ld.set(welcome);																		// show something
	
	hm.pw.setMode(0);																		// set power management mode
	hm.bt.set(1, 27, 3600000);		// 3600000 = 1h											// set battery check

	//thsens.regInHM(1, 4, &hm);																// register sensor module on channel 1, with a list4 and introduce asksin instance
	//thsens.config(&initTH1, &measureTH1, NULL);
	
	// - user related -----------------------------------------
	//uint8_t x[5] = {0x01,0x11,0x02,0x22,0x03};
	//dbg << "a:" << _HEX(x,5) << _TIME << '\n';
	//dbg << "b:" << _HEXB(0xff) << '\n';

	//xt.set(100);
	sei();																					// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop


	// - user related -----------------------------------------
	//if (xt.done()) {
	//	dbg << getBatteryVoltageExternal() << '\n';
	//	xt.set(1000);
	//}
}


//- user functions --------------------------------------------------------------------------------------------------------
void initTH1() {
	dbg << "init th1\n";
	
}
void measureTH1() {
	dbg << "measure th1\n";

}

//- predefined functions --------------------------------------------------------------------------------------------------
void serialEvent(void) {
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
void initLeds(void) {
	pinOutput(ledRedDDR,ledRedPin);															// set the led pins in port
	pinOutput(ledGrnDDR,ledGrnPin);
	if (ledActiveLow) {
		setPinHigh(ledRedPort, ledRedPin);
		setPinHigh(ledGrnPort, ledGrnPin);
	}
}
void ledRed(uint8_t stat) {
	stat ^= ledActiveLow;
	if      (stat == 1) setPinHigh(ledRedPort, ledRedPin);
	else if (stat == 0) setPinLow(ledRedPort, ledRedPin);
	else                setPinCng(ledRedPort, ledRedPin);
}
void ledGrn(uint8_t stat) {
	stat ^= ledActiveLow;
	if      (stat == 1) setPinHigh(ledGrnPort, ledGrnPin);
	else if (stat == 0) setPinLow(ledGrnPort, ledGrnPin);
	else                setPinCng(ledGrnPort, ledGrnPin);
}
void initConfKey(void) {
	// set port pin and register pin interrupt
	pinInput(confKeyDDR, PORTB0);															// init the config key pin
	setPinHigh(confKeyPort,PORTB0);

	initPCINT();																			// some sanity on interrupts
	regPCIE(confKeyPCIE);																	// set the pin change interrupt
	regPCINT(confKeyPCMSK,confKeyINT);														// description is in hal.h
}
void initExtBattMeasurement(void) {
	if (!externalMeasurement) return;														// return while external measurement is disabled

	pinInput(battMeasDDR, PORTF7);															// set the ADC pin as input
	//setPinHigh(battMeasPort, PORTF7);														// switch on pull up, otherwise we waste energy over the resistor network against VCC
	pinInput(battEnblDDR, PORTF4);															// set the measurement enable pin as input, otherwise we waste energy over the resistor network against VCC
}
void switchExtBattMeasurement(uint8_t stat) {
	if (stat) {
		pinOutput(battEnblDDR, PORTF4);														// set pin as out put
		setPinLow(battEnblPort, PORTF4);													// set low to measure the resistor network
	} else pinInput(battEnblDDR, PORTF4);
}