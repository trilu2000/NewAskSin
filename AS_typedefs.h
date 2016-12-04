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
* @brief Helper struct to remember on the pairing mode status in asksin main function
*/
typedef struct ts_pair_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	waitTimer timer;						// pairing mode timeout
} s_pair_mode;


/* 
* @brief First bytes of eeprom holds all device specific information for identification
*/
typedef struct ts_dev_ident {
	uint16_t MAGIC;							//  2 byte - magic byte
	uint8_t  HMID[3];						//  3 byte - homematic id
	uint8_t  SERIAL_NR[10];					// 10 byte - serial number
	uint8_t  HMKEY_INDEX;					//  1 byte - aes key index
	uint8_t  HMKEY[16];						// 16 byte - homematic aes key
} s_dev_ident;

/*
* @brief Holds all variables or pointers which are needed to operate a device 
*/
typedef struct ts_dev_operate {
	uint8_t  *MAID;							// pointer to the master id, which is hold in cmMaintenance
	uint8_t  *AES_FLAG;						// pointer to aes flag, handled in cmMaintenance
} s_dev_operate;

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

	/* calculate the eeprom address on base of the index, list0 / 1 didnt need an index */
	uint16_t get_ee_addr(uint8_t idx = 0) {	
		return ee_addr + (idx * len);
	}

	/* load the respective list from the eeprom  */
	void load_list(uint8_t idx = 0) {							
		getEEPromBlock(get_ee_addr(idx), len, val);
	}

	/* writes the respective list to the eeprom  */
	void save_list(uint8_t idx = 0) {
		setEEPromBlock(get_ee_addr(idx), len, val);
	}

	/* load defaults from PROGMEM  */
	void load_default() {
		memcpy_P(val, def, len);
	}

	/* Returns a pointer to the array where we load PROGMEM defaults, or eeprom stored content.
	*  The function searches for a given register value and returns the pointer to the specic address in 
	*  the value array. If a register address is not found, the pointer shows to an empty byte, defined within this function.
	*  This function can also be used to update register content, but you have to load and save the value array
	*  by calling load_defaults(), load_list() or save_list()
	*/
	uint8_t* ptr_to_val(uint8_t reg_addr) {	
		uint8_t *pos_in_reg = (uint8_t*)memchr_P(reg, reg_addr, len);
		if (!pos_in_reg) return NULL;
		return (val + (pos_in_reg - reg));
	}

	/* Writes values by a given register/value array into the local value array. 
	*  Respective list is loaded and saved by this function.
	*/
	void write_array(uint8_t *buf, uint8_t len, uint8_t idx = 0) {
		load_list(idx);
		for (uint8_t i = 0; i < len; i += 2) {
			uint8_t *ptr = ptr_to_val(buf[i]);
			if (ptr) *ptr = buf[i + 1];
		}
		save_list(idx);
	}

	/* Calculates the amount of needed slices to send all peers depending on the given 
	*  msg length in peers per message
	*/
	uint8_t get_nr_slices_pairs(uint8_t byte_per_msg = 16) {
		uint8_t slices = 0;	int16_t total = (len * 2);			// len of register + len of value
		while ((total -= byte_per_msg) > 0) slices++;
		return ++slices;
	}

	uint8_t get_slice_pairs(uint8_t idx, uint8_t slc, uint8_t *buf, uint8_t byte_per_msg = 16) { // returns a sliced peer list
		load_list(idx);											// load the eeprom content by idx into the value table
		byte_per_msg /= 2;										// divided by 2 while we mix two arrays
		uint8_t slc_start = slc * byte_per_msg;					// calculate the start point for reg and val
		uint8_t slc_end = slc_start + byte_per_msg;				// calculate the corresponding end point
		if (slc_end > len) slc_end = len;						// if calculated end point is bigger than the physical

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
	uint8_t  max;												// maximum number of peer devices
	uint16_t ee_addr;											// address of configuration data in EEprom memory
	uint8_t  dont_use_peer[4];									// placeholder for a peer id from or to eeprom

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
		if (!*(uint32_t*)buf) return 0;							// for list0/1 requests the peer address is empty
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
		uint8_t byteCnt = 0, slcCnt = 0;						// start the byte and slice counter
		for (uint8_t i = 0; i < max; i++) {						// step through the possible peer slots
			getEEPromBlock(ee_addr + (i * 4), 4, &buf[byteCnt]);// get the peer
			if (!*(uint32_t*)&buf[byteCnt]) continue;			// continue if peer is empty
			byteCnt += 4; 										// increase the byte counter while not empty 
			if (byteCnt >= byte_per_msg) {						// string is full
				if (slcCnt == slice_nr) goto end_get_slice;		// and we are in the right slice, return result
				byteCnt = 0; slcCnt++;							// wrong slice, next slice from beginning
			}
		}
		memset(&buf[byteCnt], 0, 4); byteCnt += 4;				// add the terminating zeros
	end_get_slice:
		return byteCnt;											// return the amount of bytes
	}
} s_peer_table;

