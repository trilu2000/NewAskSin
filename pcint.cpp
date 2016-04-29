// 
// 
// 

#include "pcint.h"

s_pcint_table pcint_table[PCINT_SLOTS];															// size the table were the interrupts will be registered
volatile s_pcint_vector_byte pcint_vector_byte[PCINT_PCIE_SIZE];								// size of the table depending on avr type
uint8_t pcint_table_usage;																		// how many slots are in use

void pcint_init(void) {
	memset((uint8_t*)pcint_vector_byte, 0x00, sizeof(pcint_vector_byte));						// clean the vector struct array
}

uint8_t pcinit_register(volatile uint8_t *pcint_MASK, volatile uint8_t *pcint_ICR, uint8_t pcint_IE, uint8_t pcint_IPIN, uint8_t pcint_DDR, volatile uint8_t *pcint_PORT, uint8_t pcint_PIN, uint8_t bool_Config, s_pcint_dlgt pcint_CallBack) {

	// prepare pcint_table
	if (pcint_table_usage >= PCINT_SLOTS) return 0;												// slot table is full

	// check if the interrupt pin is already in the table - if yes update, otherwise use a new slot
	pcint_table[pcint_table_usage].pin = pcint_IPIN;											// store the details
	pcint_table[pcint_table_usage].debounce = bool_Config;
	pcint_table[pcint_table_usage].ie = pcint_IE;
	pcint_table[pcint_table_usage].dlgt = pcint_CallBack;
	pcint_table_usage++;																		// slot was filled, increase the counter

	// prepare the interrupt mask and set or delete the bit in the previous vector byte to detect the first pin change
	pcint_vector_byte[pcint_IE].mask |= (1 << pcint_IPIN);										// add the interrupt pin to the mask
	if (bool_Config) pcint_vector_byte[pcint_IE].prev |= (1 << pcint_IPIN);						// pin is default high, next state will be low, so bit has to be set
	else pcint_vector_byte[pcint_IE].prev &= (1 << pcint_IPIN);

	// prepare the data direction register and the respective port register
	pinInput(pcint_DDR, pcint_PIN);																// configure pin as input

	if (bool_Config) setPinHigh(pcint_PORT, pcint_PIN);											// configure internal pull-up resistor
	else setPinLow(pcint_PORT, pcint_PIN);

	// prepare the interrupt related registers
	cli();																						// switch interrupts off while messing with their settings  
	regPCIE(pcint_ICR, pcint_IE);																// enable the interrupt vector in the interrupt change register
	regPCINT(pcint_MASK, pcint_IPIN);															// enable the interrupt pin number in the interrupt vector mask 
	sei();																						// interrupts on again
}




ISR(PCINT0_vect) {
	uint8_t pcint_byte;

	// cleanout the port byte and check if something has changed
	pcint_vector_byte[0].cur = PINB & pcint_vector_byte[0].mask;								// mask out all bits we like to doublecheck;															// read out the current port byte
	if (pcint_vector_byte[0].cur == pcint_vector_byte[0].prev) return;							// nothing to do

	// calculate the byte which was raising the interrupt
	pcint_byte = pcint_vector_byte[0].cur ^ pcint_vector_byte[0].prev;							// get the interrupt pin
	dbg << "i0:" << pcint_byte << ", i0c:" << pcint_vector_byte[0].cur << ", i0p:" << pcint_vector_byte[0].prev << "\n";	// some debug...

	// step throug  the table and search for the respective entry, check if enabled and then if we callback or debounce
																															 
	pcint_vector_byte[0].time = millis() + PCINT_DEBOUNCE_TIME;									// save the time for debouncing

	pcint_vector_byte[0].prev = pcint_vector_byte[0].cur;										// save the current port byte to identify if it was a raising or falling edge
}

ISR(PCINT1_vect) {
	pcint_vector_byte[1].prev = pcint_vector_byte[1].cur;
	pcint_vector_byte[1].cur = PINC;
	pcint_vector_byte[1].time = millis();
	dbg << "i2:" << PINC << "\n";
}

ISR(PCINT2_vect) {
	pcint_vector_byte[2].prev = pcint_vector_byte[2].cur;
	pcint_vector_byte[2].cur = PIND;
	pcint_vector_byte[2].time = millis();
	dbg << "i3:" << PIND << "\n";
}