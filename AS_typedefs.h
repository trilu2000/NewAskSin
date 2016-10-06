//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin receive function -----------------------------------------------------------------------------------------------
//- message definition mainly from homematic modul HMConfig.pm in FHEM
//- -----------------------------------------------------------------------------------------------------------------------

#include "HAL.h"
#include "00_debug-flag.h"
#include "wait_timer.h"

#ifndef _AS_TYPEDEFS_H
#define _AS_TYPEDEFS_H

#define MaxDataLen   40						// maximum length for send and receive strings


/*
* @brief Helper struct for all AES relevant variables
*/
/*typedef struct ts_aes {
	uint8_t  key_part_index;				// key part index
	uint8_t  signing_request[6];			// placeholder for signing request
	uint8_t  temp_hmkey[16];				// temp hmkey 
	uint8_t  new_hmkey[16];					// new hmkey for key exchange
	uint8_t  new_hmkey_index[1];			// new hmkey index
	uint16_t randomSeed;					// random seed flag
	uint8_t  resetStatus;					// reset status flag
} s_aes;*/

/*
* @brief Helper struct to remember on the config mode status in asksin main function
*/
typedef struct ts_config_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	uint8_t   cnl;							// channel which was opened by config start message
	uint8_t   lst;							// list which was opened
	uint8_t   idx_peer;						// and the peer index
	waitTimer timer;						// config mode timeout
} s_config_mode;

/*
* @brief Helper struct to remember on the pairing mode status in asksin main function
*/
typedef struct ts_pair_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	waitTimer timer;						// pairing mode timeout
} s_pair_mode;


/* 
* @brief First bytes of eeprom holds all device specific information for identification
*/
typedef struct ts_eeprom_start_table {
	uint16_t MAGIC;							//  2 byte - magic byte
	uint8_t  HMID[3];						//  3 byte - homematic id
	uint8_t  SERIAL_NR[10];					// 10 byte - serial number
	uint8_t  HMKEY_INDEX;					//  1 byte - aes key index
	uint8_t  HMKEY[16];						// 16 byte - homematic aes key
} s_ee_start;


/*
* @brief Every channel has two lists, the first list holds the configuration which is required to drive the channel,
*        the second list is related to peer messages and holds all information which are required to drive the functionality
*        of the channel in combination with peer devices.
*        Therefore we have in every channel two lists - lstC and lstP organized in structs
*
* cnl     - holds the channel information
* lst     - holds the list number
* *reg    - pointer to register values in PROGMEM
* *def    - pointer to default values in PROGMEM
* *val    - pointer to value array which is dynamic loaded from eeprom
* len     - length of register, defaults and value array
* ee_addr - start address for channel in eeprom
*
* get_ee_addr(idx = 0)  - calculate the eeprom address on base of the index, list0/1 didnt need an index
* load_list(idx = 0)    - load the respective list from the eeprom
* save_list(idx = 0)    - writes the respective list to the eeprom
* load_default(idx = 0) - load defaults from PROGMEM
* ptr_to_val(reg_addr, idx = 0) - search a specific register address and return a pointer to the value
*
*/
typedef struct ts_list_table {
	uint8_t cnl;							// holds the channel information
	uint8_t lst;							// holds the list number
	const uint8_t *reg;						// pointer to register values in PROGMEM
	const uint8_t *def;						// pointer to default values in PROGMEM
	uint8_t *val;							// pointer to value array which is dynamic loaded from eeprom
	uint8_t len;							// length of register, defaults and value array
	uint16_t ee_addr;						// start address for channel in eeprom

	uint16_t get_ee_addr(uint8_t idx = 0) {						// calculate the eeprom address on base of the index, list0/1 didnt need an index
		return ee_addr + (idx * len);
	}

	void load_list(uint8_t idx = 0) {							// load the respective list from the eeprom
		getEEPromBlock(get_ee_addr(idx), len, val);
	}

	void save_list(uint8_t idx = 0) {							// writes the respective list to the eeprom
		setEEPromBlock(get_ee_addr(idx), len, val);
	}

	void load_default() {										// load defaults from PROGMEM
		memcpy_P(val, def, len);
	}

	uint8_t* ptr_to_val(uint8_t reg_addr, uint8_t idx = 0) {	// search a specific register address and return a pointer to the value
		const void *pAddr =  memchr_P(reg, reg_addr, len);
		if (!pAddr) return NULL;
		return val + (uint16_t)pAddr - (uint16_t)reg + (idx * len);
	}

	uint8_t get_nr_slices_pairs(uint8_t byte_per_msg = 16) {	// calculates the amount of needed slices to send all peers depending on the given msg length in peers per message
		uint8_t slices = 0;	int16_t total = (len * 2);			// len of register + len of value
		while ((total -= byte_per_msg) > 0) slices++;
		return ++slices;
	}

	uint8_t get_slice_pairs(uint8_t idx, uint8_t slc, uint8_t *buf, uint8_t byte_per_msg = 16) { // returns a sliced peer list
		load_list(idx);																		// load the eeprom content by idx into the value table
		byte_per_msg /= 2;																	// divided by 2 while we mix two arrays
		uint8_t slc_start = slc * byte_per_msg;												// calculate the start point for reg and val
		uint8_t slc_end = slc_start + byte_per_msg;											// calculate the corresponding end point
		if (slc_end > len) slc_end = len;													// if calculated end point is bigger than the physical

		for (uint8_t i = slc_start; i < slc_end; i++) {	memcpy_P(buf++, &reg[i], 1); memcpy(buf++, &val[i], 1);	}
		return (slc_end - slc_start) * 2;
	}

} s_list_table;