/*
* @brief Helper struct to remember on the config mode status in asksin main function
*/
typedef struct ts_config_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	uint8_t   idx_peer;						// and the peer index
	s_list_table *list;						// pointer to the respective list
	waitTimer timer;						// config mode timeout
} s_config_mode;


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
* @brief active flag for send message to define how the message has to be prepared
* NONE      - 0, nothing to do
* FORWARD   - 1, send string is prepared, process the message
* INTERN    - 2, internal string, no ack necassary
* ANSWER    - 3, take message counter and snd_id from the received string, ack not needed
* BROADCAST - 4, device initiated messages only, ack not required
* PAIR      - 5, device initiated messages only, message counter from send function, ack required
* PEER      - 6, device initiated message to all known peers of a channel, message counter from send function, ack required
*/
namespace MSG_ACTIVE {
	enum E : uint8_t { NONE = 0, FORWARD = 1, DEBUG = 2, ANSWER = 3, PAIR = 4, PEER = 5, PEER_BIDI = 6, };
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
* 0x04 ff ff 19 * - SEND_AES_TO_ACTOR, no byte 10
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
		* <- 10  81  B0    01    63 19 64  1F B7 4A  01    01    11 22 32   01     02  (3801254)
		* l> 0A 81 80 02 1F B7 4A 63 19 64 00  (3801374) was successful
		* l> 0A 81 80 02 1F B7 4A 63 19 64 82  (3801374) not enoug space  */
		CONFIG_PEER_ADD = 0x01ff0110,

		/* 0x01 ff 02 10 * -CONFIG_PEER_REMOVE
		*    LEN CNT FLAG  BY03  SND       REV       CNL   BY11  PEER ADDR  PCNLA  PCNLB
		* <- 10  31  B0    01    63 19 64  1F B7 4A  01    02    11 22 33   01     02  (769098)
		* l> 0A 31 80 02 1F B7 4A 63 19 64 00  (769218) was successful
		* l> 0A 31 80 02 1F B7 4A 63 19 64 84  (769218) peer not found    */
		CONFIG_PEER_REMOVE = 0x01ff0210,

		/* 0x01 ff 03 0b * -CONFIG_PEER_LIST_REQ
		*    LEN CNT FLAG BY03 SND       RCV       CNL  BY11
		* <- 0B  90  B0   01   63 19 64  1F B7 4A  01   03  (515786)
		* l> 1A 90 A0 10 1F B7 4A 63 19 64 01 63 19 64 01 11 22 33 01 11 22 33 02 11 22 01 01  (515927)
		* l> 1A 91 A0 10 1F B7 4A 63 19 64 01 11 22 2F 02 11 22 30 01 11 22 30 02 00 00 00 00  (516177)  */
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
		* <- 0B  41  B0   01   63 19 64  1F B7 4A  01    09  (2329621)
		* l> 14  41  80   10   1F B7 4A  63 19 64  00 4B 45 51 30 32 33 37 33 39 36  (2329754) */
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
		INSTRUCTION_MSG = 0x11ffffff,

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
* @brief Message struct typedef 
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
	uint8_t	      UNKNOWN;				// by26 - unknown
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
	uint8_t       PEER_ID[4];			// by12 - peer address, incl peer cnl
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
	uint8_t       PEER_ID[4];			// by12 - peer address
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
* @brief AES_REQ message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, PARA1[2] - AES parameter 1
* byte 13, PARA2[2]- AES parameter 2
* byte 15, PARA3[2] - AES parameter 3
* byte 17, KEYNR - Key number
*/
typedef struct ts_msg0204xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message sub type
	uint8_t       PARA1[2];				// by11 - AES parameter 1
	uint8_t       PARA2[2];				// by13 - AES parameter 2
	uint8_t       PARA3[2];				// by15 - AES parameter 3
	uint8_t       KEYNR;				// by17 - Key number
} s_m0204xx; // AES_REQ message
/*
* @brief NACK message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*/
typedef struct ts_msg0280xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m0280xx; // NACK message
/*
* @brief NACK_TARGET_INVALID message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*/
typedef struct ts_msg0284xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m0284xx; // NACK_TARGET_INVALID message
/*
* @brief ACK_NACK_UNKNOWN message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
*/
typedef struct ts_msg02xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
} s_m02xxxx; // ACK_NACK_UNKNOWN message

/*
* @brief AES_REPLY message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
*    LEN CNT FLAG BY03 SND       RCV
* m> 09  16  80   03   63 19 64  33 11 22   
*/
typedef struct ts_msg03xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
} s_m03xxxx; // AES_REPLY message

/*
* @brief SEND_AES_TO_HMLAN message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, CNL - required channel
* byte 11, TYPE - type
*    LEN CNT FLAG BY03 SND       RCV       BY10  CNL TYPE
* m> 0C  16  A0   04   63 19 64  33 11 22  01    00  00   
*/
typedef struct ts_msg0401xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       CNL;					// by10 - required channel
	uint8_t       TYPE;					// by11 - type
} s_m0401xx; // SEND_AES_TO_HMLAN message
/*
* @brief SEND_AES_TO_ACTOR message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, CNL - required channel
* byte 11, TYPE - type
*    LEN CNT FLAG BY03 SND       RCV
* m> 09  16  A0   04   63 19 64  33 11 22    
*/
typedef struct ts_msg04xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
} s_m04xxxx; // SEND_AES_TO_ACTOR message

