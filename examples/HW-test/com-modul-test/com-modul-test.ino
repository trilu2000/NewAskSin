//- load library's --------------------------------------------------------------------------------------------------------
#include <newasksin.h>																		// ask sin framework

/* here you have to set the respective spi pins, you will find the definition in HAL_<cpu_type>.h in the library folder */
const s_pin_def *p_miso = &pin_B3;
const s_pin_def *p_mosi = &pin_B2;
const s_pin_def *p_sck = &pin_B1;
const s_pin_def *p_csl = &pin_B0;
const s_pin_def *p_gdo0 = &pin_B5;

COM *com = new CC1101(p_miso, p_mosi, p_sck, p_csl, p_gdo0);
//COM *com = new CC1101(&pin_B3, &pin_B2, &pin_B1, &pin_B0, &pin_B5); // 32u4; miso, mosi, sck, csl, gdo0
//COM *com = new CC1101(&pin_B4, &pin_B3, &pin_B5, &pin_B2, &pin_D2); // 328p; miso, mosi, sck, csl, gdo0


uint8_t rcv_str[50];



void setup() {

/* intent of this sketch is to test the communication modul interface and functionallity as defined in HAL.h
*  prerequisite is a started timer, please ensure that at least one timer is working and started */
	init_millis_timer0();																	// init and start the timer
	_delay_ms(400);																			// wait till usb port is settled

	Serial.begin(57600);
	dbg << F("\n\nthis is a test sketch of the Newasksin library to test the communication modul interface and functionallity...\n\n");
	dbg << F("in a first step we will set the interface and check the communication with the modul, \n");
	dbg << F("if that test was passed, we swiitch over to the communication modul and poll it regullary...\n");


	/* init the hardware to get access to the RF modul, some deselect and selects to init the TRX868modul */
	enable_spi();																			// enable spi
	set_pin_output(p_csl);																	// set chip select as output
	set_pin_high(p_csl);																	// while module is low active
	set_pin_output(p_mosi);																	// set MOSI as output
	set_pin_input(p_miso);																	// set MISO as input
	set_pin_output(p_sck);																	// set SCK as output
	set_pin_input(p_gdo0);																	// set GDO0 as input

	spi_deselect();
	_delay_us(5);
	spi_select();
	_delay_us(10);
	spi_deselect();
	_delay_us(41);

	/* send a reset and wait some time for restart of the the device */
	strobe(CC1101_SRES);
	_delay_ms(10);

	/* now we should be able to read the partnumber */
	uint8_t part_num = readReg(CC1101_PARTNUM, CC1101_STATUS);
	uint8_t part_ver = readReg(CC1101_VERSION, CC1101_STATUS);
	dbg << F("part number: ") << part_num << F(", version: ") << part_ver << '\n';


	dbg << F("\nswitch over to the library communication modul...\n");
	com->init();


}

void loop() {
	if (com->has_data()) {
		com->rcv_data(rcv_str);
		//dbg << "data: " << _HEX(rcv_str, rcv_str[0]+1);
	}

}



/* - some helpers for the spi communication -------------------------------------------------------------------------------
*/
uint8_t readReg(uint8_t regAddr, uint8_t regType) {										// read CC1101 register via SPI
	spi_select();																		// select CC1101
	spi_send_byte(regAddr | regType);													// send register address
	uint8_t val = spi_send_byte(0x00);													// read result
	spi_deselect();																		// deselect CC1101
	return val;
}

void spi_select(void) {
	set_pin_low(p_csl);																	// chip select is low active 
	for (uint8_t i = 0; i < 200; i++) {													// wait till miso pin goes low
		if (!get_pin_status(p_miso)) return;											// miso is low now, good
	}

	dbg << F("failure, miso is not going low...\n");									// not good if we are here, means there is a failure in pin definition or cabeling
}

void spi_deselect(void) {
	set_pin_high(p_csl);																// chip select is low active, there for we switch to high
}

void strobe(uint8_t cmd) {																// send command strobe to the CC1101 IC via SPI
	spi_select();																		// select CC1101
	spi_send_byte(cmd);																	// send strobe command
	spi_deselect();																		// deselect CC1101
}
