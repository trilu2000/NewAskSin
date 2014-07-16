//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _EE_H
#define _EE_H

#include "HAL.h"

//- struct declaration ----------------------------------------------------------------------------------------------------
struct s_cnlTbl {		// channel table holds all information regarding channels and lists
	uint8_t cnl;
	uint8_t lst;
	uint8_t sIdx;
	uint8_t sLen;
	uint16_t pAddr;
};
struct s_peerTbl {		// peer table holds information were to find peers in eeprom
	uint8_t cnl;
	uint8_t pMax;
	uint16_t pAddr;
};
struct s_eepDefTbl {	// eeprom default table holds information which has to be added at boot time to the eeprom
	uint16_t addr;
	uint8_t len;
	uint8_t *contPtr;
};


//- class definition ------------------------------------------------------------------------------------------------------
class EE {
  public:		//---------------------------------------------------------------------------------------------------------
	struct s_devDef {	// device definition table
		const uint8_t   cnlNbr;															// number of channels
		const uint8_t   lstNbr;															// number of lists
		const uint8_t   *devIdnt;														// pointer to device identifier
		const uint8_t   *cnlAddr;														// pointer to slice addresses
		const s_cnlTbl  *cnlTbl;														// pointer to array of elements
		const s_peerTbl *peerTbl;														// pointer to peer table
	};

	struct s_eepDef {	// eeprom default definition table
		const uint8_t nbr;																// number of lines to add
		const s_eepDefTbl *eepDefTbl;													// pointer to defaults table
	};

	uint8_t HMID[3];	// own ID for communication
	uint8_t HMSR[10];	// serial ID for pairing, etc
	
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	EE();																				// class constructor
	void init(void);
	void printRegDef(void);																// prints register.h definition on console
	//uint8t isPeerValid();
	//uint8t isPairValid();

  private:		//---------------------------------------------------------------------------------------------------------
	uint8_t getPeerSlots(uint8_t cnl);	
}; 
extern EE::s_devDef devDef;																// initial register.h
extern EE::s_eepDef eepDef; 


//- some helpers ----------------------------------------------------------------------------------------------------------
uint16_t crc16(uint16_t crc, uint8_t a);												// crc function


#endif
