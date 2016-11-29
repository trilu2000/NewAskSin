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

namespace INFO {
	enum E : uint8_t { NOTHING, SND_ACK_STATUS, SND_ACTUATOR_STATUS, SND_ACTUATOR_STATUS_AGAIN };
};


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
																							
	/*
	* @brief Keep all information for sending an ACK_STATUS or INFO_ACTUATOR_STATUS at one place
	*/

	virtual void info_config_change(void);													// list1 on registered channel had changed
	virtual void info_peer_add(s_m01xx01 *buf);												// peer was added to the specific channel, 1st 3 bytes shows peer address, 4th and 5th the peer channel
	virtual void request_peer_defaults(uint8_t idx, s_m01xx01 *buf);						// add peer channel defaults to list3/4

	void init(void);																		// init function, called after AS initialisation
	virtual void cm_init(void) {}															// init function for channel modules to overwrite

	
	/* virtual declaration for cmRemote channel module. make pin configuration and button event accessible */
	virtual void cm_init_pin(uint8_t PINBIT, volatile uint8_t *DDREG, volatile uint8_t *PORTREG, volatile uint8_t *PINREG, uint8_t PCINR, uint8_t PCIBYTE, volatile uint8_t *PCICREG, volatile uint8_t *PCIMASK, uint8_t PCIEREG, uint8_t VEC) {}
	virtual void button_action(uint8_t event) {}

	void poll(void);																		// poll function, driven by HM loop
	virtual void cm_poll(void) {}															// poll function for channel modules to overwrite

	virtual void set_toggle(void);															// toggle the module initiated by config button

	/* receive functions to handle requests forwarded by AS:processMessage 
	*  only channel module related requests are forwarded, majority of requests are handled within main AS class */

	/* - receive functions ------------------------------------------------------------------------------------------------
	* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
	* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
	* The intent is to overload them there by the respective user channel module and work with the information accordingly.
	*/

	//void DEVICE_INFO(s_m00xxxx *buf);														// in client comms not needed as receive function

	void CONFIG_PEER_ADD(s_m01xx01 *buf);													// mainly all needed to set or get information
	void CONFIG_PEER_REMOVE(s_m01xx02 *buf);												// for the defined device. all requests are
	void CONFIG_PEER_LIST_REQ(s_m01xx03 *buf);												// handled within the cmMaster class
	void CONFIG_PARAM_REQ(s_m01xx04 *buf);
	void CONFIG_START(s_m01xx05 *buf);
	void CONFIG_END(s_m01xx06 *buf);
	void CONFIG_WRITE_INDEX1(s_m01xx07 *buf);
	void CONFIG_WRITE_INDEX2(s_m01xx08 *buf);
	void CONFIG_SERIAL_REQ(s_m01xx09 *buf);
	void CONFIG_PAIR_SERIAL(s_m01xx0a *buf);
	virtual void CONFIG_STATUS_REQUEST(s_m01xx0e *buf);										// to be overwritten by a user specific channel module

	void ACK(s_m0200xx *buf);																// at the moment we need the ACK only to avoid
	void ACK_STATUS(s_m0201xx *buf);														// resends in the send class, but here we can make it virtual
	void ACK2(s_m0202xx *buf);																// to use it by the channel modules
	void AES_REQ(s_m0204xx *buf);
	void NACK(s_m0280xx *buf);
	void NACK_TARGET_INVALID(s_m0284xx *buf);
	void ACK_NACK_UNKNOWN(s_m02xxxx *buf);

	void AES_REPLY(s_m03xxxx *buf);

	void SEND_AES_TO_HMLAN(s_m0401xx *buf);
	void SEND_AES_TO_ACTOR(s_m04xxxx *buf);

	//void INFO_SERIAL(s_m1000xx *buf);														// in client communication only as
	//void INFO_PEER_LIST(s_m1001xx *buf);													// send functions required
	//void INFO_PARAM_RESPONSE_PAIRS(s_m1002xx *buf);
	//void INFO_PARAM_RESPONSE_SEQ(s_m1003xx *buf);
	//void INFO_PARAMETER_CHANGE(s_m1004xx *buf);
	//void INFO_ACTUATOR_STATUS(s_m1006xx *buf);
	//void INFO_TEMP(s_m100axx *buf);

	virtual void INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf);
	virtual void INSTRUCTION_INHIBIT_ON(s_m1101xx *buf);
	virtual void INSTRUCTION_SET(s_m1102xx *buf);
	virtual void INSTRUCTION_STOP_CHANGE(s_m1103xx *buf);
	void INSTRUCTION_RESET(s_m1104xx *buf);													// back to factory defaults
	virtual void INSTRUCTION_LED(s_m1180xx *buf);	
	virtual void INSTRUCTION_LED_ALL(s_m1181xx *buf);
	virtual void INSTRUCTION_LEVEL(s_m1181xx *buf);	
	virtual void INSTRUCTION_SLEEPMODE(s_m1182xx *buf);
	void INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf);
	virtual void INSTRUCTION_SET_TEMP(s_m1186xx *buf);										// to be evaluated
	void INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf);									// no idea about this message
	void INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf);

	void HAVE_DATA(s_m12xxxx *buf);

	virtual void SWITCH(s_m3Exxxx *buf);													// peer related communication
	virtual void TIMESTAMP(s_m3fxxxx *buf);													// needed as send and receive function,
	virtual void REMOTE(s_m40xxxx *buf);													// but all channel module related
	virtual void SENSOR_EVENT(s_m41xxxx *buf);												// all forwarded to ptr_CM
	virtual void SWITCH_LEVEL(s_m42xxxx *buf);
	virtual void SENSOR_DATA(s_m53xxxx *buf);
	virtual void GAS_EVENT(s_m54xxxx *buf);
	virtual void CLIMATE_EVENT(s_m58xxxx *buf);
	virtual void SET_TEAM_TEMP(s_m59xxxx *buf);
	virtual void THERMAL_CONTROL(s_m5axxxx *buf);
	virtual void POWER_EVENT_CYCLE(s_m5exxxx *buf);
	virtual void POWER_EVENT(s_m5fxxxx *buf);
	virtual void WEATHER_EVENT(s_m70xxxx *buf);


};


