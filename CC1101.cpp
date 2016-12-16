/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin cc1101 functions -----------------------------------------------------------------------------------------------
* - with a lot of copy and paste from culfw firmware
* - standby fix and some improvement from LineF@github.com
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "CC1101.h"
#include "HAL.h"


//public:   //------------------------------------------------------------------------------------------------------------
/*
* @brief Initialize the cc1101 rf modul
* 
* First of all we initialize the interface to the rf module, SPI and some additional lines
* are defined in hardware.h. You will find the setup function in the library folder in file HAL_extern.h.
* In this setup we need no interrupt anymore, we check the GDO2 pin by a poll function, if there are data
* received.
* Main intent of this function is to write the default values into the communication register on cc1101.
*
* This modul needs some functions defined external in a hardware abstraction layer:
* ccInitHW() - initialize the respective port and pins. For the cc1101 we need a proper initialized SPI port and a chip select
* ccSelect(), ccDeselect() - driving the chip select pin accordingly
* ccSendByte() - forwarder to the SPI send and receive function
*
*/
void    CC::init(void) {	
	//DBG_START(CC, F("CC.\n"));

	/* init the hardware to get access to the RF modul,
	*  some deselect and selects to init the TRX868modul */
	ccInitHw();																			

	ccDeselect();																		
	_delay_us(5);
	ccSelect();
	_delay_us(10);
	ccDeselect();
	_delay_us(41);

	/* send a reset and wait some time for restart of the the device */
	strobe(CC1101_SRES);
	_delay_ms(10);
	pwr_down = 0;

	/* check the hw and version flag
	*  PARTNUM | VERSION | Radio
	* -------- | ------- | ------
	*        0 |       3 | CC1100
	*        0 |       4 | CC1101
	*        0 |       5 | CC1100E
	*        0 |       6 | CC430 RF1A
	*        0 |       7 | CC110L
	*      128 |       3 | CC2500
	*/
	uint8_t part_num = readReg(CC1101_PARTNUM, CC1101_STATUS);
	uint8_t part_ver = readReg(CC1101_VERSION, CC1101_STATUS);
	DBG(CC, F("Part Nummer: "), part_num, F(", Version: "), part_ver, '\n');

	DBG(CC, '1');

	/* define init settings for TRX868 */
	static const uint8_t initVal[] PROGMEM = {
		CC1101_IOCFG2,    0x2E,						// non inverted GDO2, high impedance tri state
		CC1101_IOCFG0,    0x06,						// disable temperature sensor, non inverted GDO0,
		CC1101_FIFOTHR,   0x0D,						// 0 ADC retention, 0 close in RX, TX FIFO = 9 / RX FIFO = 56 byte
		CC1101_SYNC1,     0xE9,						// Sync word
		CC1101_SYNC0,     0xCA,
		CC1101_PKTLEN,    0x3D,						// packet length 61
		CC1101_PKTCTRL1,  0x0C,						// PQT = 0, CRC auto flush = 1, append status = 1, no address check
		CC1101_PKTCTRL0,  0x45,						// WHITE_DATA on, use FIFOs for RX and TX, CRC calculation in TX and CRC check in RX enabled, Packet length configured by the first byte after sync word
		CC1101_FSCTRL1,   0x06,	//0x06					// frequency synthesizer control
		//CC1101_FREQ2,   0x21,						// 868.299866 MHz
		//CC1101_FREQ1,   0x65,
		//CC1101_FREQ0,   0x6A,
		CC1101_FREQ2,     0x21,						// 868.289551 MHz
		CC1101_FREQ1,     0x65,
		CC1101_FREQ0,     0x6A, //50
		CC1101_MDMCFG4,   0xC8,
		CC1101_MDMCFG3,   0x93,
		CC1101_MDMCFG2,   0x03,						// digital DC blocking filter enabled (better sensitivity), 2-FSK, Manchester encoding/decoding disabled, SYNC_MODE[2:0] 30/32 sync word bits detected
		CC1101_MDMCFG1,   0x22,						// Forward Error Correction (FEC) disabled, NUM_PREAMBLE[2:0] 4 bytes, 
		CC1101_DEVIATN,   0x34,						// 19.042969 kHz
		CC1101_MCSM2,     0x01,						// RX_TIME[2:0]
		CC1101_MCSM1,     0x33, // EQ3 0x03, 0x3f	// TXOFF_MODE[1:0] 3 (11) : RX, RXOFF_MODE[1:0] 0 (00) : IDLE, CCA_MODE[1:0] 3 (11) : If RSSI below threshold unless currently receiving a packet
		CC1101_MCSM0,     0x18,						// FS_AUTOCAL[1:0] 1 (01) : When going from IDLE to RX or TX (or FSTXON), 
		CC1101_FOCCFG,    0x16,						// frequency compensation loop gain to be used before a sync word is detected 2 (10) : 3K, gain to be used after a sync word is detected 1 : K/2, Frequency offset compensation 3 (11) : ±BWCHAN/2
		CC1101_AGCCTRL2,  0x43,						// MAX_DVGA_GAIN[1:0] 1 (01) : The highest gain setting can not be used, MAGN_TARGET[2:0] 3 (011) : 33 dB
		CC1101_FREND1,    0x56,
		CC1101_FSCAL1,    0x00,
		CC1101_FSCAL0,    0x11,
		CC1101_FSTEST,    0x59,
		CC1101_TEST2,     0x81,
		CC1101_TEST1,     0x35,
		CC1101_PATABLE,   0xC3,
	};
	for (uint8_t i = 0; i < sizeof(initVal); i += 2) {									// step through the array
		writeReg(_PGM_BYTE(initVal[i]), _PGM_BYTE(initVal[i + 1]));						// and write init value to TRX868
	} DBG(CC, '2');																		// phase two is done, config is written


	/* calibrate frequency synthesizer */
	strobe(CC1101_SCAL);																// calibrate frequency synthesizer
	uint8_t x = 200;																	// set the counter
	while (readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_IDLE) {				// waits until module gets ready
		_delay_us(2);																	// wait some time while looping
		if (!--x) goto init_failure;													// otherwise we could loop forever on a missing module
	} DBG(CC, '3');																		// phase 3 is done, freq synth is calibrated

	/* set the transmition power and enter receive mode */
	writeReg(CC1101_PATABLE, PA_MaxPower);												// configure PATABLE
	strobe(CC1101_SRX);																	// flush the RX buffer
	strobe(CC1101_SWORRST);																// reset real time clock
	x = 200;																			// set the counter for timeout
	while (readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_RX) {					// waits until module gets ready
		_delay_us(2);																	// wait some time while looping
		if (!--x) goto init_failure;													// otherwise we could loop forever on a missing module
	} DBG(CC, '4');																		// we are in receive mode

	/* show that we are ready */
	DBG(CC, F(" - ready\n"));															// some debug
	return;																				// everything done, return

init_failure:																			// catch problems
	DBG(CC, F(" - something went wrong...\n"));											// and visualize
}

