//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin receive function -----------------------------------------------------------------------------------------------
//- message definition mainly from homematic modul HMConfig.pm in FHEM
//- -----------------------------------------------------------------------------------------------------------------------

#include "HAL.h"
#include "wait_timer.h"

#ifndef _AS_TYPEDEFS_H
#define _AS_TYPEDEFS_H

/*
* @brief Intent of message, used for receive and send function
* Types are, valid for send or receive id
* broadcast  - 0, messgae from or to all, indicated by 00 00 00
* master     - 1, message to or from HM Central, communication between paired devices
* peer       - 2, message to or from a known peer
* internal   - 3, message to or from ourself, will not be transmitted
* logging    - 4, not for us, show in log only
* not paired - 5, message addressed to us, but pair or peer not known
* error      - 6, don't know
*/
enum MSG_INTENT { BROADCAST = 0x00, MASTER = 0x01, PEER = 0x02, INTERN = 0x03, LOGGING = 0x04, NOT_PAIRED = 0x05, ERROR = 0x06, };



/*
* @brief Helper struct for all send function relevant variables
*/
typedef struct ts_send_flags {
	uint8_t   active;						// is send module active, 1 indicates yes
	uint8_t   timeout;						// was last message a timeout
	uint8_t   retr_cnt;						// variable to count how often a message was already send
	uint8_t   max_retr;						// how often a message has to be send until ACK
	uint16_t  max_time;						// max time for message timeout timer
	waitTimer timer;						// send mode timeout
} s_send_flags;

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
*/
typedef struct ts_list_table {
	uint8_t cnl;							// holds the channel information
	uint8_t lst;							// holds the list number
	uint8_t *reg;							// pointer to register values in PROGMEM
	uint8_t *def;							// pointer to default values in PROGMEM
	uint8_t *val;							// pointer to value array which is dynamic loaded from eeprom
	uint8_t len;							// length of register, defaults and value array
	uint16_t ee_addr;						// start address for channel in eeprom
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
} s_peer_table;


