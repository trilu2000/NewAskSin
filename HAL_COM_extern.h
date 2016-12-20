/* Helper functions for the communication modul, 
*  all functions are defined in the AS_com.h as external. This file is needed to make the connection between the hardware.h 
*  definition file in the user sketch folder and the library folder. hierarchy looks like:
*  user folder/hardware.h with include oh HAL.h in library folder (HAL.h holds further hardware reference, like macros and arduino lib definition) 
*  user folder/hardware.cpp with include of hardware.h and HAL_COM_extern.h in library folder
*  HAL_COM_extern.h without any reference to any file */


/*
* @brief Simple SPI send function. Byte to transmit is written in the send buffer and processed.
* Wait for transfer finished is done by looping through the SPI register and checking the ready bit
* @param data	Byte to transfer
* @return		If an answer is given, it will be returned
*/
uint8_t spi_send_byte(uint8_t send_byte) {
	SPDR = send_byte;																		// send byte
	while (!(SPSR & _BV(SPIF))); 															// wait until transfer finished
	return SPDR;																			// return the data register
}

/*
* @brief Chip select and deselect for the cc1101 module. Select is combined with a wait function
* while we have to wait after a select till the modul is ready for communication
*/
void spi_select(void) {
	SET_PIN_LOW(COM_CS);
	while (GET_PIN_STATUS(COM_MISO));
}
void spi_deselect(void) {
	SET_PIN_HIGH(COM_CS);
}

/*
* @brief Initialisze the hardware setup of the cc1101 communication modul. No interrupt is used any more, while
* detecting a falling edge by constantly checking the GDO0 pin for a falling edge
*/
void cc1101_init() {
	SET_PIN_OUTPUT(COM_CS);																	// set chip select as output
	SET_PIN_OUTPUT(COM_MOSI);																// set MOSI as output
	SET_PIN_INPUT(COM_MISO);																// set MISO as input
	SET_PIN_OUTPUT(COM_SCLK);																// set SCK as output
	SET_PIN_INPUT(COM_GDO0);																// set GDO0 as input

	SPCR = _BV(SPE) | _BV(MSTR);															// SPI enable, master, speed = CLK/4
}

/*
* @brief Checks the GDO0 pin if the cc1101 modul had received some data, called by com->has_data() function
* with this function we are polling the GDO0 pin to detect a falling edge.
*/
uint8_t cc1101_has_data() {
	static uint8_t prev;
	uint8_t curr = GET_PIN_STATUS(COM_GDO0);												// get the current pin status
	if (prev == curr) return 0;																// if nothing changed since last request then return a 0

	prev = curr;																			// if we are here, a change was detected, so remember for next time
	if (curr) return 0;																		// change was detected, but not a falling edge
	else return 1;																			// if we are here, it was a falling edge, so return a 1																	
}
