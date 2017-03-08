/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin framework main class -------------------------------------------------------------------------------------------
* - with a lot of support from many people at FHEM forum
*   thanks a lot to martin876, dirk, pa-pa, martin, Dietmar63 and all i have not personal named here
*   special thank you to https://git.zerfleddert.de/hmcfgusb/AES/ for bidcos(R) AES explanation
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _AS_MAIN_H
#define _AS_MAIN_H

#include "as_type_defs.h"
#include "cm_master.h"

/**
 * @short Main class for implementation of the AskSin protocol stack.
 * Every device needs exactly one instance of this class.
 *
 * AS is responsible for maintaining the complete infrastructure from
 * device register representation, non-volatile storage of device configuration
 * in the eeprom, to providing interfaces for user modules that implement
 * actual device logic.
 *
 * This is a very simple, non-working example of the basic API used in the main
 * routines:
 * @include docs/snippets/basic-AS.cpp
 *
 * All send functions are used by sensor or actor classes like THSensor or Dimmer.
 */
class AS {

public:  //----------------------------------------------------------------------------------------------------------------
	AS() {}																					// constructor
	void init(void);																		// init function for the library
	void poll(void);



	/* - receive functions ------------------------------------------------------------------------------------------------
	* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
	* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
	* The intent is to overload them there by the respective user channel module and work with the information accordingly.
	*/
	inline void rcv_poll(void);																// poll function
	inline void get_intend(void);															// checks the received string if addresses are known
	inline void process_message(void);														// herein we sort out the message and forward to the respective functions

	inline void INSTRUCTION_RESET(s_m1104xx *buf);
	inline void INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf);
	inline void INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf);
	inline void INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf);



	/* - send functions ---------------------------------------------------------------------------------------------------
	* @brief Here are the send functions for the device, this library is focused on client communication,
	* so you will find send functions for client communication only. The send functions are sorted by
	* message type and sub type. Only for the peer related messages there is the need to define the receiver.
	* Configuration and status answers send only to HMID, ACK and subtypes are always the response to a received string
	*/
	inline void snd_poll(void);																// poll function, process if something is to send
	inline void process_list_message_poll(void);											// to answer peer and register list messages, because they are send in several strings due to the size
	inline void process_peer_message_poll(void);											// peer message poll function, details are in peer_msg struct


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
	void send_ACK_STATUS(uint8_t chnl, uint8_t stat, uint8_t flag, uint8_t sum = 0xff);		// to previous received messages 
	void send_ACK2(void);																	// RCV_ID will be taken in every case from the 
	void send_AES_REQ(void);// s_m0204xx *buf);												// previous received string
	void send_NACK(void);
	void send_NACK_TARGET_INVALID(void);
	void send_ACK_NACK_UNKNOWN();

	void send_AES_REPLY(uint8_t *payload);

	void send_SEND_AES_TO_HMLAN();
	void send_SEND_AES_TO_ACTOR();

	void send_INFO_SERIAL();																// in client communication only as
	void send_INFO_PEER_LIST(uint8_t cnl);													// send functions required
	void send_INFO_PARAM_RESPONSE_PAIRS(uint8_t cnl, uint8_t lst, uint8_t *peer_id);
	void send_INFO_PARAM_RESPONSE_SEQ(uint8_t cnl, uint8_t lst, uint8_t *peer_id);
	void send_INFO_PARAMETER_CHANGE();
	void send_INFO_ACTUATOR_STATUS(uint8_t chnl, uint8_t stat, uint8_t flag, uint8_t sum = 0xff);
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

	//void send_SWITCH(s_peer_table *peerDB);												// peer related communication
	void send_TIMESTAMP(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);		// needed as send and receive function
	void send_REMOTE(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);		// will be send to the peerlist, therefor
	void send_SENSOR_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);	// handover of the respective peerDB pointer
	void send_SWITCH_LEVEL(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);	// is mandatory
	void send_SENSOR_DATA(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_GAS_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_CLIMATE_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_SET_TEAM_TEMP(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_THERMAL_CONTROL(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_POWER_EVENT_CYCLE(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_POWER_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);
	void send_WEATHER_EVENT(uint8_t bidi, CM_MASTER *channel_module, uint8_t *ptr_payload);



	/* - asksin relevant helpers */
	inline uint8_t is_peer_valid(uint8_t *peer);											// search through all instances and ceck if we know the peer, returns the channel

};

/*
* @fn void everyTimeStart()
* @brief Callback for actions after bootup
*
* This function is called when AS has started and before the main loop runs.
*/
extern void everyTimeStart(void);

/*
* @fn void firstTimeStart()
* @brief Callback for actions after EEprom deletion
*
* This function needs to be defined in the user code space. It can be used to
* set the data of complete Lists with EE::setList() or single registers using
* EE::setListArray()
*/
extern void firstTimeStart(void);










#endif