/**
* @brief Function to send data via the cc1101 rf module
* 
* @param *buf  Pointer to a byte array with the information to send
* @param burst Flag if a burst signal is needed to wakeup the other device
*
* *buf needs a specific format to detect the amount of bytes which have to be send.
* Length identification is done by byte[0] which holds the needed info.
*
*/
void    CC::sndData(uint8_t *buf, uint8_t burst) {
	/* Going from RX to TX does not work if there was a reception less than 0.5
	* sec ago. Due to CCA? Using IDLE helps to shorten this period(?)             */
	uint8_t x, prev, buf2;																// size some variables

	setActive();																		// maybe we come from power down mode
	strobe(CC1101_SIDLE);																// go to idle mode
	DBG(CC, F("<c"), _TIME, ' ');

	/* enter TX and wait till state is confirmed */
	strobe(CC1101_STX);																	// ask for transmit mode
	x = 200;																			// set the counter for timeout
	do {																				// waits until module gets ready
		_delay_us(2);																	// wait some time while looping
		if (!--x) goto snddata_failure;													// otherwise we could loop forever on a missing module
	} while (readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_TX);
	DBG(CC, F("TX"), _TIME, ' ');														// we are in transmit mode

	/* if we have to send a burst, we wait still longer */
	if (burst) {																		// BURST-bit set?
		_delay_ms(360);																	// according to ELV, devices get activated every 300ms, so send burst for 360ms
		DBG(CC, F("BURST"), _TIME, ' ');												// some debug
	}
	//DBG(CC, F("O:"), _HEX(buf, buf[0]+1), ' ');											// some debug


	/* former writeburst function, now done with ccSendByte while writing byte
	*  by byte to encode it on the fly                                          */
	prev = (~buf[1]) ^ 0x89;															// encode byte 1 of the given string
	buf2 = buf[2];																		// remember byte 2, we need it on the end of string

	ccSelect();																			// select CC1101
	ccSendByte(CC1101_TXFIFO | WRITE_BURST);											// send register address
	ccSendByte(buf[0]); DBG(CC, F("E:"), _HEXB(buf[0]), ' ');							// send byte 0, holds the length information
	ccSendByte(prev);	DBG(CC, _HEXB(prev), ' ');										// send byte 1

	for (uint8_t i = 2; i < buf[0]; i++) {												// process the string starting with byte 2
		prev = (prev + 0xDC) ^ buf[i];													// encode current (i) byte
		ccSendByte(prev); DBG(CC, _HEXB(prev), ' ');									// write it into the module buffer
	}
	prev = buf[buf[0]] ^ buf2;
	ccSendByte(prev);	DBG(CC, _HEXB(prev), ' ');										// process the last byte
	ccDeselect();																		// deselect CC1101
	DBG(CC, F("#:"), buf[0]+1, _TIME, ' ');												// bytes are written in the send buffer, some debug

	/* now wait till TX queue is finished and module enters RX mode automatically
	*  while defined in CC1101_MCSM1, 0x3F - can take up to 50ms         */
	x = 200;																			// set the counter for timeout
	while (readReg(CC1101_MARCSTATE, CC1101_STATUS) == MARCSTATE_TX) {					// waits until module gets ready
		_delay_ms(1);																	// wait some time while looping
		if (!--x) goto snddata_failure;													// otherwise we could loop forever on a missing module
	} DBG(CC, F("TX"), _TIME, F(" \n"));												// we are back in rx mode

	return;																				// nothing to do any more, return

snddata_failure:
	DBG(CC, F("something went wrong...\n")); 
}

