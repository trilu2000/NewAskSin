
#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>																				// the asksin framework
#include "register.h"																		// configuration sheet

#define MOTOR_STOP   0
#define MOTOR_LEFT   1
#define MOTOR_RIGHT  2

uint8_t  motorState = 0;
uint8_t  motorStateLast = 0;
uint32_t nextMotorEvent = 0;

int32_t travelCount = 0;

// some forward declarations
uint16_t freeMem();

//waitTimer xTmr;

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// everything off
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

//	EIMSK = 0;																			// disable external interrupts
//	ADCSRA = 0;																			// ADC off
//	power_all_disable();																	// and everything else
	
//	DDRB = DDRC = DDRD = 0x00;															// everything as input
//	PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	// enable only what is really needed
	
	
	#ifdef SER_DBG
		dbgStart();																			// serial setup
		dbg << F("Starting sketch for HB-UW-Mot-BL1...\n");
		dbg << F(LIB_VERSION_STRING);
		dbg << F("freeMem: ") << freeMem() << F(" byte") << F("\n");
		_delay_ms (50);																		// ...and some information
	#endif
	
	
	// - AskSin related ---------------------------------------
	hm.init();																				// init the asksin framework


	sei();																					// enable interrupts


	// - user related -----------------------------------------
	#ifdef SER_DBG
		dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");		// some debug
	#endif
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																				// poll the homematic main loop
	

	// - user related -----------------------------------------

}


//- user functions --------------------------------------------------------------------------------------------------------
void initBlind(uint8_t channel) {
	dbg << "init pwm\n";

	power_timer2_enable();																	// enable the timer2 in power management

	pinOutput(DDRD,3);																		// init the relay pins

	TCCR2B |= (1<<CS21);																	// configure the PWM for the respective output pin
	OCR2B = 0x00;
	TCCR2A |= 1<<COM2B1;
}

void switchBlind(uint8_t status, uint8_t channel) {
	#ifdef SER_DBG
		dbg << F("switchDim: ") << channel << ", " << status << "\n";
	#endif

	uint16_t x = status*255;

	//dbg << x << " ";
	x /= 200;																				// status = 0 to 200, but PWM needs 255 as maximum

	//dbg << x << '\n';
	OCR2B = x;																				// set the PWM value to the pin
}










extern uint16_t __bss_end, _pHeap_start;
extern void *__brkval;
uint16_t freeMem() {															// shows free memory
	uint16_t free_memory;

	if((uint16_t)__brkval == 0) {
		free_memory = ((uint16_t)&free_memory) - ((uint16_t)&__bss_end);
	} else {
		free_memory = ((uint16_t)&free_memory) - ((uint16_t)__brkval);
	}

	return free_memory;
}
