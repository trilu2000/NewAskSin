#define SER_DBG																				// serial debug messages

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>																				// ask sin framework
#include <Relay.h>																			// relay class module

#include "hardware.h"																		// hardware definition
#include "register.h"																		// configuration sheet

// some forward declarations
void initRly();
void switchRly(uint8_t status);


//- load modules ----------------------------------------------------------------------------------------------------------
AS hm;																						// stage the asksin framework
Relay relay;																				// stage a dummy module
//waitTimer wt;


//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {

	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	EIMSK = 0;																				// disable external interrupts
	ADCSRA = 0;																				// ADC off
	power_all_disable();																	// and everything else
	
	DDRB = DDRC = DDRD = 0x00;																// everything as input
	PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: led and config key should initialized internally
	initLeds();																				// initialize the leds
	initConfKey();																			// initialize the port for getting config key interrupts

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	// enable only what is really needed

	#ifdef SER_DBG
		dbgStart();																			// serial setup
		dbg << F("HM_LC_SW1_BA_PCB\n");	
		dbg << F(LIB_VERSION_STRING);
		_delay_ms (50);																		// ...and some information
	#endif

	
	// - AskSin related ---------------------------------------
	// init the homematic framework and register user modules
	hm.init();																				// init the asksin framework

	hm.confButton.config(2, CONFIG_KEY_PCIE, CONFIG_KEY_INT);								// configure the config button, mode, pci byte and pci bit
	
	hm.ld.init(2, &hm);																		// set the led
	hm.ld.set(welcome);																		// show something
	
	hm.pw.setMode(1);																		// set power management mode

	hm.bt.set(27, 1800000);		// 1800000 = 0,5h											// set battery check

	relay.regInHM(1, 3, &hm);																// register relay module on channel 1, with a list3 and introduce asksin instance
	relay.config(&initRly, &switchRly);														// hand over the relay functions of main sketch
	
	sei();																					// enable interrupts


	// - user related -----------------------------------------
	dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");		// some debug
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	
	// - user related -----------------------------------------
	
}


//- user functions --------------------------------------------------------------------------------------------------------
void initRly() {
// setting the relay pin as output, could be done also by pinMode(3, OUTPUT)

	pinOutput(DDRD,3);																		// init the relay pins
	setPinLow(PORTD,3);																		// set relay pin to ground
}
void switchRly(uint8_t status) {
// switching the relay, could be done also by digitalWrite(3,HIGH or LOW)

	if (status) setPinHigh(PORTD,3);														// check status and set relay pin accordingly
	else setPinLow(PORTD,3);
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
