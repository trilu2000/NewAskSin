//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin eeprom functions -----------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _EE_H
#define _EE_H

#include "HAL.h"
#define maxMsgLen 16																		// define max message length in byte

//- class definition ------------------------------------------------------------------------------------------------------
class EE {
	friend class AS;
	friend class RV;
	friend class CB;
	
  public:		//---------------------------------------------------------------------------------------------------------

	struct s_cnlTbl {	// channel table holds all information regarding channels and lists
		const uint8_t cnl;
		const uint8_t lst;
		const uint8_t sIdx;
		const uint8_t sLen;
		const uint16_t pAddr;
	};

	struct s_peerTbl {	// peer table holds information were to find peers in eeprom
		const uint8_t cnl;
		const uint8_t pMax;
		const uint16_t pAddr;
	};

	struct s_devDef {	// device definition table
		const uint8_t   cnlNbr;															// number of channels
		const uint8_t   lstNbr;															// number of lists
		const uint8_t   *devIdnt;														// pointer to device identifier
		const uint8_t   *cnlAddr;														// pointer to slice addresses
	};
	
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	
  public:		//---------------------------------------------------------------------------------------------------------
	uint8_t getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// get a complete list in to a given buffer
	uint8_t setList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// set a complete list to the eeprom
	uint8_t getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr);			// ok, gets a single register value

  protected:	//---------------------------------------------------------------------------------------------------------
  private:	    //---------------------------------------------------------------------------------------------------------
	EE();																				// class constructor
	void    init(void);
	void    getMasterID(void);
	void    testModul(void);															// prints register.h definition on console
	uint8_t isHMIDValid(uint8_t *toID);													// ok, check if a valid pair was given
	uint8_t isPairValid(uint8_t *reID);													// ok, check if a valid pair was given
	uint8_t isBroadCast(uint8_t *reID);
	uint8_t getIntend(uint8_t *reId, uint8_t *toId);
		
	// peer functions
	void    clearPeers(void);															// ok, clears complete peer database
	uint8_t isPeerValid (uint8_t *peer);												// ok, checks if a valid peer was given

	uint8_t countFreeSlots(uint8_t cnl);												// ok, counts the free peer slots of a channel
	uint8_t getIdxByPeer(uint8_t cnl, uint8_t *peer);									// ok, find the index of the respective peer
	uint8_t getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer);						// ok, returns the respective peer of the given index
	uint8_t addPeer(uint8_t cnl, uint8_t *peer);										// ok, writes a peer in the database on first free slot 
	uint8_t remPeer(uint8_t cnl, uint8_t *peer);										// ok, writes a zero to the respective slot
	uint8_t countPeerSlc(uint8_t cnl);													// ok, count the slices for function getPeerListSlc
	uint8_t getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf);						// ok, returns the whole peer database as a string	
	uint8_t getPeerSlots(uint8_t cnl);													// ok, returns max peers per channel
	
	// register functions
	void    clearRegs(void);															// ok, clears register space

	uint8_t countRegListSlc(uint8_t cnl, uint8_t lst);									// ok, counts the slices for a complete regs transmition
	uint8_t getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf);// ok, generates answer to a channel/list request
	uint8_t setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf);// ok, set registers from a string
	
	//uint8_t getListForMsg3(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);		
	//void    getCnlListByPeerIdx(uint8_t cnl, uint8_t peerIdx);							
	//void    setListFromModule(uint8_t cnl, uint8_t peerIdx, uint8_t *data, uint8_t len); 

	uint8_t getRegListIdx(uint8_t cnl, uint8_t lst);									// ok, returns the respective line of cnlTbl

}; 
extern EE::s_cnlTbl cnlTbl[];															// initial register.h
extern EE::s_peerTbl peerTbl[];															// initial register.h
extern EE::s_devDef devDef;																// initial register.h
extern uint8_t MAID[];
extern uint8_t HMID[];
extern uint8_t HMSR[];


//- some helpers ----------------------------------------------------------------------------------------------------------
uint16_t crc16(uint16_t crc, uint8_t a);												// crc function
uint8_t  compArray(void *p1, void *p2, uint8_t len);									// compare two byte arrays
uint8_t  isEmpty(void *p1, uint8_t len);												// check if a byte array is empty

#endif