/*
* @brief INFO_SERIAL
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, SERIALNO[10] - serial number;
*/
typedef struct ts_msg1000xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       SERIALNO[10];			// by11 - serial number
} s_m1000xx; // INFO_SERIAL message
/*
* @brief INFO_PEER_LIST
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, PEER1[4] - 1st peer;
* byte 15, PEER2[4] - 2nd peer;
* byte 19, PEER3[4] - 3rd peer;
* byte 23, PEER4[4] - 4th peer;
*/
typedef struct ts_msg1001xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       PEER1[4];				// by11 - 1st peer
	uint8_t       PEER2[4];				// by15 - 2nd peer
	uint8_t       PEER3[4];				// by19 - 3rd peer
	uint8_t       PEER4[4];				// by23 - 4th peer
} s_m1001xx; // INFO_PEER_LIST message
/*
* @brief INFO_PARAM_RESPONSE_PAIRS
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, DATA[] - data (address:value)
*/
typedef struct ts_msg1002xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       DATA[MaxDataLen - 11];// by11 - data
} s_m1002xx; // INFO_PARAM_RESPONSE_PAIRS message
/*
* @brief INFO_PARAM_RESPONSE_SEQ
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, OFFSET - offset, start address
* byte 12, DATA[] - data value array
*/
typedef struct ts_msg1003xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       OFFSET;				// by11 - offset
	uint8_t       DATA[MaxDataLen - 12];// by12 - data
} s_m1003xx; // INFO_PARAM_RESPONSE_SEQ message
/*
* @brief INFO_PARAMETER_CHANGE
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, PEER[4] - peer address
* byte 16, LST - list number
* byte 17, DATA[] - data array
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  PEER         LIST  DATA
* l> 15  42  A0   10   23 70 D8  63 19 64  04    01   11 22 33 01  04    11 22 33 00 00   
*/
typedef struct ts_msg1004xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       PEER[4];				// by12 - peer address
	uint8_t       LST;					// by16 - list number
	uint8_t	      DATA[MaxDataLen - 17];// by17 - data array
} s_m1004xx; // INFO_PARAMETER_CHANGE message
/*
* @brief INFO_ACTUATOR_STATUS
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, MSG_CNL - channel
* byte 12, MSG_STAT - device status
* byte 13, UNKNOWN - unknown
* byte 14, MSG_RSSI - rssi value
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  STATUS  UNKNOWN  RSSI
* l> 0E  42  A0   10   23 70 D8  63 19 64  06    01   00      00       80
*/
typedef struct ts_msg1006xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       MSG_CNL;				// by11 - channel
	uint8_t       MSG_STAT;				// by12 - status
	uint8_t       UNKNOWN;				// by13 - unknown
	uint8_t	      MSG_RSSI;				// by14 - rssi value
} s_m1006xx; // INFO_ACTUATOR_STATUS message
/*
* @brief INFO_TEMP
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, SET_ACT[2] - required and actual temperature
* byte 13, VALVE - error, valve, mode
*    LEN CNT FLAG BY03 SND       RCV       By10  SET/ACT  ERR/VALVE/MODE
* l> 0D  42  A0   10   23 70 D8  63 19 64  0A    01 00    00            
*/
typedef struct ts_msg100axx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       SET_ACT[2];			// by11 - required and current temperature
	uint8_t       VALVE;				// by13 - error, valve, mode
} s_m100axx; // INFO_TEMP message

/*
* @brief INSTRUCTION_INHIBIT_OFF
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
* l> 0B  42  A0   11   23 70 D8  63 19 64  00    01    
*/
typedef struct ts_msg1100xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
} s_m1100xx; // INSTRUCTION_INHIBIT_OFF message