/*
* @brief Peer Device Table Entry
*
* This structure is used to specify the number of possible peers per channel and
* assign corresponding EEprom memory sections where peer information is to be stored.
*
* For each channel and peered device, 4 bytes are written to EEprom memory denoting the
* peer device HMID (3 bytes) and peer device channel (1 byte). Consequently, the following
* definition with 6 possible peers for channel 1 will use 24 bytes in EEprom memory.
*/
typedef struct ts_peer_table {
	uint8_t  max;							// maximum number of peer devices
	uint16_t ee_addr;						// address of configuration data in EEprom memory
	uint8_t  dont_use_peer[4];				// placeholder for a peer id from or to eeprom

	uint8_t *get_peer(uint8_t idx) {							// reads a peer address by idx from the database into the struct peer
		if (idx >= max) return NULL;
		getEEPromBlock(ee_addr + (idx * 4), 4, dont_use_peer);
		return dont_use_peer;
	}

	void set_peer(uint8_t idx, uint8_t *buf) {					// writes the peer to the by idx defined place in the database
		if (idx >= max) return;
		setEEPromBlock(ee_addr + (idx * 4), 4, buf);
	}

	void clear_peer(uint8_t idx) {								// clears the peer in the by idx defined place in the database
		if (idx >= max) return;									// to secure we are in the range
		clearEEPromBlock(ee_addr + (idx * 4), 4);				// clear the specific eeprom block
	}

	uint8_t get_idx(uint8_t *buf) {								// returns the idx of the given peer, or 0xff if not found. don't use the peer array of the struct, it will be overwritten!
		for (uint8_t i = 0; i < max; i++) if (!memcmp(get_peer(i), buf, 4)) return i;
		return 0xff;
	}

	uint8_t get_free_slot() {									// returns the idx of an empty peer, or 0xff if not found. don't use the peer array of the struct, it will be overwritten!
		for (uint8_t i = 0; i < max; i++) if (!*(uint32_t*)get_peer(i)) return i;
		return 0xff;
	}

	void clear_all() {											// clear all peers
		clearEEPromBlock(ee_addr, max * 4);
	}

	uint8_t used_slots() {										// returns the amount of used slots
		uint8_t retByte = 0;
		for (uint8_t i = 0; i < max; i++) if (*(uint32_t*)get_peer(i)) retByte++;
		return retByte;
	}

	uint8_t get_nr_slices(uint8_t byte_per_msg = 16) {			// calculates the amount of needed slices to send all peers depending on the given msg length in peers per message
		uint8_t slices = 0;	int16_t total = (used_slots() + 1) * 4;		
		while ((total -= byte_per_msg) > 0) slices++;
		return ++slices;
	}

	uint8_t get_slice(uint8_t slice_nr, uint8_t *buf, uint8_t byte_per_msg = 16) {	// cpoies all known peers into the given buffer, as msg length is limited we use multipe messages
		uint8_t byteCnt = 0, slcCnt = 0;											// start the byte and slice counter
		for (uint8_t i = 0; i < max; i++) {											// step through the possible peer slots
			getEEPromBlock(ee_addr + (i * 4), 4, &buf[byteCnt]);					// get the peer
			if (!*(uint32_t*)&buf[byteCnt]) continue;								// continue if peer is empty
			byteCnt += 4; 															// increase the byte counter while not empty 
			if (byteCnt >= byte_per_msg) {											// string is full
				if (slcCnt == slice_nr) goto end_get_slice;							// and we are in the right slice, return result
				byteCnt = 0; slcCnt++;												// wrong slice, next slice from beginning
			}
		}
		memset(&buf[byteCnt], 0, 4); byteCnt += 4;									// add the terminating zeros
		end_get_slice:
		return byteCnt;																// return the amount of bytes
	}
} s_peer_table;


/*
* @brief Intent of message, used for receive and send function
* Types are, valid for send or receive id
* BROADCAST  - b, messgae from or to all, indicated by 00 00 00
* MASTER     - m, message to or from HM Central, communication between paired devices
* PEER       - p, message to or from a known peer
* INTERN     - i, message to or from ourself, will not be transmitted
* LOGGING    - l, not for us, show in log only
* NOT_PAIRED - x, message addressed to us, but pair or peer not known
* ERROR      - u, don't know
*/
namespace MSG_INTENT {
	enum E : uint8_t { BROADCAST = 'b', MASTER = 'm', PEER = 'p', INTERN = 'i', LOGGING = 'l', NOT_PAIRED = 'x', ERROR = 'u', };
};


/*
* @brief Reason for sending the message
* ANSWER  - 0, messgae answers a received string
* INITIAL - 1, we are informing others
*/
namespace MSG_REASON {
	enum E : uint8_t { ANSWER = 0x00, INITIAL = 0x01,  };
};


