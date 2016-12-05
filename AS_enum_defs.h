//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin enum definition ------------------------------------------------------------------------------------------------
//- global definition of enums
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _AS_ENUMDEFS_H
#define _AS_ENUMDEFS_H


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
* @brief Type of list request answer
* PEER_LIST, as answer to a CONFIG_PEER_LIST_REQ
* PARAM_RESPONSE_PAIRS, as answer on a CONFIG_PARAM_REQ
* PARAM_RESPONSE_SEQ, as answer on a CONFIG_PARAM_REQ
*/
namespace LIST_ANSWER {
	enum E : uint8_t { NONE = 0, PEER_LIST = 1, PARAM_RESPONSE_PAIRS = 2, PARAM_RESPONSE_SEQ = 3, };
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





#endif