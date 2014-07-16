//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define EE_DBG
#include "EE.h"

// public:		//---------------------------------------------------------------------------------------------------------

EE::EE() {
} 

void EE::init(void) {
	#ifdef EE_DBG																		// only if ee debug is set
	dbg.begin(57600);																	// serial setup
	dbg << F("\n....\n");																// ...and some information
	dbg << F("EE.\n");																	// ...and some information
	#endif

	// check for first time run by checking magic byte, if yes then prepare eeprom and set magic byte
	uint16_t eeCRC = 0, flCRC = 0;														// define variable for storing crc
	uint8_t  *p = (uint8_t*)devDef.cnlTbl;												// cast devDef to char
	
	for (uint8_t i = 0; i < (devDef.lstNbr*6); i++) {									// step through all bytes of the channel table, one line has 6 byte
		flCRC = crc16(flCRC, _pgmB(&p[i]));												// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eeCRC);													// get magic byte from eeprom
	
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("crc, flash: ") << flCRC << F(", eeprom: ") << eeCRC << '\n';				// ...and some information
	#endif
	
	if(flCRC!=eeCRC) {																	// first time detected, format eeprom, load defaults and write magic byte
		// formating eeprom
		

		// store defaults, incl HMID and serial
		//for (uint8_t i = 0; i < eepDef.nbr; i++) {
		//	dbg << F("addr: ") << _pgmB(&eepDef.eepDefTbl[i].addr) << F(", len: ") << _pgmB(&eepDef.eepDefTbl[i].len) << '\n';//<< F(", byte: ") << _pgmB(&devDef.cnlTbl[i].sLen) << '\n';
		//}
		

		// write magic byte
		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("writing magic byte\n");												// ...and some information
		#endif
		setEEPromBlock(0,2,(void*)&flCRC);												// write magic byte to eeprom
	}

	// load hmid and serial from eeprom
	getEEPromBlock(2,3,(void*)&HMID);
	getEEPromBlock(5,10,(void*)&HMSR);
	

}


void EE::printRegDef(void) {															// prints register.h content on console
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << '\n' << pLine;
	dbg << F("register.h - lists\n");
	dbg << pLine;
	dbg << F("channels: ") << devDef.cnlNbr << F(", list obj: ") << devDef.lstNbr << '\n';
	for (uint8_t i = 0; i < devDef.lstNbr; i++) {
		dbg << F("cnl: ") << _pgmB(&devDef.cnlTbl[i].cnl) << F(", lst: ") << _pgmB(&devDef.cnlTbl[i].lst) << F(", byte: ") << _pgmB(&devDef.cnlTbl[i].sLen) << '\n';
	}
	dbg << pLine;
	dbg << F("peer database\n");
	dbg << pLine;
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {
		dbg << F("cnl: ") << _pgmB(&devDef.peerTbl[i].cnl) << F(", peers: ") << _pgmB(&devDef.peerTbl[i].pMax) << '\n';
	}
	
	dbg << pLine;
	dbg << "HMID: " << pHex(HMID,3) << ", serial: "; dbg.write(HMSR,10);dbg << '\n';
	dbg << pLine;

	dbg << F("\nsome tests...\n");
	dbg << "check getPeerSlots: " << getPeerSlots(1) << '\n';
	dbg << "check getPeerSlots: " << getPeerSlots(2) << '\n';

	#endif
}
















// private:		//---------------------------------------------------------------------------------------------------------
uint8_t EE::getPeerSlots(uint8_t cnl) {													// returns the amount of possible peers
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {										// step through all channels
		if (_pgmB(&devDef.peerTbl[i].cnl) == cnl) {										// until we found a fitting					
			return _pgmB(&devDef.peerTbl[i].pMax);										// return the max amount of peers
		}
	}
	return 0;	
}



uint16_t crc16(uint16_t crc, uint8_t a) {
	uint16_t i;

	crc ^= a;
	for (i = 0; i < 8; ++i) {
		if (crc & 1)
		crc = (crc >> 1) ^ 0xA001;
		else
		crc = (crc >> 1);
	}
	return crc;
}