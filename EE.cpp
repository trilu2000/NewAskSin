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
	uint16_t eepromCRC = 0, flashCRC = 0;												// define variable for storing crc
	uint8_t  *p = (uint8_t*)devDef.cnlTbl;												// cast devDef to char
	
	for (uint8_t i = 0; i < (devDef.lstNbr*6); i++) {									// step through all bytes of the channel table, one line has 6 byte
		flashCRC = crc16(flashCRC, _pgmB(&p[i]));										// calculate the 16bit checksum for the table
	}
	getEEPromBlock(0,2,(void*)&eepromCRC);												// get magic byte from eeprom
	
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("crc, flash: ") << flashCRC << F(", eeprom: ") << eepromCRC << '\n';		// ...and some information
	#endif
	
	if(flashCRC!=eepromCRC) {															// first time detected, format eeprom, load defaults and write magic byte
		// formating eeprom
		clearPeers();

		// store defaults, incl HMID and serial
		//for (uint8_t i = 0; i < eepDef.nbr; i++) {
		//	dbg << F("addr: ") << _pgmB(&eepDef.eepDefTbl[i].addr) << F(", len: ") << _pgmB(&eepDef.eepDefTbl[i].len) << '\n';//<< F(", byte: ") << _pgmB(&devDef.cnlTbl[i].sLen) << '\n';
		//}
		

		// write magic byte
		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("writing magic byte\n");												// ...and some information
		#endif
		setEEPromBlock(0,2,(void*)&flashCRC);											// write magic byte to eeprom
	}

	// load hmid and serial from eeprom
	getEEPromBlock(2,3,(void*)&HMID);
	getEEPromBlock(5,10,(void*)&HMSR);
	

}
void EE::testModul(void) {															// prints register.h content on console
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

	dbg << F("\ntest eeprom functions...\n");
	uint8_t aTst1[10] = {'t','l','u','1','0','0','1','2','3','4'};
	setEEPromBlock(500,10,(void*)&aTst1);
	_delay_ms(1000);
	uint8_t aTst2[10];
	getEEPromBlock(500,10,(void*)&aTst2);
	dbg << F("tst2: "); dbg.write(aTst2,10); dbg << '\n';		
	dbg << F("clear eeprom\n");
	clearEEPromBlock(500,10);
	_delay_ms(1000);
	getEEPromBlock(500,10,(void*)&aTst2);
	dbg << F("tst2: ") << pHex(aTst2,10) << '\n';

	dbg << F("\ntest count, add, rem peers...\n");
	dbg << F("cnl 2: ") << countFreeSlots(2) << '\n';
	uint8_t aTst3[4] = {01,02,03,04,};
	//setEEPromBlock(_pgmB(&devDef.peerTbl[0].pAddr),4,(void*)&aTst3);
	addPeer(1, 1, aTst3);
	addPeer(1, 2, aTst3);
	_delay_ms(500);
	dbg << F("cnl 1: ") << countFreeSlots(1) << '\n';
	dbg << F("remove idx2\n");
	remPeer(1, 2);
	_delay_ms(500);
	dbg << F("cnl 1: ") << countFreeSlots(1) << '\n';

	dbg << F("\ntest get peer index...\n");
	dbg << F("idx 1: ") << getPeerIdx(1,aTst3) << '\n';

	dbg << F("\nis pair valid...\n");
	dbg << F("0: ") << isPairValid(aTst3) << '\n';
	dbg << F("1: ") << isPairValid(HMID) << '\n';

	dbg << F("\nis peer valid...\n");
	dbg << F("0: ") << isPeerValid(HMID) << '\n';
	dbg << F("1: ") << isPeerValid(aTst3) << '\n';
	
	#endif
}
uint8_t EE::isPairValid (uint8_t *pair) {
	if (pair[0] != HMID[0]) return 0;
	if (pair[1] != HMID[1]) return 0;
	if (pair[2] != HMID[2]) return 0;
	return 1; 
}

void    EE::clearPeers(void) {
	for (uint8_t i = 0; i < devDef.cnlNbr; i++) {										// step through all channels
		clearEEPromBlock(_pgmB(&devDef.peerTbl[i].pAddr), _pgmB(&devDef.peerTbl[i].pMax) * 4);

		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("clear eeprom, addr ") << _pgmB(&devDef.peerTbl[i].pAddr) << F(", len ") << (_pgmB(&devDef.peerTbl[i].pMax) * 4) << '\n';																	// ...and some information
		#endif
	}
}
uint8_t EE::isPeerValid (uint8_t *peer) {
	for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {										// step through all channels
		if (getPeerIdx(i, peer) != 0xff) return 1;										// if found a valid peer is found return 1
	}
	return 0;																			// otherwise 0
}

uint8_t EE::countFreeSlots(uint8_t cnl) {
	uint8_t bCounter = 0;																// set counter to zero
	uint32_t lPeer;
	
	if (cnl > devDef.cnlNbr) return 0;													// return if channel is out of range
	
	#ifdef EE_DBG																		// only if ee debug is set
	dbg << F("cFS: ") << _pgmB(&devDef.peerTbl[cnl-1].pMax) << '\n';
	#endif
	
	for (uint8_t i = 0; i < _pgmB(&devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);	// get peer from eeprom
		if (lPeer == 0) bCounter++;														// increase counter if peer slot is empty

		#ifdef EE_DBG																	// only if ee debug is set
		dbg << F("addr: ") << (_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4)) << F(", lPeer: ") << pHex(((uint8_t*)&lPeer),4) << '\n';
		#endif
	}
	return bCounter;																	// return the counter
}
uint8_t EE::getPeerIdx(uint8_t cnl, uint8_t *peer) {
	uint32_t lPeer;
	for (uint8_t i = 0; i < _pgmB(&devDef.peerTbl[cnl-1].pMax); i++) {					// step through the possible peer slots
		getEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(i*4), 4, (void*)&lPeer);	// get peer from eeprom
		if (lPeer == *(uint32_t*)peer) return i;										// if result matches then return slot index
		//dbg << i << ": " << lPeer << ", s: " << (*(uint32_t*)peer) << '\n';
	}
	return 0xff;
}
uint8_t EE::addPeer(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	setEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4, (void*)peer);
}
uint8_t EE::remPeer(uint8_t cnl, uint8_t idx) {
	clearEEPromBlock(_pgmB(&devDef.peerTbl[cnl-1].pAddr)+(idx*4), 4);
}


// private:		//---------------------------------------------------------------------------------------------------------
uint8_t EE::getPeerSlots(uint8_t cnl) {													// returns the amount of possible peers
	if (cnl > devDef.cnlNbr) return 0;
	return _pgmB(&devDef.peerTbl[cnl-1].pMax);											// return the max amount of peers
}



//- some helpers ----------------------------------------------------------------------------------------------------------
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