/*
* @brief Type of message, Byte 03 translation
* -------------------------------------------
* 0x00 ff ff * DEVICE_INFO
* -------------------------------------------
* 0x01 ff ff * CONFIG_REQ
* 0x01 ff 01 * - CONFIG_PEER_ADD 
* 0x01 ff 02 * - CONFIG_PEER_REMOVE  
* 0x01 ff 03 * - CONFIG_PEER_LIST_REQ 
* 0x01 ff 04 * - CONFIG_PARAM_REQ
* 0x01 ff 05 * - CONFIG_START
* 0x01 ff 06 * - CONFIG_END
* 0x01 ff 07 * - CONFIG_WRITE_INDEX1
* 0x01 ff 08 * - CONFIG_WRITE_INDEX2
* 0x01 ff 09 * - CONFIG_SERIAL_REQ
* 0x01 ff 0a * - PAIR_SERIAL
* 0x01 ff 0e * - CONFIG_STATUS_REQUEST
* -------------------------------------------
* 0x02 ff ff * ACK_MSG
* 0x02 00 ff * - ACK
* 0x02 01 ff * - ACK_STATUS
* 0x02 02 ff * - ACK2
* 0x02 04 ff * - ACK_PROC
* 0x02 80 ff * - NACK
* 0x02 84 ff * - NACK_TARGET_INVALID
* 0x02 FE ff * - ACK_NACK_UNKNOWN, short string no byte 10 value
* 0x02 FF ff * - REQUEST_AES, no by10 value, some format as ACK
* -------------------------------------------
* 0x03 ff ff * AES_REPLY
* -------------------------------------------
* 0x04 ff ff * SEND_AES_CODE
* 0x04 01 ff * - SEND_AES_TO_HMLAN
* 0x04 ff ff * - SEND_AES_TO_ACTOR, no byte 10, same format as SEND_AES_CODE
* -------------------------------------------
* 0x10 ff ff * REPLY_MSG
* 0x10 00 ff * - INFO_SERIAL
* 0x10 01 ff * - INFO_PEER_LIST
* 0x10 02 ff * - INFO_PARAM_RESPONSE_PAIRS
* 0x10 03 ff * - INFO_PARAM_RESPONSE_SEQ
* 0x10 04 ff * - INFO_PARAMETER_CHANGE
* 0x10 06 ff * - INFO_ACTUATOR_STATUS
* 0x10 0A ff * - INFO_TEMP
* -------------------------------------------
* 0x11 ff ff * INSTRUCTION_MSG = 0x11
* 0x11 02 ff * - INSTRUCTION_SET
* 0x11 03 ff * - INSTRUCTION_STOP_CHANGE
* 0x11 04 00 * - INSTRUCTION_RESET
* 0x11 80 ff * - INSTRUCTION_LED
* 0x11 81 00 * - INSTRUCTION_LED_ALL
* 0x11 81 FF * - INSTRUCTION_LEVEL
* 0x11 82 FF * - INSTRUCTION_SLEEPMODE
* -------------------------------------------
* 0x12 ff ff * HAVE_DATA = 0x12
* -------------------------------------------
* 0x3e ff ff * SWITCH = 0x3e
* 0x40 ff ff * REMOTE = 0x40
* 0x41 ff ff * SENSOR_EVENT = 0x41
* 0x53 ff ff * SENSOR_DATA = 0x53
* 0x58 ff ff * CLIMATE_EVENT = 0x58
* 0x59 ff ff * SET_TEAM_TEMP = 0x59
* 0x70 ff ff * WEATHER_EVENT = 0x70
* -------------------------------------------
*/
enum TYPE_MSG { DEVICE_INFO = 0x00ffff, 
	CONFIG_REQ = 0x01ffff, 
	CONFIG_PEER_ADD = 0x01ff01, 
	CONFIG_PEER_REMOVE = 0x01ff02, 
	CONFIG_PEER_LIST_REQ = 0x01ff03, 
	CONFIG_PARAM_REQ = 0x01ff04, 
	CONFIG_START = 0x01ff05, 
	CONFIG_END = 0x01ff06, 
	CONFIG_WRITE_INDEX1 = 0x01ff07, 
	CONFIG_WRITE_INDEX2 = 0x01ff08, 
	CONFIG_SERIAL_REQ = 0x01ff09, 
	PAIR_SERIAL = 0x01ff0a, 
	CONFIG_STATUS_REQUEST = 0x01ff0e, 
	ACK_MSG	= 0x02ffff, 
	ACK = 0x0200ff, 
	ACK_STATUS = 0x0201ff, 
	ACK2 = 0x0202ff, 
	ACK_PROC = 0x0204ff, 
	NACK = 0x0280ff, 
	NACK_TARGET_INVALID = 0x0284ff, 
	ACK_NACK_UNKNOWN = 0x02FEff, 
	REQUEST_AES = 0x02FFff, 
	AES_REPLY = 0x03ffff, 
	SEND_AES_CODE = 0x04ffff, 
	SEND_AES_TO_HMLAN = 0x0401ff, 
	SEND_AES_TO_ACTOR = 0x04ffff,
	REPLY_MSG = 0x10ffff,
	INFO_SERIAL = 0x1000ff, 
	INFO_PEER_LIST = 0x1001ff, 
	INFO_PARAM_RESPONSE_PAIRS = 0x1002ff, 
	INFO_PARAM_RESPONSE_SEQ = 0x1003ff, 
	INFO_PARAMETER_CHANGE = 0x1004ff, 
	INFO_ACTUATOR_STATUS = 0x1006ff, 
	INFO_TEMP = 0x100Aff, 
	INSTRUCTION_MSG = 0x11ffff, 
	INSTRUCTION_SET = 0x1102ff, 
	INSTRUCTION_STOP_CHANGE = 0x1103ff, 
	INSTRUCTION_RESET = 0x110400, 
	INSTRUCTION_LED = 0x1180ff, 
	INSTRUCTION_LED_ALL = 0x118100, 
	INSTRUCTION_LEVEL = 0x1181ff, 
	INSTRUCTION_SLEEPMODE = 0x1182ff, 
	HAVE_DATA = 0x12ffff, 
	SWITCH = 0x3effff, 
	REMOTE = 0x40ffff, 
	SENSOR_EVENT = 0x41ffff, 
	SENSOR_DATA = 0x53ffff, 
	CLIMATE_EVENT = 0x58ffff, 
	SET_TEAM_TEMP = 0x59ffff, 
	WEATHER_EVENT = 0x70ffff,
};



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