/* as there is no way to get the channel by setting up the pointer array for channel modules we use this
*  byte to identify the channel we are actually setting up, increased in the constructor...
   the overall amount will be kept for runtime to step through the different instances. */
extern uint8_t cnl_max;
extern cmMaster *ptr_CM[];





//- helpers ---------------------------------------------------------------------------------------------------------------
/*
* @brief Sends the ACK_STATUS and answers CONFIG_STATUS_REQUEST by sending an INFO_ACTUATOR_STATUS message
* As we dont need this function in all channel modules, it is defined outside of the master channel. To use this function set, 
* you have to define in the specific channel module a struct and poll the send_status function by handing over the defined
* struct and the channel information.
*/
typedef struct ts_cm_status {
	uint8_t   value;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t   set_value;																	// status to set on the Relay channel
	union {
		struct {
			uint8_t NA     : 3;
			uint8_t UP     : 1;
			uint8_t DOWN   : 1;
			uint8_t ERROR  : 1;
			uint8_t DELAY  : 1;
			uint8_t LOWBAT : 1;
		} f;
		uint8_t   flag;																		// module down up low battery byte
	};
	uint8_t   inhibit = 0;																	// store for inhibit message
	waitTimer delay;																		// delay timer for relay
	uint8_t	  message_type;																	// indicator for sendStatus function
	waitTimer message_delay;																// message timer for sending status
} s_cm_status;
void send_status(s_cm_status *cm, uint8_t cnl);												// help function to send status messages

uint16_t cm_prep_default(uint16_t ee_start_addr);											// prepare the defaults incl eeprom address mapping
uint8_t  is_peer_valid(uint8_t *peer);														// search through all instances and ceck if we know the peer, returns the channel

uint16_t cm_calc_crc(void);																	// calculate the crc for lists in the modules
inline uint16_t crc16_P(uint16_t crc, uint8_t len, const uint8_t *buf);						// calculates the crc for a PROGMEM byte array
inline uint16_t crc16(uint16_t crc, uint8_t a);												// calculates the crc for a given byte




/* - send functions ---------------------------------------------------------------------------------------------------
* @brief Here are the send functions for the device, this library is focused on client communication,
* so you will find send functions for client communication only. The send functions are sorted by
* message type and sub type. Only for the peer related messages there is the need to define the receiver.
* Configuration and status answers send only to HMID, ACK and subtypes are always the response to a received string
*/

