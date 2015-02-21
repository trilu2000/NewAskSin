#include "hardware.h"
#include <HAL_hwfnck.inc>

//- assigment of cc1100 hardware CS and GDO0 definitions --------------------------------------------------------------
volatile uint8_t *cc_csDdr    = &CC_CS_DDR;										// SPI chip select definition
volatile uint8_t *cc_csPort   = &CC_CS_PORT;
uint8_t           cc_csPin    =  CC_CS_PIN;

volatile uint8_t *cc_gdo0Ddr  = &CC_GDO0_DDR;									// GDO0 pin, signals received data
uint8_t           cc_gdo0Pin  =  CC_GDO0_PIN;

volatile uint8_t *cc_gdo0Pcicr = &CC_GDO0_PCICR;								// GDO0 interrupt register
volatile uint8_t *cc_gdo0Pcmsk = &CC_GDO0_PCMSK;
uint8_t           cc_gdo0Pcie  =  CC_GDO0_PCIE;									// GDO0 interrupt mask
uint8_t           cc_gdo0Int   =  CC_GDO0_INT;									// pin interrupt


volatile uint8_t *ledRedDdr    = &LED_RED_DDR;									// define led port and remaining pin
volatile uint8_t *ledRedPort   = &LED_RED_PORT;
uint8_t           ledRedPin    =  LED_RED_PIN;

volatile uint8_t *ledGrnDdr    = &LED_GRN_DDR;									// define led port and remaining pin
volatile uint8_t *ledGrnPort   = &LED_GRN_PORT;
uint8_t           ledGrnPin    =  LED_GRN_PIN;
uint8_t           ledActiveLow =  LED_ACTIVE_LOW;

void    initWakeupPin(void) {
	#if defined(wakeupDDR)

	pinInput(wakeupDDR, wakeupPIN);															// set pin as input
	setPinHigh(wakeupPRT, wakeupPIN);														// enable internal pull up

	#endif
}


uint8_t checkWakeupPin(void) {
	// to enable the USB port for upload, configure PE2 as input and check if it is 0, this will avoid sleep mode and enable program upload via serial
	#if defined(wakeupDDR)

	if (getPin(wakeupPNR, wakeupPIN)) return 1;												// return pin is active
	#endif

	return 0;																				// normal operation
}
//- -----------------------------------------------------------------------------------------------------------------------

void    initPCINT(void) {
	memset((uint8_t*)pcInt, 0x00, sizeof(pcInt));
	//dbg << "a: " << pcInt[2].cur << '\n';
}
uint8_t chkPCINT(uint8_t port, uint8_t pin) {
	// returns pin status while no interrupt had happened for the pin, 2 for falling and 3 for rising edge

	uint8_t cur  = pcInt[port].cur  & _BV(pin);
	uint8_t prev = pcInt[port].prev & _BV(pin);

	if ((cur == prev) || ( (getMillis() - pcInt[port].time) < debounce )) {																		// old and new bit is similar, or debounce time is running
		return (pcInt[port].cur & _BV(pin))?1:0;
	}
	
	//if ( (getMillis() - pcInt[port].time) < debounce ) {
	//	dbg << "xxxx\n";
	//} 

	// detect rising or falling edge
	//dbg << pcInt[port].cur << ' ' << pcInt[port].prev << ' ';
	if (cur) {																				// pin is 1
		pcInt[port].prev |= _BV(pin);														// set bit bit in prev
		//dbg << "y3\n";
		return 3;
	} else {																				// pin is 0
		//dbg << "y2\n";
		pcInt[port].prev &= ~_BV(pin);														// clear bit in prev
		return 2;
	}
}

ISR (PCINT0_vect) {
	pcInt[0].prev = pcInt[0].cur;
	pcInt[0].cur = PINB;
	pcInt[0].time = getMillis();
}
ISR (PCINT1_vect) {
	pcInt[1].prev = pcInt[1].cur;
	pcInt[1].cur = PINC;
	pcInt[1].time = getMillis();
}
ISR (PCINT2_vect) {
	pcInt[2].prev = pcInt[2].cur;
	pcInt[2].cur = PIND;
	pcInt[2].time = getMillis();
}
//- -----------------------------------------------------------------------------------------------------------------------

uint8_t  getBatteryVoltage(void) {
	#if defined EXT_BATTERY_MEASUREMENT
		initExtBattMeasurement();
		switchExtBattMeasurement(1);
	#endif

	#if defined EXT_BATTERY_MEASUREMENT
		uint16_t adcValue = getAdcValue(										// Voltage Reference = Internal 1.1V; Input Channel = external battery measure pin
			(1 << REFS1) | (1 << REFS0) | battMeasPin
		);

		adcValue = adcValue * AVR_BANDGAP_VOLTAGE / 1023 / BATTERY_FACTOR;		// calculate battery voltage in V/10
		switchExtBattMeasurement(0);
	#else
		uint16_t adcValue = getAdcValue(										// Voltage Reference = AVCC with external capacitor at AREF pin; Input Channel = 1.1V (V BG)
			(0 << REFS1) | (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0)
		);

		adcValue = AVR_BANDGAP_VOLTAGE * 1023 / adcValue / 100;					// calculate battery voltage in V/10
	#endif

	return (uint8_t)adcValue;
}

void    initExtBattMeasurement(void) {
	pinInput(battMeasDDR, battMeasPin);											// set the ADC pin as input
	pinInput(battEnblDDR, battEnblPin);											// set the measurement enable pin as input, otherwise we waste energy over the resistor network against VCC
}

void    switchExtBattMeasurement(uint8_t stat) {
	if (stat == 1) {
		pinOutput(battEnblDDR, battEnblPin);									// set pin as out put
		setPinLow(battEnblPort, battEnblPin);									// set low to measure the resistor network
		setPinLow(battMeasPort, battMeasPin);
	} else {
		pinInput(battEnblDDR, battEnblPin);

		// todo: check
		setPinHigh(battMeasPort, battMeasPin);									// switch on pull up, otherwise we waste energy over the resistor network against VCC
	}
}