/*
* @brief INSTRUCTION_INHIBIT_ON
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
* l> 0B  42  A0   11   23 70 D8  63 19 64  00    01
*/
typedef struct ts_msg1101xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
} s_m1101xx; // INSTRUCTION_INHIBIT_ON message
/*
* @brief INSTRUCTION_SET, ramp and dura time optional
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, VALUE - value
* byte 13, RAMP_TIME - ramp time
* byte 15, DURA_TIME - duration time
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  VALUE  RAMPTIME  DURATIME
* l> 10  42  A0   11   23 70 D8  63 19 64  02    01   FF     01 00     01 00     
*/
typedef struct ts_msg1102xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       VALUE;				// by12 - value
	uint16_t      RAMP_TIME;			// by13 - ramp time
	uint16_t      DURA_TIME;			// by15 - duration time
} s_m1102xx; // INSTRUCTION_SET message
/*
* @brief INSTRUCTION_STOP_CHANGE
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
* l> 0B  42  A0   11   23 70 D8  63 19 64  03    01     
*/
typedef struct ts_msg1103xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
} s_m1103xx; // INSTRUCTION_STOP_CHANGE message
/*
* @brief INSTRUCTION_RESET
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL
* l> 0B  42  A0   11   23 70 D8  63 19 64  04    01
*/
typedef struct ts_msg1104xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
} s_m1104xx; // INSTRUCTION_RESET message
/*
* @brief INSTRUCTION_LED
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, COLOR - color
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  COLOR
* l> 0C  42  A0   11   23 70 D8  63 19 64  80    01   00   
*/
typedef struct ts_msg1180xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       COLOR;				// by12 - color
} s_m1180xx; // INSTRUCTION_LED message
/*
* @brief INSTRUCTION_LED_ALL
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, LED[16] :4 - led 1 to 16
*    LEN CNT FLAG BY03 SND       RCV       By10  BY11  LED 1 -16
* l> 0F  42  A0   11   23 70 D8  63 19 64  81    00    01 02 03 04   
*/
/*typedef struct ts_msg1181xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       LED[16] :4;			// by12 - led 1 to 16
} s_m1181xx; // INSTRUCTION_LED_ALL message*/
/*
* @brief INSTRUCTION_LEVEL
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, TIME - time
* byte 13, SPEED - speed
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  TIME  SPEED
* l> 0D  42  A0   11   23 70 D8  63 19 64  81    01   15    3f      
*/
typedef struct ts_msg1181xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       TIME;					// by12 - time
	uint8_t       SPEED;				// by13 - speed
} s_m1181xx; // INSTRUCTION_LEVEL message
/*
* @brief INSTRUCTION_SLEEPMODE
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, CNL - channel
* byte 12, MODE - sleep mode
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  MODE
* l> 0C  42  A0   11   23 70 D8  63 19 64  82    01   00     
*/
typedef struct ts_msg1182xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       CNL;					// by11 - channel
	uint8_t       MODE;					// by12 - sleep mode
} s_m1182xx; // INSTRUCTION_SLEEPMODE message
/*
* @brief INSTRUCTION_ENTER_BOOTLOADER
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*    LEN CNT FLAG BY03 SND       RCV       By10
* l> 0A  42  A0   11   23 70 D8  63 19 64  83       
*/
typedef struct ts_msg1183xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m1183xx; // INSTRUCTION_ENTER_BOOTLOADER message
/*
* @brief INSTRUCTION_SET_TEMP
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
* byte 11, BV1 - value 1
* byte 12, BV2 - value 2
*    LEN CNT FLAG BY03 SND       RCV       By10  CNL  MODE
* l> 0C  42  A0   11   23 70 D8  63 19 64  82    01   00
*/
typedef struct ts_msg1186xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
	uint8_t       BV1;					// by11 - value 1
	uint8_t       BV2;					// by12 - value 2
} s_m1186xx; // INSTRUCTION_SET_TEMP message
/*
* @brief INSTRUCTION_ADAPTION_DRIVE_SET
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*    LEN CNT FLAG BY03 SND       RCV       By10
* l> 0A  42  A0   11   23 70 D8  63 19 64  87     
*/
typedef struct ts_msg1187xx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m1187xx; // INSTRUCTION_ADAPTION_DRIVE_SET message
/*
* @brief INSTRUCTION_ENTER_BOOTLOADER2
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BY10 - message type
*    LEN CNT FLAG BY03 SND       RCV       By10
* l> 0A  42  A0   11   23 70 D8  63 19 64  CA       
*/
typedef struct ts_msg11caxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m11caxx; // INSTRUCTION_ENTER_BOOTLOADER2 message