void send_DEVICE_INFO(MSG_REASON::E reason);

//void send_CONFIG_PEER_ADD(s_m01xx01 *buf);											// in client communication not needed to send
//void send_CONFIG_PEER_REMOVE(s_m01xx02 *buf);											// this type of messages are raised from master only
//void send_CONFIG_PEER_LIST_REQ(s_m01xx03 *buf);
//void send_CONFIG_PARAM_REQ(s_m01xx04 *buf);
//void send_CONFIG_START(s_m01xx05 *buf);
//void send_CONFIG_END(s_m01xx06 *buf);
//void send_CONFIG_WRITE_INDEX1(s_m01xx07 *buf);
//void send_CONFIG_WRITE_INDEX2(s_m01xx08 *buf);
//void send_CONFIG_SERIAL_REQ(s_m01xx09 *buf);
//void send_CONFIG_PAIR_SERIAL(s_m01xx0a *buf);
//void send_CONFIG_STATUS_REQUEST(s_m01xx0e *buf);		

void check_send_ACK_NACK(uint8_t ackOk);												// ACK or NACK on base of ackok
void send_ACK(void);																	// all different types of ACK are a response
void send_ACK_STATUS(uint8_t chnl, uint8_t stat, uint8_t actn);							// to previous received messages 
void send_ACK2(void);																	// RCV_ID will be taken in every case from the 
void send_AES_REQ(s_m0204xx *buf);														// previous received string
void send_NACK(void);
void send_NACK_TARGET_INVALID(void);
void send_ACK_NACK_UNKNOWN();

void send_AES_REPLY();

void send_SEND_AES_TO_HMLAN();
void send_SEND_AES_TO_ACTOR();

void send_INFO_SERIAL();																// in client communication only as
void send_INFO_PEER_LIST(uint8_t cnl);													// send functions required
void send_INFO_PARAM_RESPONSE_PAIRS(uint8_t cnl, uint8_t lst, uint8_t *peer_id);
void send_INFO_PARAM_RESPONSE_SEQ(uint8_t cnl, uint8_t lst, uint8_t *peer_id);
void send_INFO_PARAMETER_CHANGE();
void send_INFO_ACTUATOR_STATUS(uint8_t cnl, uint8_t stat, uint8_t flag);
void send_INFO_TEMP();

//void send_INSTRUCTION_INHIBIT_OFF();													// not needed in client communication to send
//void send_INSTRUCTION_INHIBIT_ON();													// this type of messages are send by the HM master only
//void send_INSTRUCTION_SET();		
//void send_INSTRUCTION_STOP_CHANGE();
//void send_INSTRUCTION_RESET();
//void send_INSTRUCTION_LED();
//void send_INSTRUCTION_LED_ALL();
//void send_INSTRUCTION_LEVEL();	
//void send_INSTRUCTION_SLEEPMODE();
//void send_INSTRUCTION_ENTER_BOOTLOADER();
//void send_INSTRUCTION_SET_TEMP();		
//void send_INSTRUCTION_ADAPTION_DRIVE_SET();	
//void send_INSTRUCTION_ENTER_BOOTLOADER2();

void send_HAVE_DATA();

void send_SWITCH(s_peer_table *peerDB);													// peer related communication
void send_TIMESTAMP(s_peer_table *peerDB);												// needed as send and receive function
void send_REMOTE(s_peer_table *peerDB, s_list_table *listP, uint8_t *payload, uint8_t bidi); // will be send to the peerlist, therefor
void send_SENSOR_EVENT(s_peer_table *peerDB);											// handover of the respective peerDB pointer
void send_SWITCH_LEVEL(s_peer_table *peerDB);											// is mandatory
void send_SENSOR_DATA(s_peer_table *peerDB);
void send_GAS_EVENT(s_peer_table *peerDB);
void send_CLIMATE_EVENT(s_peer_table *peerDB);
void send_SET_TEAM_TEMP(s_peer_table *peerDB);
void send_THERMAL_CONTROL(s_peer_table *peerDB);
void send_POWER_EVENT_CYCLE(s_peer_table *peerDB);
void send_POWER_EVENT(s_peer_table *peerDB);
void send_WEATHER_EVENT(s_peer_table *peerDB);


#endif