/**
* @brief Receive function for the cc1101 rf module
*
* @param *buf A pointer to a byte array to store the received bytes
* @return Nothing, len of the received bytes is the first byte in the receive buffer
* 
* Additionally the cc module holds a further information regarding signal quality,
* which can be checked within the cc.rssi byte variable.
* Received strings are automatically checked for their crc consistence.
*/
void    CC::rcvData(uint8_t *buf) {														// read data packet from RX FIFO
	u_rxStatus rxByte;																	// size the rx status byte
	u_rvStatus rvByte;

	/* we are here while GDO0 has indicated that something was received,
	*  read the status register, if there is something in the buffer, get it...  */
	rxByte.VAL = readReg(CC1101_RXBYTES, CC1101_STATUS);								// ask for the status of the RX queue
	DBG(CC, F("c> "), rxByte.FLAGS.WAITING, ' ');

	if ((rxByte.FLAGS.WAITING) && (!rxByte.FLAGS.OVERFLOW)) {							// any byte waiting to be read and no overflow?
		buf[0] = readReg(CC1101_RXFIFO, CC1101_CONFIG);									// read data length

		if (buf[0] <= CC1101_DATA_LEN) {												// only if it fits in the buffer
	
			ccSelect();																	// select the module
			ccSendByte(READ_BURST | CC1101_RXFIFO);										// switch into burst mode
			for (uint8_t i = 1; i <= buf[0]; i++) {										// loop through the bytes
				buf[i] = ccSendByte(0);													// get byte by byte
			}

			rssi = ccSendByte(0);														// get the rssi status
			rvByte.VAL = ccSendByte(0);													// lqi not used
			if (rvByte.FLAGS.CRC) DBG(CC, "CRC "); 
			ccDeselect();																// and deselect

			DBG(CC, buf[0], ' ');														// visualize the amount of received bytes
			decode(buf);																// decode the buffer
			
			if (rssi >= 128) rssi = 255 - rssi;											// normalize the rssi value
			rssi /= 2; rssi += 72;

		} else DBG(CC, F("error "), buf[0], ' ');										// visualize buffer too small
		if (!rvByte.FLAGS.CRC) buf[0] = 0;												// crc was failed, therefor buf[0] cleaned up
	}

	/* check if there was a failure and there are still bytes in the buffer, empty receive queue 
	*  seems there is a bug in the cc1101 chip, bug described in the errata note, 
	*  solution is - go idle, flush the buffer and back to rx mode all time */
	strobe(CC1101_SIDLE);																// idle needed to flush the buffer
	strobe(CC1101_SFRX);																// flush the receive buffer
	strobe(CC1101_SRX);																	// and back to receive mode
	DBG(CC, F(">> "), _HEX(buf, buf[0] + 1), _TIME, '\n' );
}

/**
* @brief Function to power down the cc1101 module in a sleep mode.
* Activation is done by calling detectBurst() or sndData(). If that
* functions are finished it is necassary to call setIdle() again to
* power down the cc1101 chip.
* 
*/
void    CC::setIdle() {																	// put CC1101 into power-down state
	if (pwr_down)																		// do nothing if already powered down
		return;
	strobe(CC1101_SIDLE);																// coming from RX state, we need to enter the IDLE state first
	strobe(CC1101_SFRX);
	strobe(CC1101_SPWD);																// enter power down state
	pwr_down = 1;																		// remember power down state
	//dbg << "pd\n";
}