/*
* @brief HAVE_DATA
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
*    LEN CNT FLAG BY03 SND       RCV
* l> 09  42  A0   12   23 70 D8  63 19 64   */
typedef struct ts_msg12xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BY10;					// by10 - message type
} s_m12xxxx; // HAVE_DATA message

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
*    LEN CNT FLAG BY03 SND       RCV        DST       UNKNOWN  CNL  COUNTER
* l> 0F  42  A0   3E   23 70 D8  63 19 64   11 22 33  00       01   17   
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
* @brief TIMESTAMP message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, UNKNOWN[2] - to be evaluated
* byte 12, TIME - time, not sure about the format
*    LEN CNT FLAG BY03 SND       RCV        UNKNOWN  TIME
* l> 0C  42  A0   3F   23 70 D8  63 19 64   11 22    00    
*/
typedef struct ts_msg3fxxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       UNKNOWN[2];			// by10 - to be evaluated
	uint8_t       TIME;					// by12 - time
} s_m3fxxxx; // TIMESTAMP message
/*
* @brief REMOTE message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BLL - BUTTON, LONG, LOWBAT
*      - 6bit, BUTTON, or channel
*      - 1bit, LONG, key long pressed?
*      - 1bit, LOWBAT
* byte 11, COUNTER - counter from client device
*    LEN CNT FLAG BY03 SND       RCV        BLL  COUNTER
* l> 0B  42  A0   40   23 70 D8  63 19 64   41   22    
*/
typedef struct ts_msg40xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	struct s_BLL {
		uint8_t BUTTON : 6;
		uint8_t LONG   : 1;
		uint8_t LOWBAT : 1;
	} BLL;								// by10 - button, long, lowbat
	uint8_t       COUNTER;				// by11 - time
} s_m40xxxx; // REMOTE message
/*
* @brief SENSOR_EVENT message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BLL - BUTTON, LONG, LOWBAT
*      - 6bit, BUTTON, or channel
*      - 1bit, LONG, key long pressed?
*      - 1bit, LOWBAT
* byte 11, VALUE - sensor value
* byte 12, COUNTER - counter from client device
*    LEN CNT FLAG BY03 SND       RCV        BLL  VAL  COUNTER
* l> 0C  42  A0   41   23 70 D8  63 19 64   41   50   22     
*/
typedef struct ts_msg41xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	struct s_BLL {
		uint8_t BUTTON : 6;
		uint8_t LONG : 1;
		uint8_t LOWBAT : 1;
	} BLL;								// by10 - button, long, lowbat
	uint8_t       VALUE;				// by11 - sensor value
	uint8_t       COUNTER;				// by12 - counter
} s_m41xxxx; // SENSOR_EVENT message
/*
* @brief SWITCH_LEVEL message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, BUTTON - to be evaluated
* byte 11, NBR - to be evaluated
* byte 12, LEVEL - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        BUTTON  NBR  LEVEL
* l> 0C  42  A0   42   23 70 D8  63 19 64   01      50   22    
*/
typedef struct ts_msg42xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       BUTTON;				// by10 - to be evaluated
	uint8_t       NBR;					// by11 - to be evaluated
	uint8_t       LEVEL;				// by12 - to be evaluated
} s_m42xxxx; // SWITCH_LEVEL message
/*
* @brief SENSOR_DATA message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, CMD - to be evaluated
* byte 11, FLD1 - to be evaluated
* byte 12, VAL1 - to be evaluated
* byte 14, FLD2 - to be evaluated
* byte 15, VAL2 - to be evaluated
* byte 17, FLD3 - to be evaluated
* byte 18, VAL3 - to be evaluated
* byte 20, FLD4 - to be evaluated
* byte 21, VAL4 - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        CMD  FLD1 VAL1   FLD2 VAL2   FLD3 VAL3   FLD4 VAL4
* l> 16  42  A0   53   23 70 D8  63 19 64   01   01   00 00  02   00 00  03   00 00  04   00 00   
*/
typedef struct ts_msg53xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       CMD;					// by10 - to be evaluated
	uint8_t       FLD1;					// by11 - to be evaluated
	uint16_t      VAL1;					// by12 - to be evaluated
	uint8_t       FLD2;					// by14 - to be evaluated
	uint16_t      VAL2;					// by15 - to be evaluated
	uint8_t       FLD3;					// by17 - to be evaluated
	uint16_t      VAL3;					// by18 - to be evaluated
	uint8_t       FLD4;					// by20 - to be evaluated
	uint16_t      VAL4;					// by21 - to be evaluated
} s_m53xxxx; // SENSOR_DATA message
/*
* @brief GAS_EVENT message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, ENERGY[4] - to be evaluated
* byte 14, POWER[3] - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        ENERGY       POWER
* l> 10  42  A0   54   23 70 D8  63 19 64   01 02 03 04  05 06 07   
*/
typedef struct ts_msg54xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[4];			// by10 - to be evaluated
	uint8_t       POWER[3];				// by14 - to be evaluated
} s_m54xxxx; // GAS_EVENT message
/*
* @brief CLIMATE_EVENT message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, CMD - to be evaluated
* byte 11, VALVE - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        CMD  VALVE
* l> 0B  42  A0   58   23 70 D8  63 19 64   01   00      
*/
typedef struct ts_msg58xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       CMD;					// by10 - to be evaluated
	uint8_t       VALVE;				// by11 - to be evaluated
} s_m58xxxx; // CLIMATE_EVENT message
/*
* @brief SET_TEAM_TEMP message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, CMD - to be evaluated
* byte 11, TEMP_MODE - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        CMD  TEMP/MODE
* l> 0B  42  A0   59   23 70 D8  63 19 64   01   00     
*/
typedef struct ts_msg59xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       CMD;					// by10 - to be evaluated
	uint8_t       TEMP_MODE;			// by11 - to be evaluated
} s_m59xxxx; // SET_TEAM_TEMP message
/*
* @brief THERMAL_CONTROL message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, SETTEMP - to be evaluated
* byte 11, ACTTEMP - to be evaluated
* byte 13, HUM - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        SETTEMP  ACTTEMP  HUM
* l> 0D  42  A0   5A   23 70 D8  63 19 64   01       02 00    50   
*/
typedef struct ts_msg5axxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       SETTEMP;				// by10 - to be evaluated
	uint16_t      ACTTEMP;				// by11 - to be evaluated
	uint8_t       HUM;					// by13 - to be evaluated
} s_m5axxxx; // THERMAL_CONTROL message
/*
* @brief POWER_EVENT_CYCLE message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, ENERGY[3] - to be evaluated
* byte 13, POWER[3] - to be evaluated
* byte 16, CURRENT[2] - to be evaluated
* byte 18, VOLTAGE[2] - to be evaluated
* byte 20, FREQUENCY - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        ENERGY    POWER     CURRENT  VOLTAGE  FREQUENCY
* l> 14  42  A0   5E   23 70 D8  63 19 64   01 00 00  02 00 00  03 00    04 00    50
*/
typedef struct ts_msg5exxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[3];			// by10 - to be evaluated
	uint8_t       POWER[3];				// by13 - to be evaluated
	uint8_t       CURRENT[2];			// by16 - to be evaluated
	uint8_t       VOLTAGE[2];			// by18 - to be evaluated
	uint8_t       FREQUENCY;			// by20 - to be evaluated
} s_m5exxxx; // POWER_EVENT_CYCLE message
/*
* @brief POWER_EVENT message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, ENERGY[3] - to be evaluated
* byte 13, POWER[3] - to be evaluated
* byte 16, CURRENT[2] - to be evaluated
* byte 18, VOLTAGE[2] - to be evaluated
* byte 20, FREQUENCY - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        ENERGY    POWER     CURRENT  VOLTAGE  FREQUENCY
* l> 14  42  A0   5F   23 70 D8  63 19 64   01 00 00  02 00 00  03 00    04 00    50   
*/
typedef struct ts_msg5fxxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[3];			// by10 - to be evaluated
	uint8_t       POWER[3];				// by13 - to be evaluated
	uint8_t       CURRENT[2];			// by16 - to be evaluated
	uint8_t       VOLTAGE[2];			// by18 - to be evaluated
	uint8_t       FREQUENCY;			// by20 - to be evaluated
} s_m5fxxxx; // POWER_EVENT message
/*
* @brief WEATHER_EVENT message
* byte 00, MSG_LEN - message length
* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
* byte 02, FLAG - see structure of message flags
* byte 03, MSG_TYP - type of message
* byte 04, SND_ID[3] - sender ID
* byte 07, RCV_ID[3] - receiver id, broadcast for 0
* byte 10, TEMP - to be evaluated
* byte 12, HUM - to be evaluated
*    LEN CNT FLAG BY03 SND       RCV        TEMP   HUM
* l> 0C  42  A0   70   23 70 D8  63 19 64   01 00  00      
*/
typedef struct ts_msg70xxxx {
	uint8_t       MSG_LEN;				// by00 - message length
	uint8_t       MSG_CNT;				// by01 - message counter
	struct s_mFlg FLAG;					// by02 - see structure of message flags
	uint8_t       MSG_TYP;				// by03 - type of message
	uint8_t       SND_ID[3];			// by04 - sender ID
	uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
	uint16_t       TEMP;				// by10 - to be evaluated
	uint8_t       HUM;					// by12 - to be evaluated
} s_m70xxxx; // WEATHER_EVENT message


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

	union {
		uint8_t buf[MaxDataLen];		// initial buffer for received and decoded message string
		s_mBody mBody;					// struct on buffer for easier data access
		s_m00xxxx m00xxxx;				// DEVICE_INFO
		s_m01xx01 m01xx01;				// CONFIG_PEER_ADD
		s_m01xx02 m01xx02;				// CONFIG_PEER_REMOVE
		s_m01xx03 m01xx03;				// CONFIG_PEER_LIST_REQ
		s_m01xx04 m01xx04;				// CONFIG_PARAM_REQ
		s_m01xx05 m01xx05;				// CONFIG_START
		s_m01xx06 m01xx06;				// CONFIG_END
		s_m01xx07 m01xx07;				// CONFIG_WRITE_INDEX1
		s_m01xx08 m01xx08;				// CONFIG_WRITE_INDEX2
		s_m01xx09 m01xx09;				// CONFIG_SERIAL_REQ
		s_m01xx0a m01xx0a;				// CONFIG_PAIR_SERIAL
		s_m01xx0e m01xx0e;				// CONFIG_STATUS_REQUEST
		s_m0200xx m0200xx;				// ACK message
		s_m0201xx m0201xx;				// ACK_STATUS message
		s_m0202xx m0202xx;				// ACK2 message
		s_m0204xx m0204xx;				// AES_REQ message
		s_m0280xx m0280xx;				// NACK message
		s_m0284xx m0284xx;				// NACK_TARGET_INVALID message
		s_m02xxxx m02xxxx;				// ACK_NACK_UNKNOWN message
		s_m03xxxx m03xxxx;				// AES_REPLY message
		s_m0401xx m0401xx;				// SEND_AES_TO_HMLAN
		s_m04xxxx m04xxxx;				// SEND_AES_TO_ACTOR
		s_m1000xx m1000xx;				// INFO_SERIAL message
		s_m1001xx m1001xx;				// INFO_PEER_LIST message
		s_m1002xx m1002xx;				// INFO_PARAM_RESPONSE_PAIRS message
		s_m1003xx m1003xx;				// INFO_PARAM_RESPONSE_SEQ message
		s_m1004xx m1004xx;				// INFO_PARAMETER_CHANGE message
		s_m1006xx m1006xx;				// INFO_ACTUATOR_STATUS message
		s_m100axx m100axx;				// INFO_TEMP message
		s_m1100xx m1100xx;				// INSTRUCTION_INHIBIT_OFFINSTRUCTION_INHIBIT_OFF message
		s_m1101xx m1101xx;				// INSTRUCTION_INHIBIT_ON message
		s_m1102xx m1102xx;				// INSTRUCTION_SET message
		s_m1103xx m1103xx;				// INSTRUCTION_STOP_CHANGE message
		s_m1104xx m1104xx;				// INSTRUCTION_RESET message
		s_m1180xx m1180xx;				// INSTRUCTION_LED message
		s_m1181xx m1181xx;				// INSTRUCTION_LEVEL message
		s_m1182xx m1182xx;				// INSTRUCTION_SLEEPMODE message
		s_m1183xx m1183xx;				// INSTRUCTION_ENTER_BOOTLOADER message
		s_m1186xx m1186xx;				// INSTRUCTION_SET_TEMP message
		s_m1187xx m1187xx;				// INSTRUCTION_ADAPTION_DRIVE_SET message
		s_m11caxx m11caxx;				// INSTRUCTION_ENTER_BOOTLOADER2 message
		s_m12xxxx m12xxxx;				// HAVE_DATA message
		s_m3Exxxx m3Exxxx;				// SWITCH message
		s_m3fxxxx m3fxxxx;				// TIMESTAMP message
		s_m40xxxx m40xxxx;				// REMOTE message
		s_m41xxxx m41xxxx;				// SENSOR_EVENT message
		s_m42xxxx m42xxxx;				// SWITCH_LEVEL message
		s_m53xxxx m53xxxx;				// SENSOR_DATA message
		s_m54xxxx m54xxxx;				// GAS_EVENT message
		s_m58xxxx m58xxxx;				// CLIMATE_EVENT message
		s_m59xxxx m59xxxx;				// SET_TEAM_TEMP message
		s_m5axxxx m5axxxx;				// THERMAL_CONTROL message
		s_m5exxxx m5exxxx;				// POWER_EVENT_CYCLE message
		s_m5fxxxx m5fxxxx;				// POWER_EVENT message
		s_m70xxxx m70xxxx;				// WEATHER_EVENT message	
	};
	uint8_t prev_buf[27];				// store the last receive message to verify with AES signed data.

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
typedef struct ts_send_message {
	MSG_ACTIVE::E active;				// flag that something is to process

	union {
		uint8_t buf[MaxDataLen];		// initial buffer for messages to send
		s_mBody mBody;					// struct on buffer for easier data access
		s_m00xxxx m00xxxx;				// DEVICE_INFO
		s_m01xx01 m01xx01;				// CONFIG_PEER_ADD
		s_m01xx02 m01xx02;				// CONFIG_PEER_REMOVE
		s_m01xx03 m01xx03;				// CONFIG_PEER_LIST_REQ
		s_m01xx04 m01xx04;				// CONFIG_PARAM_REQ
		s_m01xx05 m01xx05;				// CONFIG_START
		s_m01xx06 m01xx06;				// CONFIG_END
		s_m01xx07 m01xx07;				// CONFIG_WRITE_INDEX1
		s_m01xx08 m01xx08;				// CONFIG_WRITE_INDEX2
		s_m01xx09 m01xx09;				// CONFIG_SERIAL_REQ
		s_m01xx0a m01xx0a;				// CONFIG_PAIR_SERIAL
		s_m01xx0e m01xx0e;				// CONFIG_STATUS_REQUEST
		s_m0200xx m0200xx;				// ACK message
		s_m0201xx m0201xx;				// ACK_STATUS message
		s_m0202xx m0202xx;				// ACK2 message
		s_m0204xx m0204xx;				// AES_REQ message
		s_m0280xx m0280xx;				// NACK message
		s_m0284xx m0284xx;				// NACK_TARGET_INVALID message
		s_m02xxxx m02xxxx;				// ACK_NACK_UNKNOWN message
		s_m03xxxx m03xxxx;				// AES_REPLY message
		s_m0401xx m0401xx;				// SEND_AES_TO_HMLAN
		s_m04xxxx m04xxxx;				// SEND_AES_TO_ACTOR
		s_m1000xx m1000xx;				// INFO_SERIAL message
		s_m1001xx m1001xx;				// INFO_PEER_LIST message
		s_m1002xx m1002xx;				// INFO_PARAM_RESPONSE_PAIRS message
		s_m1003xx m1003xx;				// INFO_PARAM_RESPONSE_SEQ message
		s_m1004xx m1004xx;				// INFO_PARAMETER_CHANGE message
		s_m1006xx m1006xx;				// INFO_ACTUATOR_STATUS message
		s_m100axx m100axx;				// INFO_TEMP message
		s_m1100xx m1100xx;				// INSTRUCTION_INHIBIT_OFFINSTRUCTION_INHIBIT_OFF message
		s_m1101xx m1101xx;				// INSTRUCTION_INHIBIT_ON message
		s_m1102xx m1102xx;				// INSTRUCTION_SET message
		s_m1103xx m1103xx;				// INSTRUCTION_STOP_CHANGE message
		s_m1104xx m1104xx;				// INSTRUCTION_RESET message
		s_m1180xx m1180xx;				// INSTRUCTION_LED message
		s_m1181xx m1181xx;				// INSTRUCTION_LEVEL message
		s_m1182xx m1182xx;				// INSTRUCTION_SLEEPMODE message
		s_m1183xx m1183xx;				// INSTRUCTION_ENTER_BOOTLOADER message
		s_m1186xx m1186xx;				// INSTRUCTION_SET_TEMP message
		s_m1187xx m1187xx;				// INSTRUCTION_ADAPTION_DRIVE_SET message
		s_m11caxx m11caxx;				// INSTRUCTION_ENTER_BOOTLOADER2 message
		s_m12xxxx m12xxxx;				// HAVE_DATA message
		s_m3Exxxx m3Exxxx;				// SWITCH message
		s_m3fxxxx m3fxxxx;				// TIMESTAMP message
		s_m40xxxx m40xxxx;				// REMOTE message
		s_m41xxxx m41xxxx;				// SENSOR_EVENT message
		s_m42xxxx m42xxxx;				// SWITCH_LEVEL message
		s_m53xxxx m53xxxx;				// SENSOR_DATA message
		s_m54xxxx m54xxxx;				// GAS_EVENT message
		s_m58xxxx m58xxxx;				// CLIMATE_EVENT message
		s_m59xxxx m59xxxx;				// SET_TEAM_TEMP message
		s_m5axxxx m5axxxx;				// THERMAL_CONTROL message
		s_m5exxxx m5exxxx;				// POWER_EVENT_CYCLE message
		s_m5fxxxx m5fxxxx;				// POWER_EVENT message
		s_m70xxxx m70xxxx;				// WEATHER_EVENT message	
	};
	uint8_t   prev_buf[32];				// store the last receive message to verify with AES signed data.

	uint8_t   timeout;					// was last message a timeout
	uint8_t   retr_cnt;					// variable to count how often a message was already send
	uint8_t   temp_max_retr;			// is set depending on the BIDI flag and the max_retr value
	uint8_t   temp_MSG_CNT;				// store for message counter, needed to identify ACK

	uint8_t   MSG_CNT;					// message counter for initial sends
	uint8_t   max_retr;					// how often a message has to be send until ACK - info is set by cmMaintenance
	uint16_t  max_time;					// max time for message timeout timer - info is set by  cmMaintenance
	waitTimer timer;					// send mode timeout

	MSG_TYPE::E  type;					// set the message type for further processing in send function

	void clear() {						// function to reset flags
		active = MSG_ACTIVE::NONE;
		timeout = 0;
		retr_cnt = 0;
		temp_max_retr = 0;
		timer.set(0);
	}

} s_snd_msg;



