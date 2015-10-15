//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _NAS_H
#define _NAS_H

#include "HAL.h"
#include "CC1101.h"
#include "EEprom.h"
#include "Send.h"
#include "Receive.h"
#include "Registrar.h"
#include "ConfButton.h"
#include "StatusLed.h"
#include "Power.h"
#include "Battery.h"
#include "Version.h"

#define SUPPORT_AES                           1

#ifdef SUPPORT_AES
	#include "aes.h"
#endif

#define AS_MESSAGE_TYPE_DEVINFO               0x00
#define AS_MESSAGE_TYPE_CONFIG                0x01
#define AS_MESSAGE_TYPE_RESPONSE              0x02
#define AS_MESSAGE_TYPE_RESPONSE_AES          0x03
#define AS_MESSAGE_TYPE_KEY_EXCHANGE          0x04
#define AS_MESSAGE_TYPE_INFO                  0x10
#define AS_MESSAGE_TYPE_SET                   0x11
#define AS_MESSAGE_TYPE_HAVE_DATA             0x12
#define AS_MESSAGE_TYPE_SWITCH_EVENT          0x3E
#define AS_MESSAGE_TYPE_TIMESTAMP             0x3F
#define AS_MESSAGE_TYPE_REMOTE_EVENT          0x40
#define AS_MESSAGE_TYPE_SENSOR_EVENT          0x41
#define AS_MESSAGE_TYPE_SENSOR_DATA           0x53
#define AS_MESSAGE_TYPE_CLIMATE_EVENT         0x58
#define AS_MESSAGE_TYPE_WEATHER_EVENT         0x70

#define AS_CONFIG_PEER_ADD                    0x01
#define AS_CONFIG_PEER_REMOVE                 0x02
#define AS_CONFIG_PEER_LIST_REQ               0x03
#define AS_CONFIG_PARAM_REQ                   0x04
#define AS_CONFIG_START                       0x05
#define AS_CONFIG_END                         0x06
#define AS_CONFIG_WRITE_INDEX                 0x08
#define AS_CONFIG_SERIAL_REQ                  0x09
#define AS_CONFIG_PAIR_SERIAL                 0x0A
#define AS_CONFIG_STATUS_REQUEST              0x0E

#define AS_RESPONSE_TYPE_ACK                  0x00
#define AS_RESPONSE_TYPE_ACK_STATUS           0x01
#define AS_RESPONSE_TYPE_ACK2                 0x02
#define AS_RESPONSE_TYPE_AES_CHALLANGE        0x04
#define AS_RESPONSE_TYPE_NACK                 0x80
#define AS_RESPONSE_TYPE_NACK_TARGET_INVALID  0x84

