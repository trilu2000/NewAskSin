//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin cc1101 functions -----------------------------------------------------------------------------------------------
//- with a lot of copy and paste from culfw firmware
//- -----------------------------------------------------------------------------------------------------------------------

//#define CC_DBG
#include "CC1101.h"

// private:		//---------------------------------------------------------------------------------------------------------
CC::CC() {
}

void    CC::init(void) {																// initialize CC1101
	#ifdef CC_DBG																		// only if cc debug is set
	dbg.begin(57600);																	// dbg setup
	dbg << F("\n....\n");																// ...and some information
	dbg << F("CC.\n");																	// ...and some information
	#endif
	
	ccInitHw();																			// init the hardware to get access to the RF modul

	ccDeselect();																		// some deselect and selects to init the TRX868modul
	_delay_us(5);
	ccSelect();
	_delay_us(10);
	ccDeselect();
	_delay_us(41);

	strobe(CC1101_SRES);																// send reset
	_delay_ms(10);
	pwr_down = 0;																		// remember active state

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << '1';
	#endif

	// define init settings for TRX868
	static const uint8_t initVal[] PROGMEM = {
		CC1101_IOCFG2,    0x2E,	// 												// non inverted GDO2, high impedance tri state
//		CC1101_IOCFG1,    0x2E,	// (default)									// low output drive strength, non inverted GD=1, high impedance tri state
		CC1101_IOCFG0,    0x06,	// packet CRC ok								// disable temperature sensor, non inverted GDO0,
		CC1101_FIFOTHR,   0x0D,													// 0 ADC retention, 0 close in RX, TX FIFO = 9 / RX FIFO = 56 byte
		CC1101_SYNC1,     0xE9,													// Sync word
		CC1101_SYNC0,     0xCA,
		CC1101_PKTLEN,    0x3D,													// packet length 61
		CC1101_PKTCTRL1,  0x0C,													// PQT = 0, CRC auto flush = 1, append status = 1, no address check
		CC1101_FSCTRL1,   0x06,													// frequency synthesizer control

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
		CC1101_DEVIATN,  0x34,													// 19.042969 kHz
		CC1101_MCSM2,    0x01,
		CC1101_MCSM1,    0x03,													// always go into RX after TX, no CCA
		CC1101_MCSM0,    0x18,
		CC1101_FOCCFG,   0x16,
		CC1101_AGCCTRL2, 0x43,
		//CC1101_WOREVT1, 0x28,													// tEVENT0 = 50 ms, RX timeout = 390 us
		//7CC1101_WOREVT0, 0xA0,
		//CC1101_WORCTRL, 0xFB,													//EVENT1 = 3, WOR_RES = 0
		CC1101_FREND1,  0x56,
		CC1101_FSCAL1,  0x00,
		CC1101_FSCAL0,  0x11,
		CC1101_TEST1,   0x35,
		CC1101_PATABLE, 0xC3,
	};
	for (uint8_t i=0; i<sizeof(initVal); i+=2) {										// write init value to TRX868
		writeReg(_PGM_BYTE(initVal[i]), _PGM_BYTE(initVal[i+1]));
		//dbg << i << ": " << _HEXB(_PGM_BYTE(initVal[i])) << ' ' << _HEXB(_PGM_BYTE(initVal[i+1])) << '\n';
	}

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << '2';
	#endif

	strobe(CC1101_SCAL);																// calibrate frequency synthesizer and turn it off
	while (readReg(CC1101_MARCSTATE, CC1101_STATUS) != 1) {								// waits until module gets ready
		_delay_us(1);
		#ifdef CC_DBG																	// only if cc debug is set
		dbg << '.';
		#endif
	}

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << '3';
	#endif

	writeReg(CC1101_PATABLE, PA_MaxPower);												// configure PATABLE
	strobe(CC1101_SRX);																	// flush the RX buffer
	strobe(CC1101_SWORRST);																// reset real time clock

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << F(" - ready\n");
	#endif
}
uint8_t CC::sndData(uint8_t *buf, uint8_t burst) {										// send data packet via RF

	// Going from RX to TX does not work if there was a reception less than 0.5
	// sec ago. Due to CCA? Using IDLE helps to shorten this period(?)
	setActive();																		// maybe we come from power down mode
	strobe(CC1101_SIDLE);																// go to idle mode
	strobe(CC1101_SFRX );																// flush RX buffer
	strobe(CC1101_SFTX );																// flush TX buffer

	//dbg << "tx\n";

	if (burst) {																		// BURST-bit set?
		strobe(CC1101_STX);																// send a burst
		_delay_ms(360);																	// according to ELV, devices get activated every 300ms, so send burst for 360ms
		//dbg << "send burst\n";
	} else {
		_delay_ms(1);																	// wait a short time to set TX mode
	}

	writeBurst(CC1101_TXFIFO, buf, buf[0]+1);											// write in TX FIFO

	strobe(CC1101_SFRX);																// flush the RX buffer
	strobe(CC1101_STX);																	// send a burst

	for(uint8_t i = 0; i < 200; i++) {													// wait for end of calibration (about 600us) and entering state TX
		if( readReg(CC1101_MARCSTATE, CC1101_STATUS) == MARCSTATE_TX)
		break;
		_delay_us(10);
	}
	// now transmitting data
	for(uint16_t i = 0; i < 2000; i++) {												// after sending out all bytes the chip should go automatically in RX mode
		if( readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_TX)
			break;																		// now in RX mode, good
		_delay_us(10);
	}

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << F("<- ") << _HEXB(buf[0]) << _HEXB(buf[1]) << '\n';//pTime();
	#endif

	//dbg << "rx\n";
	return true;
}
uint8_t CC::rcvData(uint8_t *buf) {														// read data packet from RX FIFO
	uint8_t rxBytes = readReg(CC1101_RXBYTES, CC1101_STATUS);							// how many bytes are in the buffer
	//dbg << rxBytes << ' ';

	if ((rxBytes & 0x7F) && !(rxBytes & 0x80)) {										// any byte waiting to be read and no overflow?
		buf[0] = readReg(CC1101_RXFIFO, CC1101_CONFIG);									// read data length
		
		if (buf[0] > CC1101_DATA_LEN) {													// if packet is too long
			buf[0] = 0;																	// discard packet
			
		} else {
			readBurst(&buf[1], CC1101_RXFIFO, buf[0]);									// read data packet
			
			rssi = readReg(CC1101_RXFIFO, CC1101_CONFIG);								// read RSSI
			
			if (rssi >= 128) rssi = 255 - rssi;
			rssi /= 2; rssi += 72;
			
			uint8_t val = readReg(CC1101_RXFIFO, CC1101_CONFIG);						// read LQI and CRC_OK
			lqi = val & 0x7F;
			crc_ok = bitRead(val, 7);
	
		}

	} else buf[0] = 0;																	// nothing to do, or overflow

	strobe(CC1101_SFRX);																// flush Rx FIFO
	strobe(CC1101_SIDLE);																// enter IDLE state
	strobe(CC1101_SRX);																	// back to RX state
	strobe(CC1101_SWORRST);																// reset real time clock
	//	trx868.rfState = RFSTATE_RX;													// declare to be in Rx state

	#ifdef CC_DBG																		// only if cc debug is set
	if (buf[0] > 0) dbg << _HEX(buf, buf[0]+1) << '\n';//pTime();
	#endif

	return buf[0];																		// return the data buffer
}
void    CC::setIdle() {																	// put CC1101 into power-down state
	if (pwr_down)																		// do nothing if already powered down
		return;
	strobe(CC1101_SIDLE);																// coming from RX state, we need to enter the IDLE state first
	strobe(CC1101_SFRX);
	strobe(CC1101_SPWD);																// enter power down state
	pwr_down = 1;																		// remember power down state
	//dbg << "pd\n";
}
void    CC::setActive() {																// put CC1101 into active state
	if (!pwr_down)																		// do nothing if already active
		return;
	ccSelect();																			// wake up the communication module
	ccDeselect();

	for(uint8_t i = 0; i < 200; i++) {													// instead of delay, check the really needed time to wakeup
		if (readReg(CC1101_MARCSTATE, CC1101_STATUS) != 0xff) break;
		_delay_us(10);
	}
	pwr_down = 0;																		// remember active state
	//dbg << "act\n";
}
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

	setActive();
	strobe(CC1101_SRX);																	// set RX mode again

	uint8_t bTmp;
	for (uint8_t i = 0; i < 200; i++) {													// check if we are in RX mode
		bTmp = readReg(CC1101_PKTSTATUS, CC1101_STATUS);								// read the status of the line
		if ((bTmp & 0x10) || (bTmp & 0x40)) break;										// check for channel clear, or carrier sense
		_delay_us(10);																	// wait a bit
	}
	return (bTmp & 0x40)?1:0;															// return carrier sense bit
}

