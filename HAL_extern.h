
/**
*
* @brief Some definitions for the pin change interrupt setup
*
* <pcint_vector_byte> array holds current information per interrupt vector
* <pcint_table> array stores the pin change interrupt specific information and a call back address
* size information a stored in the hardware.h file in user sketch area
*
*/
//volatile s_pcint_vector_byte pcint_vector_byte[PCINT_PCIE_SIZE];
#ifdef PCINT_CALLBACK
extern void pci_callback(uint8_t vec, uint8_t pin, uint8_t flag);
#endif







/**
* @brief Function to register a pin change interrupt. Port is set within this function, but also gets the pin registered in the vector struct
* and pin change registers set automatically.
* @ param    simple use the defined pins from HAL_atmega.h like PIN_B2
*/
/*void    registerPCINT(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC) {
	_SET_PIN_INPUT(DDREG, PINBIT);																// set the pin as input
	_SET_PIN_HIGH(PORTREG, PINBIT);																// key is connected against ground, set it high to detect changes

	pcint_vector_byte[VEC].PINR =  PINREG;														// set the vector struct
	pcint_vector_byte[VEC].curr |=  _GET_PIN_STATUS(PINREG, PINBIT);
	pcint_vector_byte[VEC].prev  = pcint_vector_byte[VEC].curr;
	pcint_vector_byte[VEC].mask |= _BV(PINBIT);

	_REG_PCI_ICR(PCICREG, PCIEREG);																// make the pci active
	_REG_PCI_PIN(PCIMASK, PCIBYTE);
}*/

/**
* @brief This function checks is polled while an interrupt pin is registered and here is the handover.
* This functions returns a 3 for a raising edge, 2 for a falling edge, 1 if no interrupt had happend and the pin is high 
* and a 0 if the pin is low.
*
* @param port   The interrupt vector, similar to PCIE0
* @param pin    The Pin we had registered, PORTB2 or PCINT17
* @param debounce: when true (1) then wait DEBOUNCE time before returning new status
*/
/*uint8_t checkPCINT(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC, uint8_t debounce) {
	return checkPCINT(VEC, PINBIT, debounce);
}*/
/*uint8_t checkPCINT(uint8_t port, uint8_t pin, uint8_t debounce) {

	uint8_t status = pcint_vector_byte[port].curr & _BV(pin) ? 1 : 0;							// evaluate the pin status
	uint8_t prev = pcint_vector_byte[port].prev & _BV(pin) ? 1 : 0;								// evaluate the previous pin status

	if (status == prev) return status;															// check if something had changed since last time
	if (debounce && ((get_millis() - pcint_vector_byte[port].time) < DEBOUNCE)) return status;	// seems there is a change, check if debounce is necassary and done
	
	pcint_vector_byte[port].prev ^= _BV(pin);													// if we are here, there was a change and debounce check was passed, remember for next time

	if (status) return 3;																		// pin is 1, old was 0
	else return 2;																				// pin is 0, old was 1
}*/

/**
* @brief This function is called from the different interrupt vector functions while an interrupt 
* had happened. Within this function we check also if there is a callback for an interrupt registered.
*
* @param vec    Is the vector byte, were the interrupt comes from
*/
/*void    maintainPCINT(uint8_t vec) {
	pcint_vector_byte[vec].curr = *pcint_vector_byte[vec].PINR  & pcint_vector_byte[vec].mask;	// read the pin port and mask out only pins registered
	pcint_vector_byte[vec].time = get_millis();													// store the time, if debounce is asked for

	#ifdef PCINT_CALLBACK																		// callback only needed if defined in hardware.h
	uint8_t bInt = pcint_vector_byte[vec].curr ^ pcint_vector_byte[vec].prev;					// evaluate the pin which raised the interrupt
	pci_callback(vec, bInt, pcint_vector_byte[vec].curr & bInt);								// callback the interrupt function in user sketch
	#endif
}*/
//- -----------------------------------------------------------------------------------------------------------------------



//- -----------------------------------------------------------------------------------------------------------------------


/*************************************
*** Battery measurement functions ***
*************************************/
//void    initExtBattMeasurement(void);
//void    switchExtBattMeasurement(uint8_t stat);

/**
* get the voltage off battery
*/
/*uint8_t  getBatteryVoltage(void) {
#if defined EXT_BATTERY_MEASUREMENT
	initExtBattMeasurement();
	switchExtBattMeasurement(1);
#endif

#if defined EXT_BATTERY_MEASUREMENT
	uint16_t adcValue = getAdcValue(										// Voltage Reference = Internal 1.1V; Input Channel = external battery measure pin
		(1 << REFS1) | (1 << REFS0) | BATT_MEASURE_PIN
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
}*/

/**
* Initialize battery measurement pin for external battery measurement
*/
/*void    initExtBattMeasurement(void) {
	SET_PIN_INPUT(BATT_MEASURE);						// set the ADC pin as input
	SET_PIN_INPUT(BATT_ENABLE);						// set the measurement enable pin as input, otherwise we waste energy over the resistor network against VCC
}*/

/**
* activate / deactivate battery measurement pin
* @param	stat	1: activate, 0: deactivate
*/
/*void    switchExtBattMeasurement(uint8_t stat) {
	if (stat == 1) {
		SET_PIN_OUTPUT(BATT_ENABLE);					// set pin as out put
		SET_PIN_LOW(BATT_ENABLE);						// set low to measure the resistor network
		SET_PIN_LOW(BATT_MEASURE);
	}
	else {
		SET_PIN_INPUT(BATT_ENABLE);

		// todo: check
		SET_PIN_HIGH(BATT_MEASURE);					// switch on pull up, otherwise we waste energy over the resistor network against VCC
	}
}*/
//- -----------------------------------------------------------------------------------------------------------------------