/*
* @brief Type of list request answer
* PEER_LIST, as answer to a CONFIG_PEER_LIST_REQ
* PARAM_RESPONSE_PAIRS, as answer on a CONFIG_PARAM_REQ
* PARAM_RESPONSE_SEQ, as answer on a CONFIG_PARAM_REQ 
*/
namespace LIST_ANSWER {
	enum E : uint8_t { NONE = 0, PEER_LIST = 1, PARAM_RESPONSE_PAIRS = 2, PARAM_RESPONSE_SEQ = 3, };
};

/*
* @brief Struct to hold all information to answer peer or param request answers.
* This type of messages generates more than one answer string and needs to be processed in a loop.
* The answer is prepared by ts_list_table or ts_peer_table, but processed in send.h
*/
typedef struct ts_list_msg {
	//uint8_t active;						// indicates status of poll routine, 1 is active
	LIST_ANSWER::E active = LIST_ANSWER::NONE; // defines the type of answer message, valid types are: PEER_LIST, PARAM_RESPONSE_PAIRS, PARAM_RESPONSE_SEQ,
	uint8_t cur_slc;					// counter for slices which are already send
	uint8_t max_slc;					// amount of necessary slices to send content
	s_list_table *list;					// pointer to the respective list table for answering the request
	uint8_t peer_idx;					// peer index if a list3 or 4 is requested
	s_peer_table *peer;					// pointer to the peer table in case in is a PEER_LIST answer
	waitTimer timer;					// give the master some time, otherwise we need to resend
} s_list_msg;