/*
* @brief Type of message, Byte 03 translation
* by03 10 11 LEN
* -------------------------------------------
* 0x00 ff ff 1a * DEVICE_INFO
* -------------------------------------------
* 0x01 ff ff ff * CONFIG_REQ
* 0x01 ff 01 10 * - CONFIG_PEER_ADD
* 0x01 ff 02 10 * - CONFIG_PEER_REMOVE
* 0x01 ff 03 0b * - CONFIG_PEER_LIST_REQ
* 0x01 ff 04 10 * - CONFIG_PARAM_REQ
* 0x01 ff 05 10 * - CONFIG_START
* 0x01 ff 06 0a * - CONFIG_END
* 0x01 ff 07 ff * - CONFIG_WRITE_INDEX1, len unspecified
* 0x01 ff 08 ff * - CONFIG_WRITE_INDEX2, len unspecified
* 0x01 ff 09 0b * - CONFIG_SERIAL_REQ
* 0x01 ff 0a 15 * - CONFIG_PAIR_SERIAL
* 0x01 ff 0e 0b * - CONFIG_STATUS_REQUEST
* -------------------------------------------
* 0x02 ff ff ff * ACK_MSG
* 0x02 00 ff 0a * - ACK
* 0x02 01 ff 0e * - ACK_STATUS
* 0x02 02 ff 0a * - ACK2
* 0x02 04 ff 11 * - AES_REQ
* 0x02 80 ff 0a * - NACK
* 0x02 84 ff 0a * - NACK_TARGET_INVALID
* 0x02 ff ff 09 * - ACK_NACK_UNKNOWN, short string no byte 10 value
* -------------------------------------------
* 0x03 ff ff 09 * AES_REPLY
* -------------------------------------------
* 0x04 ff ff ff * - SEND_AES
* 0x04 01 ff 0c * - SEND_AES_TO_HMLAN
* 0x04 ff ff 09 * - SEND_AES_TO_ACTOR, no byte 10
* -------------------------------------------
* 0x10 ff ff ff * REPLY_MSG
* 0x10 00 ff 14 * - INFO_SERIAL
* 0x10 01 ff ff * - INFO_PEER_LIST, no fixed length
* 0x10 02 ff ff * - INFO_PARAM_RESPONSE_PAIRS, no fixed length
* 0x10 03 ff ff * - INFO_PARAM_RESPONSE_SEQ, no fixed length
* 0x10 04 ff ff * - INFO_PARAMETER_CHANGE, no fixed length
* 0x10 06 ff 0e * - INFO_ACTUATOR_STATUS
* 0x10 0A ff 0d * - INFO_TEMP
* -------------------------------------------
* 0x11 ff ff ff * INSTRUCTION_MSG = 0x11
* 0x11 00 ff 0b * - INSTRUCTION_INHIBIT_OFF
* 0x11 01 ff 0b * - INSTRUCTION_INHIBIT_ON
* 0x11 02 ff ff * - INSTRUCTION_SET
* 0x11 03 ff 0b * - INSTRUCTION_STOP_CHANGE
* 0x11 04 00 0b * - INSTRUCTION_RESET
* 0x11 80 ff 0c * - INSTRUCTION_LED
* 0x11 81 00 0f * - INSTRUCTION_LED_ALL
* 0x11 81 FF 0d * - INSTRUCTION_LEVEL
* 0x11 82 FF 0c * - INSTRUCTION_SLEEPMODE
* 0x11 83 FF 0a * - INSTRUCTION_ENTER_BOOTLOADER
* 0x11 86 FF 0c * - INSTRUCTION_SET_TEMP
* 0x11 87 FF 0a * - INSTRUCTION_ADAPTION_DRIVE_SET
* 0x11 CA FF 0a * - INSTRUCTION_ENTER_BOOTLOADER2
* -------------------------------------------
* 0x12 ff ff 09 * HAVE_DATA 
* -------------------------------------------
* 0x3e ff ff 0f * SWITCH 
* 0x3f ff ff 0c * TIMESTAMP 
* 0x40 ff ff 0b * REMOTE 
* 0x41 ff ff 0c * SENSOR_EVENT 
* 0x42 ff ff 0c * SWITCH_LEVEL 
* 0x53 ff ff 16 * SENSOR_DATA 
* 0x54 ff ff 10 * GAS_EVENT 
* 0x58 ff ff 0b * CLIMATE_EVENT 
* 0x59 ff ff 0b * SET_TEAM_TEMP
* 0x5A ff ff 0d * THERMAL_CONTROL
* 0x5E ff ff 14 * POWER_EVENT_CYCLE
* 0x5F ff ff 14 * POWER_EVENT
* 0x70 ff ff 0c * WEATHER_EVENT
* -------------------------------------------
*/
namespace MSG_TYPE {
	enum E : uint32_t {		

		/* 0x00 ff ff 1a * DEVICE_INFO
		*    LEN CNT FLAG  BY03  SND       REV       FW   ID     SERIAL                          CLASS  PCNLA  PCNLB   UNK
		* <- 1A  01  84    00    33 11 22  00 00 00  10   00 6C  48 42 73 77 69 74 63 68 30 31   00     41     01      00   */
		DEVICE_INFO = 0x00ffff1A,	


		/* 0x01 ff ff ff * CONFIG_REQ - placeholder only */
		CONFIG_REQ = 0x01ffffff,	
	
		/* 0x01 ff 01 10 * -CONFIG_PEER_ADD
		*    LEN CNT FLAG  BY03  SND       REV       CNL   BY11  PEER ADDR  PCNLA  PCNLB
		* m> 10  31  A0    01    63 19 64  33 11 22  01    01    11 22 33   01     02      */
		CONFIG_PEER_ADD = 0x01ff0110,

		/* 0x01 ff 02 10 * -CONFIG_PEER_REMOVE
		*    LEN CNT FLAG  BY03  SND       REV       CNL   BY11  PEER ADDR  PCNLA  PCNLB
		* m> 10  31  A0    01    63 19 64  33 11 22  01    02    11 22 33   01     02      */
		CONFIG_PEER_REMOVE = 0x01ff0210,

		/* 0x01 ff 03 0b * -CONFIG_PEER_LIST_REQ
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11
		* l> 0B  35  A0   01   63 19 64  23 70 D8  01   03  */
		CONFIG_PEER_LIST_REQ = 0x01ff030b,

		/* 0x01 ff 04 10 * - CONFIG_PARAM_REQ
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11  PEER ADDR  PCNL  LIST
		* l> 10  42  A0   01   63 19 64  23 70 D8  01   04    33 11 22   01    04   */
		CONFIG_PARAM_REQ = 0x01ff0410,

		/* 0x01 ff 05 10 * - CONFIG_START
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11  PEER ADDR  PCNL  LIST
		* l> 10  3F  A0   01   63 19 64  23 70 D8  01   05    33 11 22   01    04    */
		CONFIG_START = 0x01ff0510,

		/* 0x01 ff 06 0a * - CONFIG_END
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11  
		* l> 0A  41  A0   01   63 19 64  23 70 D8  01   06    */
		CONFIG_END = 0x01ff060a,

		/* 0x01 ff 07 ff * - CONFIG_WRITE_INDEX1, len unspecified
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11  ADDR  DATA
		* l> 0D  40  A0   01   63 19 64  23 70 D8  01   08    01    01   */
		CONFIG_WRITE_INDEX1 = 0x01ff07ff,

		/* 0x01 ff 08 ff * - CONFIG_WRITE_INDEX2, len unspecified
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11   DATA
		* l> 0D  40  A0   01   63 19 64  23 70 D8  01   08     01 01    */
		CONFIG_WRITE_INDEX2 = 0x01ff08ff,

		/* 0x01 ff 09 0b * - CONFIG_SERIAL_REQ                                                 // check serial request string, not clear about byte 10
		*    LEN CNT FLAG BY03 SND       RCV       BY10  BY11 
		* l> 0B  40  A0   01   63 19 64  23 70 D8  01    09       */
		CONFIG_SERIAL_REQ = 0x01ff090b,