void    CC::strobe(uint8_t cmd) {														// send command strobe to the CC1101 IC via SPI
	ccSelect();																			// select CC1101
	ccSendByte(cmd);																	// send strobe command
	ccDeselect();																		// deselect CC1101
}
inline void    CC::readBurst(uint8_t *buf, uint8_t regAddr, uint8_t len) {						// read burst data from CC1101 via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr | READ_BURST);													// send register address
	for(uint8_t i=0 ; i<len ; i++) {
		buf[i] = ccSendByte(0x00);														// read result byte by byte
		//dbg << i << ":" << buf[i] << '\n';
	}
	ccDeselect();																		// deselect CC1101
}
inline void    CC::writeBurst(uint8_t regAddr, uint8_t *buf, uint8_t len) {					// write multiple registers into the CC1101 IC via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr | WRITE_BURST);													// send register address
	for(uint8_t i=0 ; i<len ; i++) ccSendByte(buf[i]);									// send value
	ccDeselect();																		// deselect CC1101
}
uint8_t CC::readReg(uint8_t regAddr, uint8_t regType) {									// read CC1101 register via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr | regType);														// send register address
	uint8_t val = ccSendByte(0x00);														// read result
	ccDeselect();																		// deselect CC1101
	return val;
}
void    CC::writeReg(uint8_t regAddr, uint8_t val) {									// write single register into the CC1101 IC via SPI
	ccSelect();																			// select CC1101
	ccSendByte(regAddr);																// send register address
	ccSendByte(val);																	// send value
	ccDeselect();																		// deselect CC1101
}
