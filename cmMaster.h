/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel master functions ---------------------------------------------------------------------------------------
* - idea for the master class from Dietmar63 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmMaster_H
#define _cmMaster_H

#include "AS.h"
#include "HAL.h"
#include "AS_typedefs.h"

const uint8_t list_max = 5;

class cmMaster {
public://------------------------------------------------------------------------------------------------------------------

	/*
	* @brief Every channel has two lists, the first list holds the configuration which is required to drive the channel,
	*        the second list is related to peer messages and holds all information which are required to drive the functionality
	*        of the channel in combination with peer devices.
	*        Therefore we have in every channel two lists - lstC and lstP organized in structs
	*/
	s_list_table *list[list_max] = {};
	s_list_table lstC;
	s_list_table lstP;

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
	s_peer_table peerDB;

	cmMaster(const uint8_t peer_max);														// constructor

	virtual void message_trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime);	// pair set message
	virtual void message_trigger3E(uint8_t msgLng, uint8_t msgCnt);							// switch message, also config test button in HM
	virtual void message_trigger40(uint8_t msgLng, uint8_t msgCnt);							// remote messages from peer
	virtual void message_trigger41(uint8_t msgLng, uint8_t msgCnt, uint8_t msgVal);			// sensor messages from peer

	virtual void info_config_change(void);													// list1 on registered channel had changed
	virtual void info_peer_add(s_m01xx01 *buf);												// peer was added to the specific channel, 1st 3 bytes shows peer address, 4th and 5th the peer channel

	virtual void request_peer_defaults(uint8_t idx, s_m01xx01 *buf);						// add peer channel defaults to list3/4
	virtual void request_pair_status(void);													// event on status request

	void poll(void);																		// poll function, driven by HM loop
	virtual void cm_poll(void) {}															// poll function for channel modules to overwrite
	void process_config_list_answer_slice(void);											// poll the config_list_answer_slice struct for preparing answers on peer and param list requests

	virtual void set_toggle(void);															// toggle the module initiated by config button


	void CONFIG_PEER_ADD(     s_m01xx01 *buf);
	void CONFIG_PEER_REMOVE(  s_m01xx02 *buf);
	void CONFIG_PEER_LIST_REQ(s_m01xx03 *buf);
	void CONFIG_PARAM_REQ(    s_m01xx04 *buf);
	void CONFIG_START(        s_m01xx05 *buf);
	void CONFIG_END(          s_m01xx06 *buf);
	void CONFIG_WRITE_INDEX1( s_m01xx07 *buf);
	void CONFIG_WRITE_INDEX2( s_m01xx08 *buf);
	void CONFIG_SERIAL_REQ(   s_m01xx09 *buf);
	void CONFIG_PAIR_SERIAL(  s_m01xx0a *buf);
	virtual void CONFIG_STATUS_REQUEST(s_m01xx0e *buf);

};

extern s_config_list_answer_slice config_list_answer_slice;

/* as there is no way to get the channel by setting up the pointer array for channel modules we use this
*  byte to identify the channel we are actually setting up, increased in the constructor...
   the overall amount will be kept for runtime to step through the different instances. */
extern uint8_t cnl_max;
extern cmMaster *ptr_CM[];


//- send functions --------------------------------------------------------------------------------------------------------
void send_DEVICE_INFO(uint8_t *RCV_ID);																// 0x00 ff ff 1a * DEVICE_INFO

void check_send_ACK_NACK(uint8_t *RCV_ID, uint8_t ackOk);													// ACK or NACK on base of ackok
void send_ACK(uint8_t *RCV_ID);																		// 0x02 00 ff 0a * - ACK
void send_ACK_STATUS(uint8_t *RCV_ID, uint8_t chnl, uint8_t stat, uint8_t actn);								// 0x02 01 ff 0e * - ACK_STATUS
void send_ACK2(uint8_t *RCV_ID);																		// 0x02 02 ff 0a * - ACK2
void send_AES_REQ(uint8_t *RCV_ID);																	// 0x02 04 ff 11 * - AES_REQ
void send_NACK(uint8_t *RCV_ID);																		// 0x02 80 ff 0a * - NACK
void send_NACK_TARGET_INVALID(uint8_t *RCV_ID);														// 0x02 84 ff 0a * - NACK_TARGET_INVALID
void send_ACK_AES(uint8_t *RCV_ID);																	// 0x02 ff ff 0e * - ACK_AES
void send_ACK_NACK_UNKNOWN(uint8_t *RCV_ID);															// 0x02 ff ff 09 * - ACK_NACK_UNKNOWN, short string no byte 10 value

