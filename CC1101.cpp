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
#ifdef CC_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif

#include "CC1101.h"

/*
* @brief Initialize the cc1101 rf modul
* 
* First of all we initialize the interface to the rf module, SPI and some additional lines
* are defined in hardware.h. You will find the setup function in the library folder in file HAL_extern.h.
* In this setup we need no interrupt anymore, we check the GDO2 pin by a poll function, if there are data
* received.
* Main intent of this function is to write the default values into the communication register on cc1101.
*
*/
void    CC::init(void) {																// initialize CC1101
	DBG( F("CC.\n") );																	// debug message
	
	ccInitHw();																			// init the hardware to get access to the RF modul

	ccDeselect();																		// some deselect and selects to init the TRX868modul
	_delay_us(5);
	ccSelect();
	_delay_us(10);
	ccDeselect();
	_delay_us(41);

	strobe(CC1101_SRES);																// send reset
	_delay_ms(10);
	pwr_down = 0;

	DBG( '1');

	// define init settings for TRX868
	static const uint8_t initVal[] PROGMEM = {
		CC1101_IOCFG2,    0x2E,	// 					// non inverted GDO2, high impedance tri state
//		CC1101_IOCFG1,    0x2E,	// (default)		// low output drive strength, non inverted GD=1, high impedance tri state
		CC1101_IOCFG0,    0x06,	// packet CRC ok	// disable temperature sensor, non inverted GDO0,
		CC1101_FIFOTHR,   0x0D,						// 0 ADC retention, 0 close in RX, TX FIFO = 9 / RX FIFO = 56 byte
		CC1101_SYNC1,     0xE9,						// Sync word
		CC1101_SYNC0,     0xCA,
		CC1101_PKTLEN,    0x3D,						// packet length 61
		CC1101_PKTCTRL1,  0x0C,						// PQT = 0, CRC auto flush = 1, append status = 1, no address check
		CC1101_FSCTRL1,   0x06,						// frequency synthesizer control

		// 868.299866 MHz
		//CC1101_FREQ2,     0x21,
		//CC1101_FREQ1,     0x65,
		//CC1101_FREQ0,     0x6A,

		// 868.2895508
		CC1101_FREQ2,     0x21,
		CC1101_FREQ1,     0x65,
		CC1101_FREQ0,     0x50,

		CC1101_MDMCFG4,  0xC8,
		CC1101_MDMCFG3,  0x93,
		CC1101_MDMCFG2,  0x03,
		CC1101_DEVIATN,  0x34,						// 19.042969 kHz
		CC1101_MCSM2,    0x01,
		CC1101_MCSM1,    0x33,						// always go into IDLE
		CC1101_MCSM0,    0x18,
		CC1101_FOCCFG,   0x16,
		CC1101_AGCCTRL2, 0x43,
		//CC1101_WOREVT1, 0x28,						// tEVENT0 = 50 ms, RX timeout = 390 us
		//7CC1101_WOREVT0, 0xA0,
		//CC1101_WORCTRL, 0xFB,						//EVENT1 = 3, WOR_RES = 0
		CC1101_FREND1,  0x56,
		CC1101_FSCAL1,  0x00,
		CC1101_FSCAL0,  0x11,
		CC1101_TEST1,   0x35,
		CC1101_PATABLE, 0xC3,
	};
	for (uint8_t i=0; i < sizeof(initVal); i+=2) {										// write init value to TRX868
		writeReg(_PGM_BYTE(initVal[i]), _PGM_BYTE(initVal[i+1]));
		//dbg << i << ": " << _HEXB(_PGM_BYTE(initVal[i])) << ' ' << _HEXB(_PGM_BYTE(initVal[i+1])) << '\n';
	}

	DBG( '2' );

	strobe(CC1101_SCAL);																// calibrate frequency synthesizer and turn it off
	while (readReg(CC1101_MARCSTATE, CC1101_STATUS) != 1) {								// waits until module gets ready
		_delay_us(1);
		DBG( '.' );
	}

	DBG( '3' );

	writeReg(CC1101_PATABLE, PA_MaxPower);												// configure PATABLE
	strobe(CC1101_SRX);																	// flush the RX buffer
	strobe(CC1101_SWORRST);																// reset real time clock

	DBG( F(" - ready\n") );
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
void    CC::sndData(uint8_t *buf, uint8_t burst) {										// send data packet via RF

	// Going from RX to TX does not work if there was a reception less than 0.5
	// sec ago. Due to CCA? Using IDLE helps to shorten this period(?)
	setActive();																		// maybe we come from power down mode
	strobe(CC1101_SIDLE);																// go to idle mode
	strobe(CC1101_SFRX );																// flush RX buffer
	strobe(CC1101_SFTX );																// flush TX buffer

	//dbg << "tx\n";

	if (burst) {																		// BURST-bit set?
		strobe(CC1101_STX  );															// send a burst
		_delay_ms(360);																	// according to ELV, devices get activated every 300ms, so send burst for 360ms
		//dbg << "send burst\n";
	} else {
		_delay_ms(1);																	// wait a short time to set TX mode
	}

	// former writeburst function, now done here while hm encoding included
	ccSelect();																			// select CC1101
	ccSendByte(CC1101_TXFIFO | WRITE_BURST);											// send register address
	ccSendByte(buf[0]);																	// send byte 0

	// we write the given string encoded in the buffer of the communication module
	uint8_t prev = (~buf[1]) ^ 0x89;													// prepare byte 1
	ccSendByte(prev);																	// send byte 1

	uint8_t buf2 = buf[2];																// remember byte 2, we need it on the end of string

	for (uint8_t i = 2; i < buf[0]; i++) {												// process the string starting with byte 2
		prev = (prev + 0xDC) ^ buf[i];													// encode current (i) byte
		ccSendByte(prev);																// write it into the module buffer
	}
	ccSendByte( buf[buf[0] +1] ^ buf2);													// process the last byte
	ccDeselect();																		// deselect CC1101

	// write is done, cleanup buffer and wait for finishing the transmition
	strobe(CC1101_SFRX);																// flush the RX buffer
	strobe(CC1101_STX);																	// send a burst

	for (uint8_t i = 0; i < 200; i++) {													// wait for end of calibration (about 600us) and entering state TX
		if (readReg(CC1101_MARCSTATE, CC1101_STATUS) == MARCSTATE_TX)
			break;
		_delay_us(10);
	}

	// now transmitting data
	for (uint16_t i = 0; i < 2000; i++) {												// after sending out all bytes the chip should go automatically in RX mode
		if (readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_TX)
			break;																		// now in RX mode, good
		_delay_us(10);
	}

	DBG( F("<c "), _HEX(buf, buf[0] + 1), '\n' );
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
	//u_rvStatus rvByte;																// size the receive status byte

	rxByte.VAL = readReg(CC1101_RXBYTES, CC1101_STATUS);								// how many bytes are in the buffer
	//dbg << rxByte.VAL << ' ';

	if ((rxByte.FLAGS.WAITING) && !(rxByte.FLAGS.OVERFLOW)) {							// any byte waiting to be read and no overflow?
		buf[0] = readReg(CC1101_RXFIFO, CC1101_CONFIG);									// read data length
		
		if (buf[0] > CC1101_DATA_LEN) {													// if packet is too long
			buf[0] = 0;																	// discard packet
			
		} else {
			readBurst( buf + 1, CC1101_RXFIFO, buf[0]);									// read data packet
			decode(buf);																// decode the buffer

			rssi = readReg(CC1101_RXFIFO, CC1101_CONFIG);								// read RSSI
			if (rssi >= 128) rssi = 255 - rssi;											// and normalize the value
			rssi /= 2; rssi += 72;
			
			// as we are not checking LQI and CRC
			//rvByte.VAL = readReg(CC1101_RXFIFO, CC1101_CONFIG);							// read LQI and CRC_OK
		}

	} else buf[0] = 0;																	// nothing to do, or overflow

	strobe(CC1101_SFRX);																// flush Rx FIFO
	strobe(CC1101_SIDLE);																// enter IDLE state
	strobe(CC1101_SRX);																	// back to RX state
	strobe(CC1101_SWORRST);																// reset real time clock
	//	trx868.rfState = RFSTATE_RX;													// declare to be in Rx state

	DBG( F("c> "), _HEX(buf, buf[0] + 1), '\n' );
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
* @brief Reads data from cc1101 registers by selecting the rf module,
*        addressing the respective register and reading the answer.
*
* @param *buf     Pointer to a byte array for the answer
* @param regAddr  Byte variable as input for the regiser to read
* @param len      Byte variable to indicate how many bytes to read
*
*/
void   CC::readBurst(uint8_t *buf, uint8_t regAddr, uint8_t len) {						// read burst data from CC1101 via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr | READ_BURST);													// send register address
	for(uint8_t i=0 ; i<len ; i++) {
		buf[i] = ccSendByte(0x00);														// read result byte by byte
		//dbg << i << ":" << buf[i] << '\n';
	}
	ccDeselect();																		// deselect CC1101
}
/**
* @brief Write multiple registers into the cc1101 via SPI interface
*
* @param regAddr Register address byte
* @param *buf Pointer to a byte array with data to write
* @param len  Byte variable with the amount of bytes to write from *buf
*
*/
void   CC::writeBurst(uint8_t *buf, uint8_t regAddr, uint8_t len) {
	ccSelect();																			// select CC1101
																						//waitMiso();																		// wait until MISO goes low
	ccSendByte(regAddr | WRITE_BURST);													// send register address
	for (uint8_t i = 0; i<len; i++) ccSendByte(buf[i]);									// send value
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