		/* 0x01 ff 0a 15 * - PAIR_SERIAL														// check pair serial string, not clear about byte 10
		*    LEN CNT FLAG BY03 SND       RCV       BY10  BY11   Serial
		* l> 15  40  A0   01   63 19 64  23 70 D8  01    0A     01 02 03 04 05 06 07 08 09 10  */
		CONFIG_PAIR_SERIAL = 0x01ff0a15,

		/* 0x01 ff 0e 0b * - CONFIG_STATUS_REQUEST	
		*    LEN CNT FLAG BY03 SND       RCV       CNL   BY11 
		* l> 0B  40  A0   01   63 19 64  23 70 D8  01    0E     */
		CONFIG_STATUS_REQUEST = 0x01ff0e0b,


		/* 0x02 ff ff * ACK_MSG - placeholder only */
		ACK_MSG = 0x02ffffff,

		/* 0x02 00 ff 0a * - ACK
		*    LEN CNT FLAG BY03 SND       RCV       BY10   
		* m> 0A  16  80   02   63 19 64  33 11 22  00     */
		ACK = 0x0200ff0a,

		/* 0x02 01 ff 0e * - ACK_STATUS
		*    LEN CNT FLAG BY03 SND       RCV       BY10   CNL VAL DUL  RSSI
		* m> 0E  16  80   02   63 19 64  33 11 22  01     01  00  00   80   */
		ACK_STATUS = 0x0201ff0e,

		/* 0x02 02 ff 0a * - ACK2
		*    LEN CNT FLAG BY03 SND       RCV       BY10  
		* m> 0A  16  80   02   63 19 64  33 11 22  02       */
		ACK2 = 0x0202ff0a,

		/* 0x02 04 ff 11 * - AES_REQ
		*    LEN CNT FLAG BY03 SND       RCV       BY10  PARA1  PARA2  PARA3  KEYNR
		* m> 11  16  80   02   63 19 64  33 11 22  04    00 00  11 11  22 22  33   */
		AES_REQ = 0x0204ff11,

		/* 0x02 80 ff 0a * - NACK
		*    LEN CNT FLAG BY03 SND       RCV       BY10
		* m> 0A  16  80   02   63 19 64  33 11 22  80       */
		NACK = 0x0280ff0a,

		/* 0x02 84 ff 0a * - NACK_TARGET_INVALID
		*    LEN CNT FLAG BY03 SND       RCV       BY10
		* m> 0A  16  80   02   63 19 64  33 11 22  84       */
		NACK_TARGET_INVALID = 0x0284ff0a,

		/* 0x02 ff ff 09 * - ACK_NACK_UNKNOWN, short string no byte 10 value
		*    LEN CNT FLAG BY03 SND       RCV       
		* m> 09  16  80   02   63 19 64  33 11 22    */
		ACK_NACK_UNKNOWN = 0x02ffff09,



		/* 0x03 ff ff 09 * AES_REPLY
		*    LEN CNT FLAG BY03 SND       RCV
		* m> 09  16  80   03   63 19 64  33 11 22    */
		AES_REPLY = 0x03ffff09,																	// length and By10 needs check


		/* 0x04 ff ff ff * -SEND_AES, placeholder only */
		SEND_AES = 0x04ffffff,
		
		/* 0x04 01 ff 0c * -SEND_AES_TO_HMLAN
		*    LEN CNT FLAG BY03 SND       RCV       BY10  CNL TYPE
		* m> 0C  16  A0   04   63 19 64  33 11 22  01    00  00   */
		SEND_AES_TO_HMLAN = 0x0401ff0c,

		/* 0x04 ff ff 09 * -SEND_AES_TO_ACTOR, no byte 10?
		*    LEN CNT FLAG BY03 SND       RCV
		* m> 09  16  A0   04   63 19 64  33 11 22    */
		SEND_AES_TO_ACTOR = 0x04ffff09,															// length and byte 10 needs to be checked


		/* 0x10 ff ff ff * REPLY_MSG, placeholder only */
		REPLY_MSG = 0x10ffff,

		/* 0x10 00 ff 14 * - INFO_SERIAL
		*    LEN CNT FLAG BY03 SND       RCV       By10  SERIAL
		* m> 14  16  A0   10   63 19 64  33 11 22  00    01 02 03 04 05 06 07 08 09 10  */
		INFO_SERIAL = 0x1000ff14,
		
		/* 0x10 01 ff ff * - INFO_PEER_LIST, no fix length
		*    LEN CNT FLAG BY03 SND       RCV       By10  PEER 1 to 4, empty peer terminates message
		* l> 0E  35  A0   10   23 70 D8  63 19 64  01    00 00 00 00   */
		INFO_PEER_LIST = 0x1001ffff,
		
		/* 0x10 02 ff ff * - INFO_PARAM_RESPONSE_PAIRS, no fix length
		*    LEN CNT FLAG BY03 SND       RCV       By10  Data, '00 00' terminates message
		* l> 0E  42  A0   10   23 70 D8  63 19 64  02    01 01 00 00    */
		INFO_PARAM_RESPONSE_PAIRS = 0x1002ffff,
		
		/* 0x10 03 ff ff * - INFO_PARAM_RESPONSE_SEQ, no fix length
		*    LEN CNT FLAG BY03 SND       RCV       By10  OFFSET  Data, '00 00' terminates message
		* l> 0E  42  A0   10   23 70 D8  63 19 64  03    01      01 00 00    */
		INFO_PARAM_RESPONSE_SEQ = 0x1003ffff,
	
		/* 0x10 04 ff ff * - INFO_PARAMETER_CHANGE, no fix length
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  PEER         LIST  DATA
		* l> 15  42  A0   10   23 70 D8  63 19 64  04    01   11 22 33 01  04    11 22 33 00 00   */
		INFO_PARAMETER_CHANGE = 0x1004ffff,
		
		/* 0x10 06 ff 0e * - INFO_ACTUATOR_STATUS
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  STATUS  UNKNOWN  RSSI
		* l> 0E  42  A0   10   23 70 D8  63 19 64  06    01   00      00       80  */
		INFO_ACTUATOR_STATUS = 0x1006ff0e,
		