void send_AES_REPLY(void);																	// 0x03 ff ff 09 * AES_REPLY

void send_SEND_AES_TO_HMLAN(void);															// 0x04 01 ff 0c * -SEND_AES_TO_HMLAN
void send_SEND_AES_TO_ACTOR(void);															// 0x04 ff ff 09 * -SEND_AES_TO_ACTOR, no byte 10?

void send_INFO_SERIAL(void);																// 0x10 00 ff 14 * - INFO_SERIAL
void send_INFO_PEER_LIST(uint8_t *RCV_ID, uint8_t payload_len);								// 0x10 01 ff ff * - INFO_PEER_LIST, no fix length
void send_INFO_PARAM_RESPONSE_PAIRS(void);													// 0x10 02 ff ff * - INFO_PARAM_RESPONSE_PAIRS, no fix length
void send_INFO_PARAM_RESPONSE_SEQ(void);													// 0x10 03 ff ff * - INFO_PARAM_RESPONSE_SEQ, no fix length
void send_INFO_PARAMETER_CHANGE(void);														// 0x10 04 ff ff * - INFO_PARAMETER_CHANGE, no fix length
void send_INFO_ACTUATOR_STATUS(void);														// 0x10 06 ff 0e * - INFO_ACTUATOR_STATUS
void send_INFO_TEMP(void);																	// 0x10 0A ff 0d * - INFO_TEMP

void send_HAVE_DATA(void);																	// 0x12 ff ff 09 * HAVE_DATA = 0x12

void send_SWITCH(void);																		// 0x3e ff ff 0f * SWITCH = 0x3e
void send_TIMESTAMP(void);																	// 0x3f ff ff 0c * TIMESTAMP = 0x3f
void send_REMOTE(void);																		// 0x40 ff ff 0b * REMOTE = 0x40
void send_SENSOR_EVENT(void);																// 0x41 ff ff 0c * SENSOR_EVENT = 0x41
void send_SWITCH_LEVEL(void);																// 0x42 ff ff 0c * SWITCH_LEVEL = 0x42
void send_SENSOR_DATA(void);																// 0x53 ff ff 16 * SENSOR_DATA = 0x53
void send_GAS_EVENT(void);																	// 0x54 ff ff 10 * GAS_EVENT
void send_CLIMATE_EVENT(void);																// 0x58 ff ff 0b * CLIMATE_EVENT
void send_SET_TEAM_TEMP(void);																// 0x59 ff ff 0b * SET_TEAM_TEMP
void send_THERMAL_CONTROL(void);															// 0x5a ff ff 0d * THERMAL_CONTROL
void send_POWER_EVENT_CYCLE(void);															// 0x5e ff ff 14 * POWER_EVENT_CYCLE
void send_POWER_EVENT(void);																// 0x5f ff ff 14 * POWER_EVENT
void send_WEATHER_EVENT(void);																// 0x70 ff ff 0c * WEATHER_EVENT



//- helpers ---------------------------------------------------------------------------------------------------------------
uint16_t cm_prep_default(uint16_t ee_start_addr);											// prepare the defaults incl eeprom address mapping
uint8_t  is_peer_valid(uint8_t *peer);														// search through all instances and ceck if we know the peer, returns the channel

uint16_t cm_calc_crc(void);																	// calculate the crc for lists in the modules
inline uint16_t crc16_P(uint16_t crc, uint8_t len, const uint8_t *buf);						// calculates the crc for a PROGMEM byte array
inline uint16_t crc16(uint16_t crc, uint8_t a);												// calculates the crc for a given byte

#endif