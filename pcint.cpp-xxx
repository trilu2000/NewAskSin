//- -----------------------------------------------------------------------------------------------------------------------
// part of AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- pin change interrupt handler functions --------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#include "pcint.h"

s_pcint_table pcint_table[PCINT_SLOTS];															// size the table were the interrupts will be registered
volatile s_pcint_vector_byte pcint_vector_byte[PCINT_PCIE_SIZE];								// size of the table depending on avr type
uint8_t pcint_table_usage;																		// how many slots are in use
uint8_t pcint_debounce_active;																	// flag is used in the debounce function to avoid all time searching through the table



void pcint_init(void) {
	memset((uint8_t*)pcint_vector_byte, 0x00, sizeof(pcint_vector_byte));						// clean the vector struct array
}



uint8_t pcinit_register(volatile uint8_t *pcint_MASK, volatile uint8_t *pcint_ICR, uint8_t pcint_IE, uint8_t pcint_IPIN, uint8_t pcint_DDR, volatile uint8_t *pcint_PORT, volatile uint8_t *pcint_PINR, uint8_t pcint_PIN, uint8_t bool_Config, uint8_t bool_Debounce, s_pcint_dlgt pcint_CallBack) {

	// prepare pcint_table
	if (pcint_table_usage >= PCINT_SLOTS) return 0;												// slot table is full

	// check if the interrupt pin is already in the table - if yes update, otherwise use a new slot
	pcint_table[pcint_table_usage].pin = pcint_IPIN;											// store the details
	pcint_table[pcint_table_usage].debounce = bool_Debounce;									// shall we debounce the port?
	pcint_table[pcint_table_usage].ie = pcint_IE;
	pcint_table[pcint_table_usage].dlgt = pcint_CallBack;
	pcint_table_usage++;																		// slot was filled, increase the counter

	// prepare the interrupt mask and set or delete the bit in the previous vector byte to detect the first pin change
	pcint_vector_byte[pcint_IE].PINR = pcint_PINR;												// store the pointer to the port
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



void pcint_poll() {

	if (!pcint_debounce_active) return;															// nothing to do

	// step through the table and search for active interrupts which has to be debounced
	for (uint8_t i = 0; i < pcint_table_usage; i++) {
		if (!pcint_table[i].active) continue;													// check if the current row is active, if not check the next

		// if we are here, there is something to debounce
		if ( ( PCINT_MILLIS - pcint_table[i].time ) < PCINT_DEBOUNCE_TIME ) continue;				// time is not over yet
		pcint_table[i].dlgt((pcint_table[i].ie * 8) + pcint_table[i].pin, pcint_table[i].active - 1);// fire the callback
		pcint_table[i].active = 0;																// no need to jump in again
		pcint_debounce_active--;
	}
}



void pcint_process(uint8_t vector) {
	uint8_t pcint_byte;																			// variable to hold port byte 
	uint8_t pcint_bit;																			// variable for masking out the pin which raised the interrupt

	// is something to do?
	if (!pcint_table_usage) return;																// nothing in our table, leave the function

	// cleanout the port byte and check if something has changed
	pcint_byte = *pcint_vector_byte[vector].PINR & pcint_vector_byte[vector].mask;				// mask out all bits we like to doublecheck;
	if (pcint_byte == pcint_vector_byte[vector].prev) return;									// if there is no change, we have nothing to do

	// get the pin and search through the table, check if enabled and then if we callback or debounce
	pcint_bit = pcint_byte ^ pcint_vector_byte[vector].prev;									// get the interrupt pin

	for (uint8_t i = 0; i < pcint_table_usage; i++) { 

		// find the right entry by skipping the false
		if (pcint_table[i].ie != vector) continue;												// interrupt vector didn't match
		if (pcint_bit != (1 << pcint_table[i].pin) ) continue;									// pin number didn't match

		// if here we have the right entry and can check if debounce is needed
		if (pcint_table[i].debounce) {										
			// debounce is done in the poll function, so we have to setup only some flags
			pcint_table[i].time = PCINT_MILLIS;													// storing the time
			if (!pcint_table[i].active) pcint_debounce_active++;								// increase the armed counter only, if this interrupt was not active before
			pcint_table[i].active = (pcint_byte & pcint_bit) ? 2 : 1;							// making debounce poll active, raising (2) or falling (1) edge

		} else {
			// no debounce needed, fire the callback function - first argument is the pcint number, second if it was rising (1) or falling (0) edge
			// as we dont store the complete pcint number, we have to calculate it. assumption is, PCIE0  PCINT00:07, PCIE1 PCINT08:15, PCIE2 PCINT16:24  
			pcint_table[i].dlgt((pcint_table[i].ie * 8) + pcint_table[i].pin, (pcint_byte & pcint_bit) ? 1 : 0);
		}
		break;																					// nothing to do any more, end search
		//dbg << "i0:" << pcint_bit << ", i0c:" << pcint_byte << ", i0p:" << pcint_vector_byte[vector].prev << "\n\n";	// some debug...
	}
	pcint_vector_byte[vector].prev = pcint_byte;												// save the current port byte to identify if it was a raising or falling edge
}



// the respective interrupt handlers, every request will be forwarded to the interrupt processing function
ISR(PCINT0_vect) {
	pcint_process(0);
}

ISR(PCINT1_vect) {
	pcint_process(1);
}

ISR(PCINT2_vect) {
	pcint_process(2);
}

ISR(PCINT3_vect) {
	pcint_process(3);
}