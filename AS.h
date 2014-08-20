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
#include "CC.h"
#include "EE.h"
#include "RV.h"
#include "SN.h"


class AS {
  public:		//---------------------------------------------------------------------------------------------------------
	EE ee;																					// load eeprom module
	CC cc;																					// load communication module

	struct s_msgBody rcv;																	// cast the receive buffer to a struct
	uint8_t *rcvBuf = (uint8_t*)&rcv;														// buffer for received string

	struct s_msgBody snd;																	// define the send buffer by struct
	uint8_t *sndBuf = (uint8_t*)&snd;														// buffer for the send string
	
  protected:	//---------------------------------------------------------------------------------------------------------
	struct s_peerList {
		uint8_t active   :1;
		uint8_t peer     :1;
		uint8_t reg2     :1;
		uint8_t reg3     :1;
		uint8_t totSlc;
		uint8_t curSlc;
		uint8_t cnl;
		uint8_t lst;
		uint8_t idx;
		uint8_t mCnt;
		uint8_t toID[3];
	} slcList;

	struct s_cnfFlag {
		uint8_t active   :1;
		uint8_t cnl;
		uint8_t lst;
		uint8_t idx;
	} cnfFlag;

	struct s_sndStc {
		uint8_t active   :1;
		uint8_t timeOut  :1;
		uint8_t cntr;
		uint8_t retr;
		uint8_t mCnt;
	} sndStc;
	
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	AS();
	void init(void);

// - poll functions --------------------------------
	void poll(void);
	void sender(void);
	void sendSlcList(void);
	
// - received functions ----------------------------
	void received(void);

// - send functions --------------------------------
 	void sendACK(void);
 	void sendACK_STATUS(void);
 	void sendNACK(void);
 	void sendNACK_TARGET_INVALID(void);
 	void sendINFO_SERIAL(void);
 	void sendINFO_PEER_LIST(uint8_t len);
 	void sendINFO_PARAM_RESPONSE_PAIRS(void);
 	void sendINFO_PARAM_RESPONSE_SEQ(void);
 	void sendINFO_PARAMETER_CHANGE(void);
 	void sendINFO_ACTUATOR_STATUS(void);
 	void sendINFO_TEMP(void);
 	void sendHAVE_DATA(void);
 	void sendSWITCH(void);
 	void sendTimeStamp(void);
 	void sendREMOTE(void);
 	void sendSensor_event(void);
 	void sendSensorData(void);
 	void sendClimateEvent(void);
 	void sendSetTeamTemp(void);
 	void sendWeatherEvent(void);
	
// - homematic specific functions ------------------
	void decode(uint8_t *buf);																// decodes the message
	void encode(uint8_t *buf);																// encodes the message
	void explainMessage(uint8_t *buf);

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

};
extern AS hm;

class MilliTimer {
	// http://jeelabs.net/pub/docs/jeelib/Ports_8h_source.html
  private:		//---------------------------------------------------------------------------------------------------------
	uint16_t next;
	byte armed;

  public:		//---------------------------------------------------------------------------------------------------------
	MilliTimer () : armed (0) {}

	uint8_t  poll(uint16_t ms =0);
	uint16_t remaining() const;
	uint8_t  idle() const { return !armed; }
	void     set(uint16_t ms);
};

#endif

