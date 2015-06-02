#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>																				// the asksin framework
#include "register.h"																		// configuration sheet

// some forward declarations
void initPWM();
void switchPWM(uint8_t status, uint8_t characteristic);



//waitTimer xTmr;

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// everything off
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	//EIMSK = 0;																			// disable external interrupts
	//ADCSRA = 0;																			// ADC off
	//power_all_disable();																	// and everything else
	
	//DDRB = DDRC = DDRD = 0x00;															// everything as input
	//PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: led and config key should initialized internally
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	// enable only what is really needed


	#ifdef SER_DBG
		dbgStart();																			// serial setup
		dbg << F("HM_LC_Dim1PWM_CV\n");
		dbg << F(LIB_VERSION_STRING);
		_delay_ms (50);																		// ...and some information
	#endif
	
	
	// - AskSin related ---------------------------------------
	// init the homematic framework and register user modules
	hm.init();																				// init the asksin framework

	hm.confButton.config(2, CONFIG_KEY_PCIE, CONFIG_KEY_INT);								// configure the config button, mode, pci byte and pci bit
	
	hm.ld.init(2, &hm);																		// set the led
	hm.ld.set(welcome);																		// show something
	
	hm.pw.setMode(0);																		// set power management mode
	hm.bt.set(27, 3600000);		// 3600000 = 1h												// set battery check

	dimmer.regInHM(1, 3, &hm);																// register relay module on channel 1, with a list3 and introduce asksin instance
	dimmer.config(&initPWM, &switchPWM, NULL);

	sei();																					// enable interrupts

	
	// - user related -----------------------------------------
	dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");		// some debug
	
	//uint8_t xT[] = {0x15, 0xff, 0x16, 0xff};
	//hm.ee.setListArray(0, 0, 0, 4, xT);	

	uint8_t pT[] = {0x01,0x02,0x04,0x01,0x00};
	hm.ee.addPeer(1, pT);
	hm.ee.addPeer(2, pT);
	hm.ee.addPeer(3, pT);
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	

	// - user related -----------------------------------------

}


//- user functions --------------------------------------------------------------------------------------------------------
void initPWM() {
	dbg << "init pwm\n";
	
	power_timer2_enable();																	// enable the timer2 in power management
	
	pinOutput(DDRD,3);																		// init the relay pins
	//setPinLow(PORTD,3);
	
	TCCR2B |= (1<<CS21);																	// configure the PWM for the respective output pin
	OCR2B = 0x00;
	TCCR2A |= 1<<COM2B1;

}
void switchPWM(uint8_t status, uint8_t characteristic) {
	uint16_t x = status*255;
	//dbg << x << " ";
	x /= 200;																				// status = 0 to 200, but PWM needs 255 as maximum
	//dbg << x << '\n';
	OCR2B = x;																				// set the PWM value to the pin

	//if (status) setPinHigh(PORTD,3);
	//else setPinLow(PORTD,3);
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
