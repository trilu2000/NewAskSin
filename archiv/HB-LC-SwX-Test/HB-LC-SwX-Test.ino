#define SER_DBG																				// serial debug messages

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>																				// ask sin framework
#include <Relay.h>																			// relay class module

#include "hardware.h"																		// hardware definition
#include "register.h"																		// configuration sheet

#define MOTOR_STOP   0
#define MOTOR_LEFT   1
#define MOTOR_RIGHT  2

uint8_t  motorState = 0;
uint8_t  motorStateLast = 0;
uint32_t nextMotorEvent = 0;

int32_t travelCount = 0;



// some forward declarations
void setupExtInterrupts();

void initRly();
void switchRly1(uint8_t status);
void switchRly2(uint8_t status);
void switchRly(uint8_t status, uint8_t ch);


//- load modules ----------------------------------------------------------------------------------------------------------
AS hm;																						// stage the asksin framework
Relay relay[2];																				// stage a dummy module
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
	
	hm.pw.setMode(POWER_MODE_NO_SLEEP);														// set power management mode

	hm.bt.set(27, 1800000);		// 1800000 = 0,5h											// set battery check

	relay[0].regInHM(1, 3, &hm);															// register relay module on channel 1, with a list3 and introduce asksin instance
	relay[0].config(&initRly, &switchRly1);													// hand over the relay functions of main sketch

	relay[1].regInHM(2, 3, &hm);															// register relay module on channel 2, with a list3 and introduce asksin instance
	relay[1].config(&initRly, &switchRly2);													// hand over the relay functions of main sketch

	setupExtInterrupts();

	sei();																					// enable interrupts

	// - user related -----------------------------------------
	dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");		// some debug
}


void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	
	// - user related -----------------------------------------
	pollExtInterrupts();
	pollStatus();
}

//- user functions --------------------------------------------------------------------------------------------------------

void setupExtInterrupts() {
	pinInput(DDRC, PORTC2);
	pinInput(DDRC, PORTC3);
	setPinHigh(PORTC,PORTC2);
	setPinHigh(PORTC,PORTC3);

	regPCIE(PCIE1);
	regPCINT(PCMSK1, PCINT10);
	regPCINT(PCMSK1, PCINT11);
}

void pollExtInterrupts() {
	if (chkPCINT(PCIE1, PCINT10) == 2) {
		motorState = MOTOR_STOP;
		travelCount = 0;
		dbg << F("c: ") << travelCount << "\n";
	}

	if (chkPCINT(PCIE1, PCINT11) == 2) {
		travelCount = (motorState == MOTOR_LEFT) ? (travelCount - 1) : (travelCount + 1);
		dbg << F("c: ") << travelCount << "\n";
	}

	if (travelCount >10 && motorState == MOTOR_RIGHT) {
		motorState = MOTOR_STOP;
	}
}

void pollStatus() {
	if (motorState != motorStateLast && nextMotorEvent == 0) {
		// Motor Stop
		setPinLow (PORTC, 0);
		setPinLow (PORTC, 1);
		nextMotorEvent = getMillis();
	}

	if ( (motorState != motorStateLast) &&  (getMillis() - nextMotorEvent) > 100 ) {		// Delay before motor change direction
		if (motorState == MOTOR_STOP) {
			relay[0].modStat = 0;
			relay[0].setStat = 0;
			relay[0].sendStat = 2;
			relay[1].modStat = 0;
			relay[1].setStat = 0;
			relay[1].sendStat = 2;

		} else if (motorState == MOTOR_LEFT) {
			setPinHigh(PORTC, 0);

			relay[1].modStat = 0;
			relay[1].setStat = 0;
			relay[1].sendStat = 2;

		} else if (motorState == MOTOR_RIGHT) {
			setPinHigh(PORTC, 1);

			relay[0].modStat = 0;
			relay[0].setStat = 0;
			relay[0].sendStat = 2;
		}

		motorStateLast = motorState;
		nextMotorEvent = 0;
	}
}

 void initRly() {
// setting the relay pin as output, could be done also by pinMode(3, OUTPUT)

	pinOutput(DDRC,0);																		// init the relay pins
	setPinLow(PORTC,0);																		// set relay pin to ground

	pinOutput(DDRC,1);																		// init the relay pins
	setPinLow(PORTC,1);																		// set relay pin to ground
}

void switchRly1(uint8_t status) {
	switchRly(status, 0);
}

void switchRly2(uint8_t status) {
	switchRly(status, 1);
}

void switchRly(uint8_t status, uint8_t ch) {
	dbg << F("switchRly: ") << ch << " - " << status << "\n";

	if (status > 0) {
		if (ch == 0) {
			motorState = MOTOR_LEFT;
		} else if (ch == 1)  {
			motorState = MOTOR_RIGHT;
		}
	} else {
		motorState = MOTOR_STOP;
	}
}
