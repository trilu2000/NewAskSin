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

	_ccDeselect;																		// some deselect and selects to init the TRX868modul
	_delay_us(5);
	_ccSelect;
	_delay_us(10);
	_ccDeselect;
	_delay_us(41);

	strobe(CC1101_SRES);																// send reset
	_delay_ms(10);

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << '1';
	#endif

	static const uint8_t initVal[] PROGMEM = {											// define init settings for TRX868
		0x00, 0x2E,			// IOCFG2: tristate											// non inverted GDO2, high impedance tri state
		0x01, 0x2E,			// IOCFG1: tristate											// low output drive strength, non inverted GD=1, high impedance tri state
		0x02, 0x06,			// IOCFG0: packet CRC ok									// disable temperature sensor, non inverted GDO0, asserts when a sync word has been sent/received, and de-asserts at the end of the packet. in RX, the pin will also de-assert when a package is discarded due to address or maximum length filtering
		0x03, 0x0D,			// FIFOTHR: TX:9 / RX:56									// 0 ADC retention, 0 close in RX, TX FIFO = 9 / RX FIFO = 56 byte
		0x04, 0xE9,			// SYNC1													// Sync word
		0x05, 0xCA,			// SYNC0
		0x06, 0x3D,			// PKTLEN(x): 61											// packet length 61
		0x07, 0x0C,			// PKTCTRL1:												// PQT = 0, CRC auto flush = 1, append status = 1, no address check
		0x0B, 0x06,			// FSCTRL1:													// frequency synthesizer control
		0x0D, 0x21,			// FREQ2
		0x0E, 0x65,			// FREQ1
		0x0F, 0x6A,			// FREQ0
		0x10, 0xC8,			// MDMCFG4
		0x11, 0x93,			// MDMCFG3
		0x12, 0x03,			// MDMCFG2
		0x15, 0x34,			// DEVIATN
		0x16, 0x01,         // MCSM2
		0x17, 0x30,			// MCSM1: always go into IDLE
		0x18, 0x18,			// MCSM0
		0x19, 0x16,			// FOCCFG
		0x1B, 0x43,			// AGCTRL2
		//0x1E, 0x28,       // ..WOREVT1: tEVENT0 = 50 ms, RX timeout = 390 us
		//0x1F, 0xA0,		// ..WOREVT0:
		//0x20, 0xFB,		// ..WORCTRL: EVENT1 = 3, WOR_RES = 0
		0x21, 0x56,			// FREND1
		0x25, 0x00,
		0x26, 0x11,			// FSCAL0
		0x2D, 0x35,			// TEST1
		0x3E, 0xC3,			// ?
	};
	for (uint8_t i=0; i<sizeof(initVal); i+=2) {										// write init value to TRX868
		writeReg(_pgmB(initVal[i]), _pgmB(initVal[i+1]));
		//dbg << i << ": " << pHexB(_pgmB(initVal[i])) << " " << pHexB(_pgmB(initVal[i+1])) << '\n';
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
	//ccStrobe(CC1100_SIDLE);
	//uint8_t cnt = 0xff;
	//while(cnt-- && (ccStrobe( CC1100_STX ) & 0x70) != 2)
	//my_delay_us(10);
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

	writeBurst(CC1101_TXFIFO, buf, buf[0]+1);											// write in TX FIFO

	strobe(CC1101_SFRX);																// flush the RX buffer
	strobe(CC1101_STX);																	// send a burst

	for(uint8_t i=0; i< 200; ++i) {														// after sending out all bytes the chip should go automatically in RX mode
		if( readReg(CC1101_MARCSTATE, CC1101_STATUS) == MARCSTATE_RX)
			break;																		//now in RX mode, good
		if( readReg(CC1101_MARCSTATE, CC1101_STATUS) != MARCSTATE_TX) {
			break;																		//neither in RX nor TX, probably some error
		}
		_delay_us(10);
	}

	//uint8_t cnt = 0xff;
	//while(cnt-- && (ccSendByte(CC1101_SRX) & 0x70) != 1)
	//_delay_us(10);

	#ifdef CC_DBG																		// only if cc debug is set
	dbg << F("<- ") << pHexB(buf[0]) << pHexB(buf[1]) << '\n';//pTime();
	#endif

	//dbg << "rx\n";
	return true;
}
uint8_t CC::rcvData(uint8_t *buf) {														// read data packet from RX FIFO
	uint8_t rxBytes = readReg(CC1101_RXBYTES, CC1101_STATUS);							// how many bytes are in the buffer
	
	//dbg << rxBytes << ' ';

	if ((rxBytes & 0x7F) && !(rxBytes & 0x80)) {										// any byte waiting to be read and no overflow?
		buf[0] = readReg(CC1101_RXFIFO, CC1101_CONFIG);									// read data length

		if (buf[0] > CC1101_DATA_LEN)													// if packet is too long
			buf[0] = 0;																	// discard packet
		else {
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
	if (buf[0] > 0) dbg << pHex(buf, buf[0]+1) << '\n';//pTime();
	#endif

	return buf[0];																		// return the data buffer
}
void    CC::setIdle() {																	// put CC1101 into power-down state
	strobe(CC1101_SIDLE);																// coming from RX state, we need to enter the IDLE state first
	strobe(CC1101_SFRX);
	strobe(CC1101_SPWD);																// enter power down state
	//dbg << "pd\n";
}
uint8_t CC::getStatus() {
	return readReg(CC1101_PKTSTATUS, CC1101_STATUS);
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

	// power on cc1101 module and set to RX mode
	_ccSelect;																			// wake up the communication module
	_waitMiso;
	_ccDeselect;
	_delay_ms(1);																		// give some time to come up

	//strobe(CC1101_SIDLE);																// enter idle mode first
	strobe(CC1101_SRX);																	// set RX mode again
	_delay_ms(2);																		// wait a short time to set RX mode

	// check carrier sense for 2ms to avoid wakeup due to normal transmition
	// will be checked in the power management function
	//if (! (getStatus() & (1<<6)) ) return 0;
	//_delay_ms(2);
	return (getStatus() & (1<<6))?1:0;
}

void    CC::strobe(uint8_t cmd) {														// send command strobe to the CC1101 IC via SPI
	_ccSelect;																			// select CC1101
	_waitMiso;																			// wait until MISO goes low
	ccSendByte(cmd);																	// send strobe command
	_ccDeselect;																		// deselect CC1101
}
void    CC::readBurst(uint8_t *buf, uint8_t regAddr, uint8_t len) {						// read burst data from CC1101 via SPI
	_ccSelect;																			// select CC1101
	_waitMiso;																			// wait until MISO goes low
	ccSendByte(regAddr | READ_BURST);													// send register address
	for(uint8_t i=0 ; i<len ; i++) buf[i] = ccSendByte(0x00);							// read result byte by byte
	_ccDeselect;																		// deselect CC1101
}
void    CC::writeBurst(uint8_t regAddr, uint8_t *buf, uint8_t len) {					// write multiple registers into the CC1101 IC via SPI
	_ccSelect;																			// select CC1101
	_waitMiso;																			// wait until MISO goes low
	ccSendByte(regAddr | WRITE_BURST);													// send register address
	for(uint8_t i=0 ; i<len ; i++) ccSendByte(buf[i]);									// send value
	_ccDeselect;																		// deselect CC1101
}
uint8_t CC::readReg(uint8_t regAddr, uint8_t regType) {									// read CC1101 register via SPI
	_ccSelect;																			// select CC1101
	_waitMiso;																			// wait until MISO goes low
	ccSendByte(regAddr | regType);														// send register address
	uint8_t val = ccSendByte(0x00);														// read result
	_ccDeselect;																		// deselect CC1101
	return val;
}
void    CC::writeReg(uint8_t regAddr, uint8_t val) {									// write single register into the CC1101 IC via SPI
	_ccSelect;																			// select CC1101
	_waitMiso;																			// wait until MISO goes low
	ccSendByte(regAddr);																// send register address
	ccSendByte(val);																	// send value
	_ccDeselect;																		// deselect CC1101
}