#define AS_BUTTON_BYTE_KEY_BITS               0b00111111
#define AS_BUTTON_BYTE_LONGPRESS_BIT          0b01000000
#define AS_BUTTON_BYTE_LOWBAT_BIT             0b10000000

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
	friend class SN;
	friend class RV;
	friend class RG;
	friend class PW;

  public:		//---------------------------------------------------------------------------------------------------------
	EE ee;			///< eeprom module
	SN sn;			///< send module
	RG rg;			///< user module registrar
	CB confButton;		///< config button
	LD ld;			///< status led
	PW pw;			///< power management
	CC cc;			///< load communication module
	BT bt;

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

	//CC cc;		///< load communication module
	RV rv;			///< receive module

	/** @brief Helper structure for keeping track of active config mode */
	struct s_confFlag {						// - remember that we are in config mode, for config start message receive
		uint8_t  active   :1;				//< indicates status, 1 if config mode is active
		uint8_t  cnl;						//< channel
		uint8_t  lst;						//< list
		uint8_t  idx;						//< peer index
	} cFlag;

	struct s_stcSlice {						// - send peers or reg in slices, store for send slice function
		uint8_t active   :1;				// indicates status of poll routine, 1 is active
		uint8_t peer     :1;				// is it a peer list message
		uint8_t reg2     :1;				// or a register send
		uint8_t reg3     :1;				// not implemented at the moment
		uint8_t totSlc;						// amount of necessary slices to send content
		uint8_t curSlc;						// counter for slices which are already send
		uint8_t cnl;						// indicates channel
		uint8_t lst;						// the respective list
		uint8_t idx;						// the peer index
		uint8_t mCnt;						// the message counter
		uint8_t toID[3];					// to whom to send
	} stcSlice;

	struct s_stcPeer {
		uint8_t active   :1;				// indicates status of poll routine, 1 is active
		uint8_t rnd      :3;				// send retries
		uint8_t burst    :1;				// burst flag for send function
		uint8_t bidi     :1;				// ack required
		uint8_t mTyp;						// message type to build the right message
		uint8_t *pL;						// pointer to payload
		uint8_t lenPL;						// length of payload
		uint8_t cnl;						// which channel is the sender
		uint8_t curIdx;						// current peer slots
		uint8_t maxIdx;						// amount of peer slots
		uint8_t slt[8];						// slot measure, all filled in a first step, if ACK was received, one is taken away by slot
	} stcPeer;

	struct s_l4_0x01 {
		uint8_t  peerNeedsBurst      :1;     // 0x01, s:0, e:1
		uint8_t                      :6;     //
		uint8_t  expectAES           :1;     // 0x01, s:7, e:8
	} l4_0x01;

	uint8_t pairActive    :1;

	#ifdef SUPPORT_AES
		uint8_t  signingRequestData[6];
		uint8_t  tempHmKey[16];
		uint8_t  newHmKey[16];
		uint8_t  hmKeyIndex = 0;
		uint16_t randomSeed = 0;

		aes128_ctx_t ctx; 					// the context where the round keys are stored
	#endif

  public:		//---------------------------------------------------------------------------------------------------------
	AS();

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

	// - send functions --------------------------------
	void sendDEVICE_INFO(void);
	void sendACK(void);
	void checkSendACK(uint8_t ackOk);
	void sendPayload(uint8_t payloadType, uint8_t *data, uint8_t dataLen);
	void sendAckAES(uint8_t *data);
	void sendACK_STATUS(uint8_t channel, uint8_t state, uint8_t action);
	void sendNACK(void);
	void sendNACK_TARGET_INVALID(void);
	void sendINFO_ACTUATOR_STATUS(uint8_t channel, uint8_t state, uint8_t flag);
	void sendINFO_TEMP(void);
	void sendHAVE_DATA(void);
	void sendSWITCH(void);
	void sendTimeStamp(void);
	void sendREMOTE(uint8_t channel, uint8_t burst, uint8_t *payload);
	void sendSensor_event(uint8_t channel, uint8_t burst, uint8_t *payload);
	void sendSensorData(void);
	void sendClimateEvent(void);
	void sendSetTeamTemp(void);
	void sendWeatherEvent(void);
	void sendEvent(uint8_t channel, uint8_t burst, uint8_t mType, uint8_t *payload, uint8_t pLen);

  private:		//---------------------------------------------------------------------------------------------------------

	// - poll functions --------------------------------
	void sendSliceList(void);																// scheduler to send config messages, peers and regs
	void sendPeerMsg(void);																	// scheduler for peer messages
	void prepPeerMsg(uint8_t *xPeer, uint8_t retr);
			
	// - receive functions -----------------------------
	void recvMessage(void);

	// - send functions --------------------------------
	void sendINFO_SERIAL(void);
	void sendINFO_PEER_LIST(uint8_t len);
	void sendINFO_PARAM_RESPONSE_PAIRS(uint8_t len);
	void sendINFO_PARAM_RESPONSE_SEQ(uint8_t len);
	void sendINFO_PARAMETER_CHANGE(void);

	void prepareToSend(uint8_t mCnt, uint8_t mTyp, uint8_t *addrTo);

	// - AES Signing related methods -------------------
	void makeTmpKey(uint8_t *challenge);
	void payloadEncrypt(uint8_t *encPayload, uint8_t *msgToEnc);
	uint8_t checkPayloadDecrypt (uint8_t *data, uint8_t *msgOriginal);

	void sendSignRequest(void);
	void sendSignResponse(void);

	void getRandomBytes(uint8_t *buffer, uint8_t length);
	void initRandomSeed();

	
  protected:	//---------------------------------------------------------------------------------------------------------
	// - homematic specific functions ------------------
	void decode(uint8_t *buf);																// decodes the message
	void encode(uint8_t *buf);																// encodes the message
	void explainMessage(uint8_t *buf);														// explains message content, part of debug functions

	// - some helpers ----------------------------------


};
extern AS hm;

/**
 * @short Timer class for non-blocking delays
 *
 * The following examples shows how to use the waitTimer class to
 * perform an action every 500ms. Note that the first time loop()
 * is called, delay.done() will return true and the action will
 * be performed. The example also shows how to avoid the execution
 * time of the action to influence the new delay time by setting 
 * the delay before performing the action.
 * @code
 * void loop()
 * {
 *     static waitTimer delay;
 *     if (delay.done()) {
 *         delay.set(500); // perform next action after 500ms
 *         // do something now
 *     }
 * }
 * @endcode
 *
 * @attention The waitTimer can only make sure a minimum time passes
 * between set() and done(). If calls to done() are delayed due to other
 * actions, more time may pass. Also, actual delay times strongly depend 
 * on the behaviour of the system clock.
 *
 * @see http://www.gammon.com.au/forum/?id=12127
 */
class waitTimer {

  private:		//---------------------------------------------------------------------------------------------------------
	uint8_t  armed;
	uint32_t checkTime;
	uint32_t startTime;

  public:		//---------------------------------------------------------------------------------------------------------
	uint8_t  done(void);
	void     set(uint32_t ms);
	uint32_t remain(void);
};



uint32_t byteTimeCvt(uint8_t tTime);
uint32_t intTimeCvt(uint16_t iTime);

#endif