		/* 0x10 0A ff 0d * - INFO_TEMP
		*    LEN CNT FLAG BY03 SND       RCV       By10  SET/ACT  ERR/VALVE/MODE
		* l> 0D  42  A0   10   23 70 D8  63 19 64  0A    01 00    00             */
		INFO_TEMP = 0x100Aff0d,

			
		/* 0x11 ff ff ff * INSTRUCTION_MSG = 0x11, placeholder only */	
		INSTRUCTION_MSG = 0x11ffff,

		/* 0x11 00 ff 0b * -INSTRUCTION_INHIBIT_OFF
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
		* l> 0B  42  A0   11   23 70 D8  63 19 64  00    01    */
		INSTRUCTION_INHIBIT_OFF = 0x1100ff0b,

		/* 0x11 01 ff 0b * -INSTRUCTION_INHIBIT_ON
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
		* l> 0B  42  A0   11   23 70 D8  63 19 64  01    01    */
		INSTRUCTION_INHIBIT_ON = 0x1101ff0b,

		/* 0x11 02 ff ff * - INSTRUCTION_SET, ramp and dura time optional
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  VALUE  RAMPTIME  DURATIME
		* l> 10  42  A0   11   23 70 D8  63 19 64  02    01   FF     01 00     01 00     */
		INSTRUCTION_SET = 0x1102ffff,

		/* 0x11 03 ff 0b * - INSTRUCTION_STOP_CHANGE
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL 
		* l> 0B  42  A0   11   23 70 D8  63 19 64  03    01     */
		INSTRUCTION_STOP_CHANGE = 0x1103ff0b,

		/* 0x11 04 00 0b * - INSTRUCTION_RESET
		*    LEN CNT FLAG BY03 SND       RCV       By10  BY11
		* l> 0B  42  A0   11   23 70 D8  63 19 64  04    00     */
		INSTRUCTION_RESET = 0x1104000b,

		/* 0x11 80 ff 0c * - INSTRUCTION_LED
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  COLOR
		* l> 0C  42  A0   11   23 70 D8  63 19 64  80    01   00     */
		INSTRUCTION_LED = 0x1180ff0c,

		/* 0x11 81 00 0f * - INSTRUCTION_LED_ALL
		*    LEN CNT FLAG BY03 SND       RCV       By10  BY11  LED 1 -16
		* l> 0F  42  A0   11   23 70 D8  63 19 64  81    00    01 02 03 04   */
		INSTRUCTION_LED_ALL = 0x1181000f,

		/* 0x11 81 FF 0d * - INSTRUCTION_LEVEL
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  TIME  SPEED
		* l> 0D  42  A0   11   23 70 D8  63 19 64  81    01   15    3f      */
		INSTRUCTION_LEVEL = 0x1181ff0d,

		/* 0x11 82 FF 0c * - INSTRUCTION_SLEEPMODE
		*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  MODE
		* l> 0C  42  A0   11   23 70 D8  63 19 64  82    01   00      */
		INSTRUCTION_SLEEPMODE = 0x1182ff0c,

		/* 0x11 83 FF 0a * - INSTRUCTION_ENTER_BOOTLOADER
		*    LEN CNT FLAG BY03 SND       RCV       By10 
		* l> 0A  42  A0   11   23 70 D8  63 19 64  83       */
		INSTRUCTION_ENTER_BOOTLOADER = 0x1183ff0a,

		/* 0x11 86 FF 0c * - INSTRUCTION_SET_TEMP
		*    LEN CNT FLAG BY03 SND       RCV       By10  B1  B2
		* l> 0C  42  A0   11   23 70 D8  63 19 64  86    00  00      */
		INSTRUCTION_SET_TEMP = 0x1186ff0c,

		/* 0x11 87 FF 0a * - INSTRUCTION_ADAPTION_DRIVE_SET
		*    LEN CNT FLAG BY03 SND       RCV       By10  
		* l> 0A  42  A0   11   23 70 D8  63 19 64  87     */
		INSTRUCTION_ADAPTION_DRIVE_SET = 0x1187ff0a,

		/* 0x11 CA FF 0a * - INSTRUCTION_ENTER_BOOTLOADER2
		*    LEN CNT FLAG BY03 SND       RCV       By10  
		* l> 0A  42  A0   11   23 70 D8  63 19 64  CA       */
		INSTRUCTION_ENTER_BOOTLOADER2 = 0x11caff0a,
			
		
		/* 0x12 ff ff 09 * HAVE_DATA = 0x12
		*    LEN CNT FLAG BY03 SND       RCV      
		* l> 09  42  A0   12   23 70 D8  63 19 64   */
		HAVE_DATA = 0x12ffff09,

		
		/* 0x3e ff ff 0f * SWITCH = 0x3e
		*    LEN CNT FLAG BY03 SND       RCV        DST       UNKNOWN  CNL  COUNTER
		* l> 0F  42  A0   3E   23 70 D8  63 19 64   11 22 33  00       01   17   */
		SWITCH = 0x3effff0f,

		/* 0x3f ff ff 0c * TIMESTAMP = 0x3f
		*    LEN CNT FLAG BY03 SND       RCV        UNKNOWN  TIME
		* l> 0C  42  A0   3F   23 70 D8  63 19 64   11 22    00    */
		TIMESTAMP = 0x3fffff0c,

		/* 0x40 ff ff 0b * REMOTE = 0x40
		*    LEN CNT FLAG BY03 SND       RCV        BLL  COUNTER
		* l> 0B  42  A0   40   23 70 D8  63 19 64   41   22     */
		REMOTE = 0x40ffff0b,

		/* 0x41 ff ff 0c * SENSOR_EVENT = 0x41
		*    LEN CNT FLAG BY03 SND       RCV        BLL  NBR  COUNTER
		* l> 0C  42  A0   41   23 70 D8  63 19 64   41   50   22     */
		SENSOR_EVENT = 0x41ffff0c,

		/* 0x42 ff ff 0c * SWITCH_LEVEL = 0x42
		*    LEN CNT FLAG BY03 SND       RCV        BUTTON  NBR  LEVEL
		* l> 0C  42  A0   42   23 70 D8  63 19 64   01      50   22     */
		SWITCH_LEVEL = 0x42ffff0c,

