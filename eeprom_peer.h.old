/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin peer database functions ----------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _EEPROM_PEER_H
#define _EEPROM_PEER_H

#include "HAL.h"

/**
* @brief Helper class for providing access to non-volatile data in the EEprom.
*
* This class is used by the main AS class to handle non-volatile peer data stored in
* the EEprom. It maintains the peer database to validate incoming messages and
* help formulate outgoing messages.
*
* @paragraph section_device_properties Device properties
* Every device requires a set of definitions that define its properties and capabilities:
*   - the device definition @c EE::s_devDef @c devDef comprising
*     device identity and the channel slice address definition
*   - the channel table @c EE::s_cnlTbl @c cnlTbl referring to the channel slice address definition
*   - the peer device table @c EE::s_peerTbl @c peerTbl
*
* @todo Insert defDev example here.
*
* @paragraph section_eeprom_memory_layout EEprom memory layout
*
* | Start      | Length | Content                     |
* | ---------- | -----: | --------------------------- |
* | 0x0000     |      2 | magic byte                  |
* | 0x0002     |      3 | HMID                        |
* | 0x0005     |     10 | serial number               |
* | 0x000f     |    N_l | register values (see below) |
* | 0x000f+N_l |    N_p | peer list (see below)       |
*
* @todo Insert description of peerTbl here
*/
class EE_PEER {
public:		//---------------------------------------------------------------------------------------------------------
	EE_PEER();																			// constructor

	//uint8_t  isPeerValid(uint8_t *peer);												// ok, checks if a valid peer was given
	//void     getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer);						// ok, returns the respective peer of the given index

	//uint8_t  addPeers(uint8_t cnl, uint8_t *peer);										// ok, writes a peer in the database on first free slot
	//uint8_t  remPeers(uint8_t cnl, uint8_t *peer);										// ok, writes a zero to the respective slot

	//uint8_t  countPeerSlc(uint8_t cnl);													// ok, count the slices for function getPeerListSlc
	//uint8_t  getPeerListSlc(uint8_t cnl, uint8_t slc, uint8_t *buf);					// ok, returns the whole peer database as a string

	//uint8_t  countFreeSlots(uint8_t cnl);												// ok, counts the free peer slots of a channel
	//void     clearPeers(void);															// ok, clears complete peer database
	//inline uint8_t getIdxByPeer(uint8_t cnl, uint8_t *peer);							// ok, find the index of the respective peer
};

//extern EE_PEER ee_peer;

#endif