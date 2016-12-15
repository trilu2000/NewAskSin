//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin typedefs -------------------------------------------------------------------------------------------------------
//- global definition of type definition beside messages
//- -----------------------------------------------------------------------------------------------------------------------

#include "HAL.h"
#include "00_debug-flag.h"
#include "wait_timer.h"
#include "AS_enum_defs.h"
#include "AS_message_defs.h"

#ifndef _AS_TYPEDEFS_H
#define _AS_TYPEDEFS_H




/* 
* @brief First bytes of eeprom holds all device specific information for identification
*/
typedef struct ts_dev_ident {
	uint16_t MAGIC;							//  2 byte - magic byte
	uint8_t  HMID[3];						//  3 byte - homematic id
	uint8_t  SERIAL_NR[10];					// 10 byte - serial number
	uint8_t  HMKEY_INDEX[1];				//  1 byte - aes key index
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
		//dbg << "ptr_to_val(" << _HEXB(reg_addr) << ") = "; if (!pos_in_reg) dbg << "NULL = "; dbg << _HEXB(*(val + (pos_in_reg - reg))) << '\n';
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
		uint8_t slices = 0;	int16_t total = (len * 2) + 2;		// len of register + len of value + 2byte because of termination of string
		while ((total -= byte_per_msg) > 0) slices++;
		return ++slices;
	}

	uint8_t get_slice_pairs(uint8_t idx, uint8_t slc, uint8_t *buf, uint8_t byte_per_msg = 16) { // returns a sliced peer list
		load_list(idx);											// load the eeprom content by idx into the value table
		byte_per_msg /= 2;										// divided by 2 while we mix two arrays
		uint8_t slc_start = slc * byte_per_msg;					// calculate the start point for reg and val

		uint8_t slc_end = slc_start + byte_per_msg;				// calculate the corresponding slice end byte
		if (slc_end > (len + 1)) slc_end = len + 1;				// if calculated end point is bigger than the physical; + 1 because of terminating 00 00

		for (uint8_t i = slc_start; i < slc_end; i++) {	
			if (i >= len) {										// check the case that it is the last slice, because we have to add the terminating 00 00
				memset(buf, 0, 2); 
			} else {											// add the byte from reg and val to the buffer
				memcpy_P(buf++, &reg[i], 1); memcpy(buf++, &val[i], 1);
			}
		}
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
typedef struct ts_receive_message {

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

	MSG_INTENT::E intend;				// remember the intent of the message, filled by receive class
	uint8_t peer[4];					// peer is stored as a 4 byte array, but most messages delivers it with a seperate channel field (byte 10)
	uint8_t cnl;						// by getting the intent the peer is checked, here we are store the channel where the peer is registered in
	uint8_t prev_MSG_CNT;				// remember the last message counter to check if it is a repeated message

	void clear() {						// function to reset flags
		buf[0] = 0;
	}
} s_rcv_msg;

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
* @brief Struct to hold all information to answer peer or param request answers.
* This type of messages generates more than one answer string and needs to be processed in a loop.
* The answer is prepared by ts_list_table or ts_peer_table, but processed in send.h
*/
typedef struct ts_list_msg {
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


/*
* @brief Helper struct to remember on the pairing mode status in asksin main function
*/
typedef struct ts_pair_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	waitTimer timer;						// pairing mode timeout
} s_pair_mode;

/*
* @brief Helper struct to remember on the config mode status in asksin main function
*/
typedef struct ts_config_mode {
	uint8_t   active;						// indicates status, 1 if config mode is active
	uint8_t   idx_peer;						// and the peer index
	s_list_table *list;						// pointer to the respective list
	waitTimer timer;						// config mode timeout
} s_config_mode;

#endif