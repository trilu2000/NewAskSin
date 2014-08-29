//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _AS_H
#define _AS_H

#include "HAL.h"
#include "CC1101.h"
#include "EEprom.h"
#include "SndRcv.h"


class AS {
  public:		//---------------------------------------------------------------------------------------------------------
	EE ee;																					// load eeprom module
	CC cc;																					// load communication module

	struct s_msgBody rcv;																	// define the receive buffer as a struct
	uint8_t *rcvBuf = (uint8_t*)&rcv;														// buffer for received string

	struct s_msgBody snd;																	// define the send buffer by struct
	uint8_t *sndBuf = (uint8_t*)&snd;														// buffer for the send string
	uint8_t sndCnt;																			// message counter for standard sends, while not answering something

	struct s_sndStc {						// - struct for remember send status, for send function
		uint8_t active   :1;				// is send module active, 1 indicates yes
		uint8_t timeOut  :1;				// was last message a timeout
		uint8_t cntr;						// variable to count how often a message was already send
		uint8_t retr;						// how often a message has to be send until ACK
		uint8_t mCnt;						// store of message counter, needed to identify ACK
	} sndStc;

  protected:	//---------------------------------------------------------------------------------------------------------
	struct s_cnfFlag {						// - remember that we are in config mode, for config start message receive
		uint8_t active   :1;				// indicates status, 1 if config mode is active
		uint8_t cnl;						// channel
		uint8_t lst;						// list
		uint8_t idx;						// peer index
	} cnfFlag;

	struct s_slcList {						// - send peers or reg in slices, store for send slice function
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
	} slcList;
	
	struct s_peerMsg {
		uint8_t active   :1;				// indicates status of poll routine, 1 is active
		uint8_t rnd      :3;				// send retries
		uint8_t *pL;						// pointer to payload
		uint8_t lenPL;						// length of payload
		uint8_t cnl;						// which channel is the sender
		uint8_t curIdx;						// current peer slots
		uint8_t maxIdx;						// amount of peer slots
		struct s_mFlg mFlg;					// message flag
		uint8_t mTyp;						// message type
		uint8_t slt[8];						// slot measure, all filled in a first step, if ACK was received, one is taken away by slot
	} peerMsg;
	
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	AS();
	void init(void);																		// init the AS module

// - poll functions --------------------------------
	void poll(void);																		// poll routine for regular operation
	void sender(void);																		// send scheduler, to handle all send messages
	void sendSlcList(void);																	// scheduler to send config messages, peers and regs
	void sendPeerMsg(void);																	// scheduler for peer messages
	
// - received functions ----------------------------
	void received(void);																	// receiver function, all answers are generated here

// - send functions --------------------------------
	void sendDEVICE_INFO(void);													
 	void sendACK(void);
 	void sendACK_STATUS(void);
 	void sendNACK(void);
 	void sendNACK_TARGET_INVALID(void);
 	void sendINFO_SERIAL(void);
 	void sendINFO_PEER_LIST(uint8_t len);
 	void sendINFO_PARAM_RESPONSE_PAIRS(uint8_t len);
 	void sendINFO_PARAM_RESPONSE_SEQ(uint8_t len);
 	void sendINFO_PARAMETER_CHANGE(void);
 	void sendINFO_ACTUATOR_STATUS(void);
 	void sendINFO_TEMP(void);
 	void sendHAVE_DATA(void);
 	void sendSWITCH(void);
 	void sendTimeStamp(void);
 	void sendREMOTE(uint8_t cnl, uint8_t *pL);
 	void sendSensor_event(uint8_t cnl, uint8_t *pL);
 	void sendSensorData(void);
 	void sendClimateEvent(void);
 	void sendSetTeamTemp(void);
 	void sendWeatherEvent(void);
	
// - homematic specific functions ------------------
	void decode(uint8_t *buf);																// decodes the message
	void encode(uint8_t *buf);																// encodes the message
	void explainMessage(uint8_t *buf);														// explains message content, part of debug functions

// - some helpers ----------------------------------

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

};
extern AS hm;



class waitTimer {

  private:		//---------------------------------------------------------------------------------------------------------
	uint8_t  armed;
	uint32_t nexTime;

  public:		//---------------------------------------------------------------------------------------------------------
	waitTimer () {}

	uint8_t  done(void);
	void     set(uint32_t ms);
};

#endif

