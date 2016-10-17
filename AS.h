/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin framework main class -------------------------------------------------------------------------------------------
* - with a lot of support from many people at FHEM forum
*   thanks a lot to martin876, dirk, pa-pa, martin, Dietmar63 and all i have not personal named here
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _NAS_H
#define _NAS_H

#include "AS_typedefs.h"
#include "HAL.h"
#include "macros.h"
#include "defines.h"
#include "version.h"
#include "wait_timer.h"

#include "cmMaster.h"
#include "CC1101.h"
#include "Send.h"
#include "Receive.h"
#include "ConfButton.h"
#include "StatusLed.h"
#include "Power.h"
#include "Battery.h"






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
public:		//-------------------------------------------------------------------------------------------------------------

	struct s_stcSlice {						// - send peers or reg in slices, store for send slice function
		uint8_t active;						// indicates status of poll routine, 1 is active
		uint8_t peer;						// is it a peer list message
		uint8_t reg2;						// or a register send
		uint8_t reg3;						// not implemented at the moment
		uint8_t totSlc;						// amount of necessary slices to send content
		uint8_t curSlc;						// counter for slices which are already send
		uint8_t cnl;						// indicates channel
		uint8_t lst;						// the respective list
		uint8_t idx;						// the peer index
		uint8_t mCnt;						// the message counter
		uint8_t toID[3];					// to whom to send
	} stcSlice;

	struct s_stcPeer {
		uint8_t active;//    :1;			// indicates status of poll routine, 1 is active
		uint8_t retries;//     :3;			// send retries
		uint8_t burst;//     :1;			// burst flag for send function
		uint8_t bidi;//      :1;			// ack required
		//uint8_t :2;
		uint8_t msg_type;					// message type to build the right message
		uint8_t *ptr_payload;				// pointer to payload
		uint8_t len_payload;				// length of payload
		uint8_t channel;					// which channel is the sender
		uint8_t idx_cur;					// current peer slots
		uint8_t idx_max;					// amount of peer slots
		uint8_t slot[8];					// slot measure, all filled in a first step, if ACK was received, one is taken away by slot
	} stcPeer;

	union {
		struct s_l4_0x01 {
			uint8_t  peerNeedsBurst:1;			// 0x01, s:0, e:1
			uint8_t  :6;
			uint8_t  expectAES:1;				// 0x01, s:7, e:8
		} s;
		uint8_t	ui;
	} l4_0x01;


	uint8_t  keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;
	uint8_t  signingRequestData[6];
	uint8_t  tempHmKey[16];
	uint8_t  newHmKey[16];
	uint8_t  newHmKeyIndex[1];
	uint16_t randomSeed = 0;
	uint8_t  resetStatus = 0;

  public:		//---------------------------------------------------------------------------------------------------------
	AS();																					// constructor

	/**
	 * @brief Initialize the AS module
	 *
	 * init() has to be called from the main setup() routine.
	 */
	void init(void);

	/**
	 * @brief Poll routine for regular operation
	 *
	 * poll() needs to be called regularily from the main loop(). It takes care of
	 * all major tasks like sending and receiving messages, device configuration
	 * and message delegation.
	 */
	void poll(void);


	/* - receive functions ------------------------------------------------------------------------------------------------
	* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
	* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
	* The intent is to overload them there by the respective user channel module and work with the information accordingly.
	*/
	void processMessage(void);

	//void DEVICE_INFO(s_m00xxxx *buf);														// in client comms not needed as receive function

	inline void CONFIG_PEER_ADD(s_m01xx01 *buf);											// mainly all needed to set or get information
	inline void CONFIG_PEER_REMOVE(s_m01xx02 *buf);											// for the defined device. all requests are
	inline void CONFIG_PEER_LIST_REQ(s_m01xx03 *buf);										// handled within AS main class
	inline void CONFIG_PARAM_REQ(s_m01xx04 *buf);
	inline void CONFIG_START(s_m01xx05 *buf);
	inline void CONFIG_END(s_m01xx06 *buf);
	inline void CONFIG_WRITE_INDEX1(s_m01xx07 *buf);
	inline void CONFIG_WRITE_INDEX2(s_m01xx08 *buf);
	inline void CONFIG_SERIAL_REQ(s_m01xx09 *buf);
	inline void CONFIG_PAIR_SERIAL(s_m01xx0a *buf);
	//void CONFIG_STATUS_REQUEST(s_m01xx0e *buf);											// forwarded to ptr_CM

	inline void ACK(s_m0200xx *buf);														// all different types of ACK are a response
	inline void ACK_STATUS(s_m0201xx *buf);													// to previous send messages, handled via  
	inline void ACK2(s_m0202xx *buf);														// AS main class only
	inline void AES_REQ(s_m0204xx *buf);
	inline void NACK(s_m0280xx *buf);
	inline void NACK_TARGET_INVALID(s_m0284xx *buf);
	inline void ACK_NACK_UNKNOWN(s_m02ffxx *buf);

	inline void AES_REPLY();

	inline void SEND_AES_TO_HMLAN();
	inline void SEND_AES_TO_ACTOR();
	
	//void INFO_SERIAL();																	// in client communication only as
	//void INFO_PEER_LIST();																// send functions required
	//void INFO_PARAM_RESPONSE_PAIRS();
	//void INFO_PARAM_RESPONSE_SEQ();
	//void INFO_PARAMETER_CHANGE();
	//void INFO_ACTUATOR_STATUS();
	//void INFO_TEMP();

	//void INSTRUCTION_INHIBIT_OFF();														// forwarded to ptr_CM
	//void INSTRUCTION_INHIBIT_ON();														// forwarded to ptr_CM
	//void INSTRUCTION_SET();																// forwarded to ptr_CM
	//void INSTRUCTION_STOP_CHANGE();														// forwarded to ptr_CM
	inline void INSTRUCTION_RESET();
	//void INSTRUCTION_LED();																// forwarded to ptr_CM
	//void INSTRUCTION_LED_ALL();															// forwarded to ptr_CM
	//void INSTRUCTION_LEVEL();																// forwarded to ptr_CM
	inline void INSTRUCTION_SLEEPMODE();
	inline void INSTRUCTION_ENTER_BOOTLOADER();
	//void INSTRUCTION_SET_TEMP();															// forwarded to ptr_CM
	//void INSTRUCTION_ADAPTION_DRIVE_SET();												// forwarded to ptr_CM
	inline void INSTRUCTION_ENTER_BOOTLOADER2();

	void HAVE_DATA();

	//void SWITCH();																		// peer related communication
	//void TIMESTAMP();																		// needed as send and receive function,
	//void REMOTE();																		// but all channel module related
	//void SENSOR_EVENT();																	// all forwarded to ptr_CM
	//void SWITCH_LEVEL();
	//void SENSOR_DATA();
	//void GAS_EVENT();
	//void CLIMATE_EVENT(); ,
	//void SET_TEAM_TEMP();
	//void THERMAL_CONTROL();
	//void POWER_EVENT_CYCLE();
	//void POWER_EVENT();
	//void WEATHER_EVENT();



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
	void send_INFO_ACTUATOR_STATUS();
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
	void send_REMOTE(s_peer_table *peerDB);													// will be send to the peerlist, therefor
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



	void sendPayload(uint8_t payloadType, uint8_t *data, uint8_t dataLen);
	inline void sendAckAES(uint8_t *data);
	void sendINFO_ACTUATOR_STATUS(uint8_t channel, uint8_t state, uint8_t flag);
	void sendINFO_POWER_EVENT(uint8_t *data);
	void sendINFO_TEMP(void);
	void sendHAVE_DATA(void);
	void sendSWITCH(void);
	void sendTimeStamp(void);
	void sendREMOTE(uint8_t channel, uint8_t *ptr_payload, uint8_t msg_flag = 0);
	void sendSensor_event(uint8_t channel, uint8_t burst, uint8_t *payload);
	void sendSensorData(void);
	void sendClimateEvent(void);
	void sendSetTeamTemp(void);
	void sendWeatherEvent(void);
	void sendEvent(uint8_t channel, uint8_t msg_type, uint8_t msg_flag, uint8_t *ptr_payload, uint8_t len_payload);

	void processMessageConfigAction(uint8_t by10, uint8_t cnl1);
	void processMessageAction11();
	void processMessageAction3E(uint8_t cnl, uint8_t pIdx);
	void deviceReset(uint8_t clearEeprom);

	uint8_t getChannelFromPeerDB(uint8_t *pIdx);

	void initPseudoRandomNumberGenerator();



  //private:		//---------------------------------------------------------------------------------------------------------

	inline void processMessageSwitchEvent();

	inline void processMessageResponseAES_Challenge(void);
	inline void processMessageResponseAES(void);
	inline void processMessageKeyExchange(void);
	uint8_t checkAnyChannelForAES(void);

	uint8_t processMessageConfig();
	inline void processMessageConfigStatusRequest(uint8_t by10);
	inline void processMessageConfigPairSerial(void);
	inline void processMessageConfigSerialReq(void);
	inline void processMessageConfigParamReq(void);
	//inline void processMessageConfigPeerListReq(void);
	inline void processMessageConfigAESProtected();

	inline void actionSwitchEvent();
	inline void configStart();
	inline void configEnd();
	inline void configWriteIndex(void);

	// - poll functions --------------------------------
	inline void sendSliceList(void);															// scheduler to send config messages, peers and regs
	inline void sendPeerMsg(void);																// scheduler for peer messages
	void preparePeerMessage(uint8_t *xPeer, uint8_t retr);
			

	// - send functions --------------------------------
	inline void sendINFO_SERIAL(void);
	inline void sendINFO_PEER_LIST(uint8_t len);
	inline void sendINFO_PARAM_RESPONSE_PAIRS(uint8_t len);
	void sendINFO_PARAM_RESPONSE_SEQ(uint8_t len);
	void sendINFO_PARAMETER_CHANGE(void);

	void prepareToSend(uint8_t mCounter, uint8_t mType, uint8_t *receiverAddr);

	// - AES Signing related methods -------------------
	void makeTmpKey(uint8_t *challenge);
	void payloadEncrypt(uint8_t *encPayload, uint8_t *msgToEnc);

	void sendSignRequest(uint8_t rememberBuffer);

	inline void initRandomSeed();
	
  protected:	//---------------------------------------------------------------------------------------------------------

	// - some helpers ----------------------------------


};