		/* 0x53 ff ff 16 * SENSOR_DATA = 0x53
		*    LEN CNT FLAG BY03 SND       RCV        CMD  FLD1 VAL1   FLD2 VAL2   FLD3 VAL3   FLD4 VAL4
		* l> 16  42  A0   53   23 70 D8  63 19 64   01   01   00 00  02   00 00  03   00 00  04   00 00   */
		SENSOR_DATA = 0x53ffff16,
		
		/* 0x54 ff ff 10 * GAS_EVENT 
		*    LEN CNT FLAG BY03 SND       RCV        ENERGY       POWER
		* l> 10  42  A0   54   23 70 D8  63 19 64   01 02 03 04  05 06 07   */
		GAS_EVENT = 0x54ffff10,
		
		/* 0x58 ff ff 0b * CLIMATE_EVENT
		*    LEN CNT FLAG BY03 SND       RCV        CMD  VALVE
		* l> 0B  42  A0   58   23 70 D8  63 19 64   01   00      */
		CLIMATE_EVENT = 0x58ffff0b,
		
		/* 0x59 ff ff 0b * SET_TEAM_TEMP
		*    LEN CNT FLAG BY03 SND       RCV        CMD  TEMP/MODE
		* l> 0B  42  A0   59   23 70 D8  63 19 64   01   00      */
		SET_TEAM_TEMP = 0x59ffff0b,

		/* 0x5a ff ff 0d * THERMAL_CONTROL
		*    LEN CNT FLAG BY03 SND       RCV        SETTEMP  ACTTEMP  HUM
		* l> 0D  42  A0   5A   23 70 D8  63 19 64   01       02 00    50    */
		THERMAL_CONTROL = 0x5affff0d,

		/* 0x5e ff ff 14 * POWER_EVENT_CYCLE
		*    LEN CNT FLAG BY03 SND       RCV        ENERGY    POWER     CURRENT  VOLTAGE  FREQUENCY
		* l> 14  42  A0   5E   23 70 D8  63 19 64   01 00 00  02 00 00  03 00    04 00    50     */
		POWER_EVENT_CYCLE = 0x5effff14,
		
		/* 0x5f ff ff 14 * POWER_EVENT
		*    LEN CNT FLAG BY03 SND       RCV        ENERGY    POWER     CURRENT  VOLTAGE  FREQUENCY
		* l> 14  42  A0   5F   23 70 D8  63 19 64   01 00 00  02 00 00  03 00    04 00    50     */
		POWER_EVENT = 0x5fffff14,

		/* 0x70 ff ff 0c * WEATHER_EVENT
		*    LEN CNT FLAG BY03 SND       RCV        TEMP   HUM
		* l> 0C  42  A0   70   23 70 D8  63 19 64   01 00  00      */
		WEATHER_EVENT = 0x70ffff0c,
	};
}


/*
* @brief MESSAGE_FLAGS, byte 02 of every message
* 0x01: send initially to keep the device awake
* 0x02: awake - hurry up to send messages
* 0x04: Device in Config mode
* 0x08: unknown
* 0x10: set if burst is required by device
* 0x20: response is expected
* 0x40: repeated (repeater operation)
* 0x80: set in every message. Meaning?
*/
struct s_mFlg {
	uint8_t WKUP   : 1;						// 0x01: send initially to keep the device awake
	uint8_t WKMEUP : 1;						// 0x02: awake - hurry up to send messages
	uint8_t CFG    : 1;						// 0x04: Device in Config mode
	uint8_t        : 1;
	uint8_t BURST  : 1;						// 0x10: set if burst is required by device
	uint8_t BIDI   : 1;						// 0x20: response is expected
	uint8_t RPTED  : 1;						// 0x40: repeated (repeater operation)
	uint8_t RPTEN  : 1;						// 0x80: set in every message. Meaning?
}; // MESSAGE_FLAGS


/*
* @brief Union which holds the received byte string for easier processing
* 03, 10, 11, message, struct name
*
* xx, xx, xx, GENERIC, msgBody
*
* 00, xx, xx, DEVICE_INFO, msg00xxxx
*
* 01, xx, 01, CONFIG_PEER_ADD, msg01xx01
* 01, xx, 02, CONFIG_PEER_REMOVE, msg01xx02
* 01, xx, 03, CONFIG_PEER_LIST_REQ, msg01xx03
* 01, xx, 04, CONFIG_PARAM_REQ, msg01xx04
* 01, xx, 05, CONFIG_START, msg01xx05
* 01, xx, 06, CONFIG_END, msg01xx06
* 01, xx, 07, CONFIG_WRITE_INDEX, msg01xx07
* 01, xx, 08, CONFIG_WRITE_INDEX, msg01xx08
* 01, xx, 09, CONFIG_SERIAL_REQ, msg01xx09
* 01, xx, 0a, PAIR_SERIAL, msg01xx0a
* 01, xx, 0e, CONFIG_STATUS_REQUEST, msg01xx0e
*
* 02, 00, xx, ACK, msg0200xx
* 02, 01, xx, ACK_STATUS, msg0201xx
* 02, 02, xx, ACK2, msg0202xx
* 02, 04, xx, ACK_PROC, msg0204xx
* 02, 80, xx, NACK, msg0280xx
* 02, 84, xx, NACK_TARGET_INVALID, msg0284xx
* 02, xx, xx, ACK_NACK_UNKNOWN, msg02xxxx
* 02, xx, xx, REQUEST_AES, msg02xxxx
*
* 3E, xx, xx, SWITCH, msg3Exxxx
*/

/*
* @brief GENERIC message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, BY11 - message type
* byte 12, PAYLOAD - payload of message
*/
typedef struct ts_msgBody {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - type of message
	uint8_t       BY11;					// by10 - type of message
	uint8_t       PAYLOAD[MaxDataLen - 12];// payload
} s_mBody; // GENERIC message