/**
* @brief Detect a burst signal in the air.
*
* @ returns a byte value with 0 for nothing detected or a 1 for burst detected
*
* A burst signal for 300ms is send to wakeup battery powered homematic devices.
* The client is checking every 250ms if there is a burst signal for at least 50ms received.
* If so, the device stays awake to receive a string and to double check the address.
*
*/
uint8_t CC::detectBurst(void) {
	// 10 7/10 5 in front of the received string; 33 after received string
	// 10 - 00001010 - sync word found
	// 7  - 00000111 - GDO0 = 1, GDO2 = 1
	// 5  - 00000101 - GDO0 = 1, GDO2 = 1
	// 33 - 00100001 - GDO0 = 1, preamble quality reached
	// 96 - 01100000 - burst sent
	// 48 - 00110000 - in receive mode
	//
	// Status byte table:
	//	0 current GDO0 value
	//	1 reserved
	//	2 GDO2
	//	3 sync word found
	//	4 channel is clear
	//	5 preamble quality reached
	//	6 carrier sense
	//	7 CRC ok
	//
	// possible solution for finding a burst is to check for bit 6, carrier sense

	// power on cc1101 module and set to RX mode
	setActive();
	strobe(CC1101_SRX);																	// set RX mode again
	for (uint8_t i = 0; i < 200; i++) {													// wait for reaching RX state
		if (readReg(CC1101_MARCSTATE, CC1101_STATUS) == MARCSTATE_RX)
			break;																		// now in RX mode, good
		_delay_us(10);
	}

	u_ccStatus ccStat;
	for (uint8_t i = 0; i < 200; i++) {													// check if we are in RX mode
		ccStat.VAL = readReg(CC1101_PKTSTATUS, CC1101_STATUS);							// read the status of the line
		if ( (ccStat.FLAGS.CLEAR) || (ccStat.FLAGS.CARRIER) ) break;					// check for channel clear, or carrier sense
		_delay_us(10);																	// wait a bit
	}
	return ccStat.FLAGS.CARRIER;														// return carrier sense bit
}

//private:  //------------------------------------------------------------------------------------------------------------
/**
* @brief Wakeup the cc1101 module
*
* setActive is an internal function of the cc class to wakeup the communication 
* hardware and ensure it is back to work again.
*
*/
void   CC::setActive() {																// put CC1101 into active state
	if (!pwr_down)																		// do nothing if already active
		return;
	ccSelect();																			// wake up the communication module
	ccDeselect();

	for (uint8_t i = 0; i < 200; i++) {													// instead of delay, check the really needed time to wakeup
		if (readReg(CC1101_MARCSTATE, CC1101_STATUS) != 0xff) break;
		_delay_us(10);
	}
	pwr_down = 0;																		// remember active state
	//dbg << "act\n";
}

/**
* @brief Simple function to write a byte per strobe over the SPI bus
*
*/
void   CC::strobe(uint8_t cmd) {														// send command strobe to the CC1101 IC via SPI
	ccSelect();																			// select CC1101
	ccSendByte(cmd);																	// send strobe command
	ccDeselect();																		// deselect CC1101
}

/**
* @brief Read a byte from a specific register
*
* @param regAddr Byte value with the register address
* @param regType Byte value with the register type
*
* @returns the byte which were read
*/
uint8_t CC::readReg(uint8_t regAddr, uint8_t regType) {									// read CC1101 register via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr | regType);														// send register address
	uint8_t val = ccSendByte(0x00);														// read result
	ccDeselect();																		// deselect CC1101
	return val;
}

/**
* @brief Writes a byte to a single register
*
* @regAddr Byte value with the register address
* @val Byte value with the content which should be written
*
*/
void    CC::writeReg(uint8_t regAddr, uint8_t val) {									// write single register into the CC1101 IC via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr);																// send register address
	ccSendByte(val);																	// send value
	ccDeselect();																		// deselect CC1101
}

/**
* @brief Decode the incoming messages
*        Note: this is no encryption!
*
* @param buf   pointer to buffer
*/
void    CC::decode(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i = 2; i < buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xDC) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}

/**
* @brief Encode the outgoing messages
*        Note: this is no encryption!
*
* @param buf   pointer to buffer
*/
void CC::encode(uint8_t *buf) {
	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i = 2; i < buf[0]; i++) {
		prev = (prev + 0xDC) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}

