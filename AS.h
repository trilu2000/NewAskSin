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
#include "Send.h"
#include "Receive.h"


class AS {
  public:		//---------------------------------------------------------------------------------------------------------
	EE ee;																					// load eeprom module
	CC cc;																					// load communication module
	SN sn;
	RV rv;

  protected:	//---------------------------------------------------------------------------------------------------------
	struct s_confFlag {						// - remember that we are in config mode, for config start message receive
		uint8_t active   :1;				// indicates status, 1 if config mode is active
		uint8_t cnl;						// channel
		uint8_t lst;						// list
		uint8_t idx;						// peer index
	} cFlag;

	struct s_sliceList {					// - send peers or reg in slices, store for send slice function
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
	} sList;
	
	struct s_peMsg {
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
	} peMsg;
	
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	AS();
	void init(void);																		// init the AS module


// - poll functions --------------------------------
	void poll(void);																		// poll routine for regular operation
	void sendSliceList(void);																// scheduler to send config messages, peers and regs
	void sendPeerMsg(void);																	// scheduler for peer messages

	
// - receive functions -----------------------------	// by03, by10, by11
	void recvDEVICE_INFO(void);							//   00

	void recvCONFIG_PEER_ADD(void);						//   01          01
	void recvCONFIG_PEER_REMOVE(void);					//   01          02
	void recvCONFIG_PEER_LIST_REQ(void);				//   01          03
	void recvCONFIG_PARAM_REQ(void);					//   01          04
	void recvCONFIG_START(void);						//   01          05
	void recvCONFIG_END(void);							//   01          06
	void recvCONFIG_WRITE_INDEX(void);					//   01          08
	void recvCONFIG_SERIAL_REQ(void);					//   01          09
	void recvPAIR_SERIAL(void);							//   01          0A
	void recvCONFIG_STATUS_REQUEST(void);				//   01          0E
	
	void recvACK(void);									//   02    00
	void recvACK_STATUS(void);							//   02    01
	void recvACK2(void);								//   02    02
	void recvACK_PROC(void);							//   02    04
	void recvNACK(void);								//   02    80
	void recvNACK_TARGET_INVALID(void);					//   02    84

	void recvSET(void);									//   11    02
	void recvSTOP_CHANGE(void);							//   11    03
	void recvRESET(void);								//   11    04    00
	void recvLED(void);									//   11    80
	void recvLEDALL(void);								//   11    81    00
	void recvLEVEL(void);								//   11    81
	void recvSLEEPMODE(void);							//   11    82

	void recvHAVE_DATA(void);							//   12
	void recvSWITCH(void);								//   3E
	void recvTIMESTAMP(void);							//   3F
	void recvREMOTE(void);								//   40
	void recvSENSOR_EVENT(void);						//   41
	void recvSENSOR_DATA(void);							//   53
	void recvCLIMATE_EVENT(void);						//   58
	void recvWEATHER_EVENT(void);						//   70


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
 	void sendREMOTE(uint8_t cnl, uint8_t burst, uint8_t *pL);
 	void sendSensor_event(uint8_t cnl, uint8_t burst, uint8_t *pL);
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