#define MaxDataLen   40						// maximum length of received bytes
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
typedef union u_Message {

	/*
	* @brief GENERIC message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, BY10 - message type
	* byte 11, BY11 - message type
	* byte 12, PAYLOAD - payload of message
	*/
	struct s_msgBody {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       BY10;					// by10 - type of message
		uint8_t       BY11;					// by10 - type of message
		uint8_t       PAYLOAD[MaxDataLen - 12];// payload
	} mBody; // GENERIC message


	/*
	* @brief DEVICE_INFO message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
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
	struct s_msg00xxxx {
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
	} m00xxxx; // DEVICE_INFO message


	/*
	* @brief CONFIG_PEER_ADD message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, PEER_ID - peer address
	* byte 15, PEER_CNL_A - peer channel 1
	* byte 16, PEER_CNL_B - peer channel 2
	*/
	struct s_msg01xx01 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       BY03;					// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
		uint8_t	      PEER_ID[3];			// by12 - peer address
		uint8_t       PEER_CNL_A;			// by15 - peer channel 1
		uint8_t       PEER_CNL_B;			// by16 - peer channel 2
	} m01xx01; // CONFIG_PEER_ADD message

	/*
	* @brief CONFIG_PEER_REMOVE message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, PEER_ID - peer address
	* byte 15, PEER_CNL_A - peer channel 1
	* byte 16, PEER_CNL_B - peer channel 2
	*/
	struct s_msg01xx02 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
		uint8_t	      PEER_ID[3];			// by12 - peer address
		uint8_t       PEER_CNL_A;			// by15 - peer channel 1
		uint8_t       PEER_CNL_B;			// by16 - peer channel 2
	} m01xx02; // CONFIG_PEER_REMOVE message

	/*
	* @brief CONFIG_PEER_LIST_REQ message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	*/
	struct s_msg01xx03 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
	} m01xx03; // CONFIG_PEER_LIST_REQ message

	/*
	* @brief CONFIG_PARAM_REQ message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, PEER_ID - peer address
	* byte 15, PEER_CNL - peer channel
	* byte 16, PARAM_LIST - parameter list 0, 1, 3, 4, etc.
	*/
	struct s_msg01xx04 {
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
	} m01xx04; // CONFIG_PARAM_REQ message

	/*
	* @brief CONFIG_START message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, PEER_ID - peer address
	* byte 15, PEER_CNL - peer channel
	* byte 16, PARAM_LIST - parameter list 0, 1, 3, 4, etc.
	*/
	struct s_msg01xx05 {
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
	} m01xx05; // CONFIG_START message

	/*
	* @brief CONFIG_END message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	*/
	struct s_msg01xx06 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
	} m01xx06; // CONFIG_END message

	/*
	* @brief CONFIG_WRITE_INDEX message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, ADDR - address for data
	* byte 13, DATA[MaxDataLen - 13] - data, start register in address, remaining data
	*/
	struct s_msg01xx07 {
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
	} m01xx07; // CONFIG_WRITE_INDEX message

	/*
	* @brief CONFIG_WRITE_INDEX message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 13, DATA[MaxDataLen - 12] - data, 1 byte register, 1 byte data, ...
	*/
	struct s_msg01xx08 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
		uint8_t       DATA[MaxDataLen - 12];// by12 - data, 1 byte register, 1 byte data, ...
	} m01xx08; // CONFIG_WRITE_INDEX message

	/*
	* @brief CONFIG_SERIAL_REQ message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 13, DATA[MaxDataLen - 12] - data, 1 byte register, 1 byte data, ...
	*/
	struct s_msg01xx09 {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
		uint8_t       DATA[MaxDataLen - 12];// by12 - data, 1 byte register, 1 byte data, ...
	} m01xx09; // CONFIG_SERIAL_REQ message

	/*
	* @brief PAIR_SERIAL message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	* byte 12, SERIALNO[10] - serial number of device
	*/
	struct s_msg01xx0a {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
		uint8_t       SERIALNO[10];			// by12 - 10 byte serial number
	} m01xx0a; // PAIR_SERIAL message

	/*
	* @brief CONFIG_STATUS_REQUEST message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, MSG_CNL - message channel
	* byte 11, BY11 - message type
	*/
	struct s_msg01xx0e {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       MSG_CNL;				// by10 - message channel
		uint8_t       BY11;					// by11 - message type
	} m01xx0e; // CONFIG_STATUS_REQUEST message


	/*
	* @brief ACK message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, BY10 - message type
	*/
	struct s_msg0200xx {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       BY10;					// by10 - message type
	} m0200xx; // ACK message

	/*
	* @brief ACK_STATUS message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, BY10 - message type
	* byte 11, MSG_CNL - message channel
	* byte 12, MSG_STATUS - message status
	* byte 13, MSG_FLAG - message flags, up, down, lowbat
	* byte 14, MSG_RSSI - RSSI recording of device
	*/
	struct s_msg0201xx {
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
			uint8_t	 UP     : 1;	// & 0x10
			uint8_t  DOWN   : 1;	// & 0x20
			uint8_t	        : 1;	// & 0x40
			uint8_t	 LOWBAT : 1;	// & 0x80
		} MSG_FLAG;							// by13 - message flags
		uint8_t      MSG_RSSI;				// by14 - recording of device
	} m0201xx; // ACK_STATUS message

	/*
	* @brief ACK2 message
	* byte 00, MSG_LEN - message length
	* byte 01, MSG_CNT - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02, FLAG - see structure of message flags
	* byte 03, BY03 - type of message
	* byte 04, SND_ID[3] - sender ID
	* byte 07, RCV_ID[3] - receiver id, broadcast for 0
	* byte 10, BY10 - message type
	*/
	struct s_msg0202xx {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       BY10;					// by10 - message channel
	} m0202xx; // ACK2 message

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
	* byte 00 - message length
	* byte 01 - counter, if it is an answer counter has to reflect the answered message, otherwise own counter has to be used
	* byte 02 - see structure of message flags
	* byte 03 - type of message
	* byte 04 - sender ID
	* byte 07 - receiver id, broadcast for 0
	* byte 10 - peer address registered for action
	* byte 13 - unknown
	* byte 14 - peer channel
	* byte 15 - peer count
	*/
	struct s_msg3Exxxx {
		uint8_t       MSG_LEN;				// by00 - message length
		uint8_t       MSG_CNT;				// by01 - message counter
		struct s_mFlg FLAG;					// by02 - see structure of message flags
		uint8_t       MSG_TYP;				// by03 - type of message
		uint8_t       SND_ID[3];			// by04 - sender ID
		uint8_t       RCV_ID[3];			// by07 - receiver id, broadcast for 0
		uint8_t       PEER[3];				// by10 - peer address registered for action
		uint8_t       : 8;					// by13 - unknown
		uint8_t       P_CNL;				// by14 - peer channel
		uint8_t       P_CNT;				// by15 - peer count
	} m3Exxxx; // SWITCH message
} u_Message;




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