/*
* @brief DEVICE_INFO message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, FIRMWARE - firmware version
* byte 11, DEV_TYPE[2] - device type
* byte 13, SERIALNO[10] - 10 byte serial number of device
* byte 23, CLASS - ???
* byte 24, PEER_CNL_A, peer channel 1
* byte 25, PEER_CNL_B, peer channel 2
* byte 26, unknown
*/
typedef struct ts_msg00xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       FIRMWARE;				// by10 - firmware version
	uint8_t       DEV_TYPE[2];			// by11 - device type
	uint8_t       SERIALNO[10];			// by13 - 10 byte serial number of device
	uint8_t       CLASS;				// by23 - ??
	uint8_t       PEER_CNL_A;			// by24 - peer channel 1
	uint8_t       PEER_CNL_B;			// by25 - peer channel 2
	uint8_t : 8;						// by26 - unknown
} s_m00xxxx; // DEVICE_INFO message


/*
* @brief CONFIG_PEER_ADD message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, PEER_ID - by12,13,14 - peer address
* byte 15, PEER_CNL[2] - by15,16 - peer channel A and B
*/
typedef struct ts_msg01xx01 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t	      PEER_ID[3];			// by12 - peer address
	uint8_t       PEER_CNL[2];			// by15, by16 - peer channel A and B
} s_m01xx01; // CONFIG_PEER_ADD message

/*
* @brief CONFIG_PEER_REMOVE message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, PEER_ID - by12,13,14 - peer address
* byte 15, PEER_CNL[2] - by15,16 - peer channel A and B
*/
typedef struct ts_msg01xx02 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t	      PEER_ID[3];			// by12 - peer address
	uint8_t       PEER_CNL[2];			// by15, by16 - peer channel A and B
} s_m01xx02; // CONFIG_PEER_REMOVE message

/*
* @brief CONFIG_PEER_LIST_REQ message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
*/
typedef struct ts_msg01xx03 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
} s_m01xx03; // CONFIG_PEER_LIST_REQ message

/*
* @brief CONFIG_PARAM_REQ message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, PEER_ID - peer address
* byte 15, PEER_CNL - peer channel
* byte 16, PARAM_LIST - parameter list 0, 1, 3, 4, etc.
*/
typedef struct ts_msg01xx04 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       PEER_ID[3];			// by12 - peer address
	uint8_t       PEER_CNL;				// by15 - peer channel
	uint8_t       PARAM_LIST;			// by16 - parameter list 0, 1, 3, 4, etc.
} s_m01xx04; // CONFIG_PARAM_REQ message

/*
* @brief CONFIG_START message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, PEER_ID - peer address
* byte 15, PEER_CNL - peer channel
* byte 16, PARAM_LIST - parameter list 0, 1, 3, 4, etc.
*/
typedef struct ts_msg01xx05 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       PEER_ID[3];			// by12 - peer address
	uint8_t       PEER_CNL;				// by15 - peer channel
	uint8_t       PARAM_LIST;			// by16 - parameter list 0, 1, 3, 4, etc.
} s_m01xx05; // CONFIG_START message

/*
* @brief CONFIG_END message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
*/
typedef struct ts_msg01xx06 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
} s_m01xx06; // CONFIG_END message

/*
* @brief CONFIG_WRITE_INDEX message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, ADDR - address for data
* byte 13, DATA[MaxDataLen - 13] - data, start register in address, remaining data
*/
typedef struct ts_msg01xx07 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       ADDR;					// by12 - address for data
	uint8_t       DATA[MaxDataLen - 13];// by13 - data, start register in address, remaining data
} s_m01xx07; // CONFIG_WRITE_INDEX message

/*
* @brief CONFIG_WRITE_INDEX message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 13, DATA[MaxDataLen - 12] - data, 1 byte register, 1 byte data, ...
*/
typedef struct ts_msg01xx08 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       DATA[MaxDataLen - 12];// by12 - data, 1 byte register, 1 byte data, ...
} s_m01xx08; // CONFIG_WRITE_INDEX message

/*
* @brief CONFIG_SERIAL_REQ message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 13, DATA[MaxDataLen - 12] - data, 1 byte register, 1 byte data, ...
*/
typedef struct ts_msg01xx09 {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       DATA[MaxDataLen - 12];// by12 - data, 1 byte register, 1 byte data, ...
} s_m01xx09; // CONFIG_SERIAL_REQ message

/*
* @brief CONFIG_PAIR_SERIAL message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
* byte 12, SERIALNO[10] - serial number of device
*/
typedef struct ts_msg01xx0a {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
	uint8_t       SERIALNO[10];			// by12 - 10 byte serial number
} s_m01xx0a; // CONFIG_PAIR_SERIAL message

/*
* @brief CONFIG_STATUS_REQUEST message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, MSG_CNL - message channel
* byte 11, BY11 - message type
*/
typedef struct ts_msg01xx0e {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;				// by10 - message channel
	uint8_t       BY11;					// by11 - message type
} s_m01xx0e; // CONFIG_STATUS_REQUEST message


/*
* @brief ACK message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*/
typedef struct ts_msg0200xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m0200xx; // ACK message

/*
* @brief ACK_STATUS message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, MSG_CNL - message channel
* byte 12, MSG_STATUS - message status
* byte 13, MSG_FLAG - message flags, up, down, lowbat
* byte 14, MSG_RSSI - RSSI recording of device
*/
typedef struct ts_msg0201xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message channel
	uint8_t       MSG_CNL;				// by11 - message channel
	uint8_t       MSG_STATUS;			// by12 - message status
	struct s_DUL {
		uint8_t         : 4;
		uint8_t	 UP     : 1;			// & 0x10
		uint8_t  DOWN   : 1;			// & 0x20
		uint8_t	        : 1;			// & 0x40
		uint8_t	 LOWBAT : 1;			// & 0x80
	} MSG_FLAG;							// by13 - message flags
	uint8_t      MSG_RSSI;				// by14 - recording of device
} s_m0201xx; // ACK_STATUS message