/*
* @brief Struct to hold the buffer for any send or received string with some flags for further processing
*/
extern s_recv rcv_msg;
extern s_send snd_msg;

/*
* @brief Global definition of a struct to hold the device identification related information.
*
* First bytes of eeprom holds all device specific information for identification. Struct is used
* to hold this information in memory on hand.
*  2 byte - magic byte
*  3 byte - homematic id
* 10 byte - serial number
*  1 byte - aes key index
* 16 byte - homematic aes key
*/
extern s_ee_start dev_ident;
/*
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

/*
* @brief Helper structure for keeping track of active pairing mode
*/
extern s_pair_mode pair_mode;
/*
* @brief Helper structure for keeping track of active config mode
*/
extern s_config_mode config_mode;

/*
* @brief Global definition of device HMSerialData. Must be declared in user space.
*
* The HMSerialData holds the default HMID, HMSerial and HMKEY.
* At every start, values of HMID and HMSerial was copied to related variables.
* The HKEY was only copied at initial sketch start in the EEprom
*/
extern const uint8_t HMSerialData[] PROGMEM;
/*
* @brief Settings of HM device
* firmwareVersion: The firmware version reported by the device
*                  Sometimes this value is important for select the related device-XML-File
*
* modelID:         Important for identification of the device.
*                  @See Device-XML-File /device/supported_types/type/parameter/const_value
*
* subType:         Identifier if device is a switch or a blind or a remote
* DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
*                  Other bytes not known.
*                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
*/
extern const uint8_t dev_static[] PROGMEM;

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


extern AS hm;



//- some helpers ----------------------------------------------------------------------------------------------------------
uint32_t byteTimeCvt(uint8_t tTime);
uint32_t intTimeCvt(uint16_t iTime);

inline uint8_t  isEmpty(void *ptr, uint8_t len);										// check if a byte array is empty
#define isEqual(p1,p2,len) memcmp(p1, p2, len)?0:1										// check if a byte array is equal

//- -----------------------------------------------------------------------------------------------------------------------





#endif

