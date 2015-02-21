#include "hardware.h"
#include <HAL_extern.h>

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