/*
* @brief ACK2 message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*/
typedef struct ts_msg0202xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message channel
} s_m0202xx; // ACK2 message

 /*
	* 02, 04, xx, ACK_PROC, msg0204xx
	* 02, 80, xx, NACK, msg0280xx
	* 02, 84, xx, NACK_TARGET_INVALID, msg0284xx
	* 02, xx, xx, ACK_NACK_UNKNOWN, msg02xxxx
	* 02, xx, xx, REQUEST_AES, msg02xxxx

	"02;p01=04" = > { txt = > "ACK-proc", params = > {# connected to AES ? ?
		Para1 = > "02,4",
		Para2 = > "06,4",
		Para3 = > "10,4",
		Para4 = > "14,2", }}, # remote ?
	"02;p01=80" = > { txt = > "NACK"},
	"02;p01=84" = > { txt = > "NACK_TARGET_INVALID"},
	"02" = > { txt = > "ACK/NACK_UNKNOWN   "},

	"02" = > { txt = > "Request AES", params = > {  #todo check data
	DATA = >  "0," } },*/



/*
* @brief SWITCH message
* seen as the test button in HM config soft
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, PEER[3] - peer address registered for action
* byte 13, NA - unknown
* byte 14, P_CNL - peer channel
* byte 15, P_CNT - peer count
*/
typedef struct ts_msg3Exxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       PEER[3];				// by10 - peer address registered for action
	uint8_t       NA;					// by13 - unknown
	uint8_t       P_CNL;				// by14 - peer channel
	uint8_t       P_CNT;				// by15 - peer count
} s_m3Exxxx; // SWITCH message




/*
* @brief Struct to hold the buffer for any received string with some flags for further processing
*
* hasdata      - flag that something is to process
* use_prev_buf - flag to store prev_buf is used
*
* buf[]        - initial buffer for received and decoded message string
* prev_buf[27] - store the last receive message to verify with AES signed data.
* *mBody       - struct on buffer for easier data access
*
* intent       - remember the intent of the message, filled by receive class
* peer[4]      - peer is stored as a 4 byte array, but most messages delivers it with a seperate channel field (byte 10)
* cnl          - by getting the intent the peer is checked, here we are store the channel where the peer is registered in
* prev_MSG_CNT - remember the last message counter to check if it is a repeated message
*
* clear()      - function to reset flags
*/
typedef struct ts_recv {
	uint8_t hasdata;					// flag that something is to process
	uint8_t use_prev_buf;				// flag to store prev_buf is used

	uint8_t buf[MaxDataLen];			// initial buffer for received and decoded message string
	uint8_t prev_buf[27];				// store the last receive message to verify with AES signed data.
	s_mBody *mBody = (s_mBody*)buf;		// struct on buffer for easier data access

	MSG_INTENT::E intent;				// remember the intent of the message, filled by receive class
	uint8_t peer[4];					// peer is stored as a 4 byte array, but most messages delivers it with a seperate channel field (byte 10)
	uint8_t cnl;						// by getting the intent the peer is checked, here we are store the channel where the peer is registered in
	uint8_t prev_MSG_CNT;				// remember the last message counter to check if it is a repeated message

	void clear() {						// function to reset flags
		hasdata = 0;
		buf[0] = 0;
	}
} s_recv;

/*
* @brief Struct to hold the buffer for any string to send with some flags for further processing
*
* hasdata      - flag that something is to process
*
* buf[]        - initial buffer for received and decoded message string
* *mBody       - struct on buffer for easier data access
*/
typedef struct ts_send {
	uint8_t   active;					// flag that something is to process

	uint8_t   buf[MaxDataLen];			// initial buffer for received and decoded message string
	uint8_t   prev_buf[32];				// store the last receive message to verify with AES signed data.
	s_mBody   *mBody = (s_mBody*)buf;	// struct on buffer for easier data access

	uint8_t   timeout;					// was last message a timeout
	uint8_t   retr_cnt;					// variable to count how often a message was already send
	uint8_t   temp_max_retr;			// is set depending on the BIDI flag and the max_retr value
	uint8_t   temp_MSG_CNT;				// store for message counter, needed to identify ACK

	uint8_t   MSG_CNT;					// message counter for initial sends
	uint8_t   max_retr;					// how often a message has to be send until ACK - info is set by cmMaintenance
	uint16_t  max_time;					// max time for message timeout timer - info is set by  cmMaintenance
	waitTimer timer;					// send mode timeout

	void clear() {						// function to reset flags
		active = 0;
		buf[0] = 0;
		timeout = 0;
		retr_cnt = 0;
		temp_max_retr = 0;
		timer.set(0);
	}
} s_send;




/*"3F" = > { txt = > "TimeStamp", params = > {
UNKNOWN = > "00,4",
TIME = > "04,2", } },
"40" = > { txt = > "REMOTE", params = > {
BUTTON = > '00,2,$val=(hex($val)&0x3F)',
LONG = > '00,2,$val=(hex($val)&0x40)?1:0',
LOWBAT = > '00,2,$val=(hex($val)&0x80)?1:0',
COUNTER = > "02,2", } },
"41" = > { txt = > "Sensor_event", params = > {
BUTTON = > '00,2,$val=(hex($val)&0x3F)',
LONG = > '00,2,$val=(hex($val)&0x40)?1:0',
LOWBAT = > '00,2,$val=(hex($val)&0x80)?1:0',
NBR = > '02,2,$val=(hex($val))',
VALUE = > '04,2,$val=(hex($val))', } },
"53" = > { txt = > "SensorData", params = > {
CMD = > "00,2",
Fld1 = > "02,2",
Val1 = > '04,4,$val=(hex($val))',
Fld2 = > "08,2",
Val2 = > '10,4,$val=(hex($val))',
Fld3 = > "14,2",
Val3 = > '16,4,$val=(hex($val))',
Fld4 = > "20,2",
Val4 = > '24,4,$val=(hex($val))'} },
"58" = > { txt = > "ClimateEvent", params = > {
CMD = > "00,2",
ValvePos = > '02,2,$val=(hex($val))', } },
"59" = > { txt = > "setTeamTemp", params = > {
CMD = > "00,2",
desTemp = > '02,2,$val=((hex($val)>>2) /2)',
mode = > '02,2,$val=(hex($val) & 0x3)', } },
"70" = > { txt = > "WeatherEvent", params = > {
TEMP = > '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
HUM = > '04,2,$val=(hex($val))', } },
*/





#endif