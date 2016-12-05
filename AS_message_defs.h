//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin message definition ---------------------------------------------------------------------------------------------
//- global definition of asksin message structs
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _AS_MESSAGEDEFS_H
#define _AS_MESSAGEDEFS_H

#define MaxDataLen   40						// maximum length for send and receive strings

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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - type of message
	uint8_t       BY11;						// by10 - type of message
	uint8_t       PAYLOAD[MaxDataLen - 12];	// payload
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       FIRMWARE;					// by10 - firmware version
	uint8_t       DEV_TYPE[2];				// by11 - device type
	uint8_t       SERIALNO[10];				// by13 - 10 byte serial number of device
	uint8_t       CLASS;					// by23 - ??
	uint8_t       PEER_CNL_A;				// by24 - peer channel 1
	uint8_t       PEER_CNL_B;				// by25 - peer channel 2
	uint8_t	      UNKNOWN;					// by26 - unknown
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t	      PEER_ID[3];				// by12 - peer address
	uint8_t       PEER_CNL[2];				// by15, by16 - peer channel A and B
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t	      PEER_ID[3];				// by12 - peer address
	uint8_t       PEER_CNL[2];				// by15, by16 - peer channel A and B
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t       PEER_ID[4];				// by12 - peer address, incl peer cnl
	uint8_t       PARAM_LIST;				// by16 - parameter list 0, 1, 3, 4, etc.
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t       PEER_ID[4];				// by12 - peer address
	uint8_t       PARAM_LIST;				// by16 - parameter list 0, 1, 3, 4, etc.
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t       ADDR;						// by12 - address for data
	uint8_t       DATA[MaxDataLen - 13];	// by13 - data, start register in address, remaining data
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t       DATA[MaxDataLen - 12];	// by12 - data, 1 byte register, 1 byte data, ...
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
	uint8_t       SERIALNO[10];				// by12 - 10 byte serial number
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       MSG_CNL;					// by10 - message channel
	uint8_t       BY11;						// by11 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message channel
	uint8_t       MSG_CNL;					// by11 - message channel
	uint8_t       MSG_STATUS;				// by12 - message status
	struct s_DUL {
		uint8_t         : 4;
		uint8_t	 UP     : 1;				// & 0x10
		uint8_t  DOWN   : 1;				// & 0x20
		uint8_t         : 1;				// & 0x40
		uint8_t	 LOWBAT : 1;				// & 0x80
	} MSG_FLAG;								// by13 - message flags
	uint8_t      MSG_RSSI;					// by14 - recording of device
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message channel
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message sub type
	uint8_t       PARA1[2];					// by11 - AES parameter 1
	uint8_t       PARA2[2];					// by13 - AES parameter 2
	uint8_t       PARA3[2];					// by15 - AES parameter 3
	uint8_t       KEYNR;					// by17 - Key number
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       CNL;						// by10 - required channel
	uint8_t       TYPE;						// by11 - type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       SERIALNO[10];				// by11 - serial number
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       PEER1[4];					// by11 - 1st peer
	uint8_t       PEER2[4];					// by15 - 2nd peer
	uint8_t       PEER3[4];					// by19 - 3rd peer
	uint8_t       PEER4[4];					// by23 - 4th peer
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       DATA[MaxDataLen - 11];	// by11 - data
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       OFFSET;					// by11 - offset
	uint8_t       DATA[MaxDataLen - 12];	// by12 - data
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       PEER[4];					// by12 - peer address
	uint8_t       LST;						// by16 - list number
	uint8_t	      DATA[MaxDataLen - 17];	// by17 - data array
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       MSG_CNL;					// by11 - channel
	uint8_t       MSG_STAT;					// by12 - status
	uint8_t       UNKNOWN;					// by13 - unknown
	uint8_t	      MSG_RSSI;					// by14 - rssi value
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       SET_ACT[2];				// by11 - required and current temperature
	uint8_t       VALVE;					// by13 - error, valve, mode
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       VALUE;					// by12 - value
	uint16_t      RAMP_TIME;				// by13 - ramp time
	uint16_t      DURA_TIME;				// by15 - duration time
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       COLOR;					// by12 - color
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       LED[16] :4;				// by12 - led 1 to 16
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       TIME;						// by12 - time
	uint8_t       SPEED;					// by13 - speed
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       CNL;						// by11 - channel
	uint8_t       MODE;						// by12 - sleep mode
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
	uint8_t       BV1;						// by11 - value 1
	uint8_t       BV2;						// by12 - value 2
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BY10;						// by10 - message type
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       PEER[3];					// by10 - peer address registered for action
	uint8_t       NA;						// by13 - unknown
	uint8_t       P_CNL;					// by14 - peer channel
	uint8_t       P_CNT;					// by15 - peer count
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       UNKNOWN[2];				// by10 - to be evaluated
	uint8_t       TIME;						// by12 - time
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	struct s_BLL {
		uint8_t BUTTON : 6;
		uint8_t LONG : 1;
		uint8_t LOWBAT : 1;
	} BLL;									// by10 - button, long, lowbat
	uint8_t       COUNTER;					// by11 - time
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	struct s_BLL {
		uint8_t BUTTON : 6;
		uint8_t LONG   : 1;
		uint8_t LOWBAT : 1;
	} BLL;									// by10 - button, long, lowbat
	uint8_t       VALUE;					// by11 - sensor value
	uint8_t       COUNTER;					// by12 - counter
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       BUTTON;					// by10 - to be evaluated
	uint8_t       NBR;						// by11 - to be evaluated
	uint8_t       LEVEL;					// by12 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       CMD;						// by10 - to be evaluated
	uint8_t       FLD1;						// by11 - to be evaluated
	uint16_t      VAL1;						// by12 - to be evaluated
	uint8_t       FLD2;						// by14 - to be evaluated
	uint16_t      VAL2;						// by15 - to be evaluated
	uint8_t       FLD3;						// by17 - to be evaluated
	uint16_t      VAL3;						// by18 - to be evaluated
	uint8_t       FLD4;						// by20 - to be evaluated
	uint16_t      VAL4;						// by21 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[4];				// by10 - to be evaluated
	uint8_t       POWER[3];					// by14 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       CMD;						// by10 - to be evaluated
	uint8_t       VALVE;					// by11 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       CMD;						// by10 - to be evaluated
	uint8_t       TEMP_MODE;				// by11 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       SETTEMP;					// by10 - to be evaluated
	uint16_t      ACTTEMP;					// by11 - to be evaluated
	uint8_t       HUM;						// by13 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[3];				// by10 - to be evaluated
	uint8_t       POWER[3];					// by13 - to be evaluated
	uint8_t       CURRENT[2];				// by16 - to be evaluated
	uint8_t       VOLTAGE[2];				// by18 - to be evaluated
	uint8_t       FREQUENCY;				// by20 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint8_t       ENERGY[3];				// by10 - to be evaluated
	uint8_t       POWER[3];					// by13 - to be evaluated
	uint8_t       CURRENT[2];				// by16 - to be evaluated
	uint8_t       VOLTAGE[2];				// by18 - to be evaluated
	uint8_t       FREQUENCY;				// by20 - to be evaluated
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
	uint8_t       MSG_LEN;					// by00 - message length
	uint8_t       MSG_CNT;					// by01 - message counter
	struct s_mFlg FLAG;						// by02 - see structure of message flags
	uint8_t       MSG_TYP;					// by03 - type of message
	uint8_t       SND_ID[3];				// by04 - sender ID
	uint8_t       RCV_ID[3];				// by07 - receiver id, broadcast for 0
	uint16_t      TEMP;						// by10 - to be evaluated
	uint8_t       HUM;						// by12 - to be evaluated
} s_m70xxxx; // WEATHER_EVENT message

#endif