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

/**
 * @file EEprom.h
 * Ínclude file with EE class definiton and forward declaration of
 * external helper functions
 */

/**
 * @fn void everyTimeStart()
 * @brief Callback for actions after bootup
 *
 * This function is called when AS has started and before the main loop runs.
 */
extern void everyTimeStart(void);															// add this function in register.h to setup default values every start

/**
 * @fn void firstTimeStart()
 * @brief Callback for actions after EEprom deletion
 *
 * This function needs to be defined in the user code space. It can be used to
 * set the data of complete Lists with EE::setList() or single registers using
 * EE::setListArray()
 */
extern void firstTimeStart(void);															// only on first start of the device

//- class definition ------------------------------------------------------------------------------------------------------
/**
 * @brief Helper class for providing access to non-volatile data in the EEprom.
 *
 * This class is used by the main AS class to handle non-volatile data stored in the EEprom
 * and accessed in form of Registers in the context of Lists.
 *
 * Every device requires a set of definitions that define its properties and capabilities.
 *
 * @todo Describe device properties and how they are represented (register.h)
 * @todo Describe registers, channel slice addresses, list tables and peer tables
 */
class EE {
	friend class AS;
	friend class RV;
	friend class CB;

  public:		//---------------------------------------------------------------------------------------------------------

    /**
     * @brief Channel Table Entry
     * This structure is used in the channels definition, where all existing channels
     * are assigned with channel slice address information and EEprom addresses.
     *
     * @include docs/snippets/register-h-cnlTblAddr.cpp
     *
     */
	struct s_cnlTbl {	// channel table holds all information regarding channels and lists
		const uint8_t cnl;     ///< Channel
		const uint8_t lst;     ///< List within the channel
		const uint8_t sIdx;    ///< Index of first entry in channel slice address definition
		const uint8_t sLen;    ///< Number of registers
		const uint16_t pAddr;  ///< Address of first byte in EEprom memory
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
	uint8_t  getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// get a complete list in to a given buffer
	uint8_t  setList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// set a complete list to the eeprom
	uint8_t  getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr);			// ok, gets a single register value
	uint32_t getHMID(void);																// get HMID as 32 bit integer

  protected:	//---------------------------------------------------------------------------------------------------------
  public:		//---------------------------------------------------------------------------------------------------------
  //private:	//---------------------------------------------------------------------------------------------------------
	EE();																				// class constructor
	void     init(void);
	void     getMasterID(void);
	void     testModul(void);															// prints register.h definition on console
	uint8_t  isHMIDValid(uint8_t *toID);												// ok, check if a valid pair was given
	uint8_t  isPairValid(uint8_t *reID);												// ok, check if a valid pair was given
	uint8_t  isBroadCast(uint8_t *reID);
	uint8_t  getIntend(uint8_t *reId, uint8_t *toId, uint8_t *peId);

	// peer functions
	void     clearPeers(void);															// ok, clears complete peer database
	uint8_t  isPeerValid (uint8_t *peer);												// ok, checks if a valid peer was given

	uint8_t  countFreeSlots(uint8_t cnl);												// ok, counts the free peer slots of a channel
	uint8_t  getIdxByPeer(uint8_t cnl, uint8_t *peer);									// ok, find the index of the respective peer
	uint8_t  getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer);						// ok, returns the respective peer of the given index
	uint8_t  addPeer(uint8_t cnl, uint8_t *peer);										// ok, writes a peer in the database on first free slot
	uint8_t  remPeer(uint8_t cnl, uint8_t *peer);										// ok, writes a zero to the respective slot
	uint8_t  countPeerSlc(uint8_t cnl);													// ok, count the slices for function getPeerListSlc
	uint8_t  getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf);					// ok, returns the whole peer database as a string
	uint8_t  getPeerSlots(uint8_t cnl);													// ok, returns max peers per channel

	// register functions
	void     clearRegs(void);															// ok, clears register space

	uint8_t  countRegListSlc(uint8_t cnl, uint8_t lst);									// ok, counts the slices for a complete regs transmition
	uint8_t  getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf);// ok, generates answer to a channel/list request
	uint8_t  setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf);// ok, set registers from a string

	//uint8_t getListForMsg3(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);
	//void    getCnlListByPeerIdx(uint8_t cnl, uint8_t peerIdx);
	//void    setListFromModule(uint8_t cnl, uint8_t peerIdx, uint8_t *data, uint8_t len);

	uint8_t  getRegListIdx(uint8_t cnl, uint8_t lst);									// ok, returns the respective line of cnlTbl
	uint8_t  checkIndex(uint8_t cnl, uint8_t lst, uint8_t idx);
};

/**
 * @brief Global device register channel table definition. Must be declared in user space.
 *
 *
 *
 */
extern EE::s_cnlTbl cnlTbl[];															// initial register.h

/**
 * @brief Global peer table definition. Must be declared in user space.
 *
 *
 * @todo Provide description and example for peerTbl
 */
extern EE::s_peerTbl peerTbl[];															// initial register.h

/**
 * @brief Global device definition. Must be declared in user space.
 *
 *
 * @todo Provide description and example for devDef
 */
extern EE::s_devDef devDef;																// initial register.h

/**
 * @brief Global definition of master HM-ID (paired central).
 *
 * MAID is valid after initialization of AS with AS::init(). While not paired to a central,
 * MAID equals the broadcast address 000000. This is the case after first upload of a firmware
 * when an unconfigured EEprom is encountered (EEprom magic number does not match) or after a
 * reset of the device (RESET command sent by paired central or initiated by means of the
 * config button).
 *
 * The following example shows how HMID can be used for debugging purposes in user space:
 * @code
 * Serial << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");
 * @endcode
 */
extern uint8_t MAID[];

/**
 * @brief Global definition of device HM-ID. Must be declared in user space.
 *
 * The HM-ID is a network-wide unique ID that is preassigned to each individual device along with
 * its serial number HMSR (See @ref device_address for details).
 *
 * The following example demonstrates how the HM-ID is declared in a way that allows to
 * override the default during compile time with a compiler switch:
 * @include docs/snippets/hmserial.c
 */
extern uint8_t HMID[];

/**
 * @brief Global definition of device serial number. Must be declared in user space.
 *
 * The following example demonstrates how the serial number is declared in a way that allows to
 * override the default during compile time with a compiler switch:
 * @include docs/snippets/hmserial.c
 */
extern uint8_t HMSR[];


//- some helpers ----------------------------------------------------------------------------------------------------------
uint16_t crc16(uint16_t crc, uint8_t a);												// crc function
uint8_t  compArray(void *p1, void *p2, uint8_t len);									// compare two byte arrays
uint8_t  isEmpty(void *p1, uint8_t len);												// check if a byte array is empty

#endif

