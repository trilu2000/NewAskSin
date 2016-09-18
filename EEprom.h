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
#include "AS_typedefs.h"
#define maxMsgLen 16																		// define max message length in byte

/**
 * @file EEprom.h
 * Include file with EE class definiton and forward declaration of
 * external helper functions
 */

/**
 * @fn void everyTimeStart()
 * @brief Callback for actions after bootup
 *
 * This function is called when AS has started and before the main loop runs.
 */
extern void everyTimeStart(void);

/**
 * @fn void firstTimeStart()
 * @brief Callback for actions after EEprom deletion
 *
 * This function needs to be defined in the user code space. It can be used to
 * set the data of complete Lists with EE::setList() or single registers using
 * EE::setListArray()
 */
extern void firstTimeStart(void);


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
class EE {
public:		//---------------------------------------------------------------------------------------------------------
	uint8_t  setList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// set a complete list to the eeprom
	uint8_t  setListArray(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t len, uint8_t *buf);// ok, set registers from a string

	uint8_t  getList(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t *buf);				// get a complete list in to a given buffer

	uint8_t  getRegAddr(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t addr);			// ok, gets a single register value


	EE();																				// class constructor
	void     init(void);
	void     initHMKEY(void);
	void     testModul(void);															// prints register.h definition on console




	// register functions
	uint8_t  countRegListSlc(uint8_t cnl, uint8_t lst);									// ok, counts the slices for a complete regs transmition
	uint8_t  getRegListSlc(uint8_t cnl, uint8_t lst, uint8_t idx, uint8_t slc, uint8_t *buf);// ok, generates answer to a channel/list request

	//uint8_t getListForMsg3(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);
	//void    getCnlListByPeerIdx(uint8_t cnl, uint8_t peerIdx);
	//void    setListFromModule(uint8_t cnl, uint8_t peerIdx, uint8_t *data, uint8_t len);

	//uint8_t   getRegListIdx(uint8_t cnl, uint8_t lst);									// ok, returns the respective line of cnlTbl
	//uint8_t   checkIndex(uint8_t cnl, uint8_t lst, uint8_t idx);
};

extern const uint8_t devIdnt[];
//extern const uint8_t cnl_max;
//extern const uint16_t cm_size;

extern EE ee;


	
/**
 * @brief Global device definition. Must be declared in user space.
 *
 *
 * @todo Insert description and example for devDef
 */
//extern EE::s_devDef devDef;																// initial register.h

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
extern uint8_t *MAID;

/**
 * @brief Global definition of device HMSerialData. Must be declared in user space.
 *
 * The HMSerialData holds the default HMID, HMSerial and HMKEY.
 * At every start, values of HMID and HMSerial was copied to related variables.
 * The HKEY was only copied at initial sketch start in the EEprom
 */
extern const uint8_t HMSerialData[] PROGMEM;

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

/**
 * @brief Global definition of device AES key.
 *
 */
extern uint8_t HMKEY[];

/**
 * @brief Index of AES key of master.
 *
 */
extern uint8_t hmKeyIndex[];



//- some helpers ----------------------------------------------------------------------------------------------------------
inline uint16_t crc16(uint16_t crc, uint8_t a);											// crc function
uint8_t  isEmpty(void *p1, uint8_t len);												// check if a byte array is empty
#define isEqual(p1,p2,len) memcmp(p1, p2, len)?0:1										// check if a byte array is equal

#endif