/*
* @brief Struct to hold all information to send peer messages.
*/
typedef struct ts_peer_msg {
	MSG_ACTIVE::E active = MSG_ACTIVE::NONE;// flag that something is to process
	MSG_TYPE::E   type;					// set the message type for further processing in send function

	s_peer_table *peerDB;				// pointer to respective peer table for peer message
	s_list_table *lstC;
	s_list_table *lstP;					// ponter to list4 for peer message
	uint8_t      *payload_ptr;			// pointer to the payload
	uint8_t       payload_len;			// length of payload
	uint8_t       max_retr;				// max retry counter for peer messages

	uint8_t       slot_tbl[8];			// peer slot table
	uint8_t       slot_cnt;				// peer slot counter
	uint8_t       retr_cnt;				// current retry counter for peer messages

	void set_slot(uint8_t idx) {		// set bit in slot table
		slot_tbl[idx >> 3] |= (1 << (idx & 0x07));
	}
	uint8_t get_slot(uint8_t idx) {		// get bit in slot table	
		return ((slot_tbl[idx >> 3] & (1 << (idx & 0x07)))) ? 1 : 0;
	}
	void clear_slot(uint8_t idx) {		// clear bit in slot table
		slot_tbl[idx >> 3] &= ~(1 << (idx & 0x07));
	}
	void prep_slot(void) {				// prepare the slot table
		for (uint8_t i = 0; i < peerDB->max; i++) {
			if (*(uint32_t*)peerDB->get_peer(i)) set_slot(i);
			else clear_slot(i);
		}
	}

	void clear() {						// function to reset flags
		active = MSG_ACTIVE::NONE;
		slot_cnt = 0;
		retr_cnt = 0;
	}

} s_peer_msg;

#endif