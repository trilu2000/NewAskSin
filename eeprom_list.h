/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin eeprom list functions ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _EEPROM_LIST_H
#define _EEPROM_LIST_H

#include "HAL.h"

/**
* @brief Helper class for providing access to non-volatile data in the EEprom.
*
* This class is used by the main AS class to handle non-volatile data stored in
* the EEprom and accessed in form of Registers in the context of Lists.
* Additionally, it maintains the peer database to validate incoming messages and
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
* Register values are stored for each possible Channel/List combination.
* A typical device knows exactly one List0 (Channel 0), and for each channel a combination of
* List1 and either List3 or List4.
*
* For each channel, the number of possible peer devices can be specified individually.
* With chanX_listY_len as the number of register bytes for list Y in channel X and
* peers_chanX as the maximum number of peers for channel X, the memory footprint can be
* calculated as follows:
*
* | Channel | List0 | List1 | List3 | List4 | Size                   | Note      |
* | :-----: | :---: | :---- | :---: | :---: | :--------------------- | --------- |
* |    0    | chan0_list0_len | | | | chan0_l0_len                   | mandatory |
* |    1    | | chan1_list1_len | | | peers_chan1 * chan1_list1_len  | mandatory |
* |    1    | | | chan1_list3_len | | peers_chan1 * chan1_list3_len  | actor     |
* |    1    | | | | chan1_list4_len | peers_chan1 * chan1_list4_len  | sensor    |
* |    2    | | chan2_list1_len | | | peers_chan2 * chan1_list1_len  | mandatory |
* |    2    | | | chan2_list3_len | | peers_chan2 * chan1_list3_len  | actor     |
* |    2    | | | | chan2_list4_len | peers_chan2 * chan1_list4_len  | sensor    |
* |         | | | |                 |                                |           |
* |    N    | | chanN_list1_len | | | peers_chan2 * chan1_list1_len  | mandatory |
* |    N    | | | chanN_list3_len | | peers_chan2 * chan1_list3_len  | actor     |
* |    N    | | | | chanN_list4_len | peers_chan2 * chan1_list4_len  | sensor    |
*
*/
class EE_LIST {
public:		//---------------------------------------------------------------------------------------------------------
	EE_LIST();																			// constructor

	//uint8_t  getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// get a complete list in to a given buffer
	uint8_t  getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr);			// ok, gets a single register value

	//uint8_t  setList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// set a complete list to the eeprom
	uint8_t  setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf);// ok, set registers from a string

	//uint8_t  countRegListSlc(uint8_t cnl, uint8_t lst);									// ok, counts the slices for a complete regs transmition
	//uint8_t  getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf);// ok, generates answer to a channel/list request

	s_list_table* find_list(uint8_t cnl, uint8_t lst);
};

extern EE_LIST ee_list;

#endif