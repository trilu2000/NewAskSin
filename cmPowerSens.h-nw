//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin cmPowerSens class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _CM_POWER_SENS_H
#define _CM_POWER_SENS_H
#define CLASS_NAME cmPowerSens


#include "AS.h"
#include "HAL.h"
#include "cm.h"
#include "InfraredSignalDetector.h"

/*
 * default settings for list3 or list4
 * TODO: to be checked!
 */

const uint8_t peerEven[] =    {		// cnl 2, 4, 6
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x01,0x12,0x22,0x23,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63,
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x24,0x12,0x22,0x23,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};

const uint8_t peerOdd[] =   {		// cnl 1, 3, 5
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x01,0x44,0x54,0x64,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63,
	0x00,0x00,0x00,0x32,0x64,0x00,0x0A,0x00,0xFF,0xA5,0x44,0x54,0x64,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};

const uint8_t peerSingle[] = {
	0x00,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x01,0x14,0x52,0x63,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63,
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x26,0x14,0x52,0x63,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};

class cmPowerSens {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------

	struct s_cnl0lst0 {
	      uint8_t INTERNAL_KEYS_VISIBLE                :1;    // 0x02.7, 0x01
	      uint8_t MASTER_ID_BYTE_1                     :8;    // 0x0a.0, 0x00
	      uint8_t MASTER_ID_BYTE_2                     :8;    // 0x0b.0, 0x00
	      uint8_t MASTER_ID_BYTE_3                     :8;    // 0x0c.0, 0x00
	      uint8_t LOW_BAT_LIMIT                        :8;    // 0x12.0, 0x69
	      uint8_t LOCAL_RESET_DISABLE                  :1;    // 0x18.0, 0x00
	      uint8_t                                      :7;    // 0x18.1
	   };


	/*
	 * Contains all register from list 1
	 */
	   struct s_lstCnl {
	      uint8_t AES_FLAG                             :1;    // 0x08.0, 0x00
	      uint8_t                                      :7;    // 0x08.1
	      uint8_t METER_TYPE                           :8;    // 0x95.0, 0x03
	      uint8_t METER_CONSTANT_IR1                    :8;    // 0x96.0, 0x64
	      uint8_t METER_CONSTANT_IR2                    :8;    // 0x97.0, 0x00
	      uint8_t METER_CONSTANT_GAS1                   :8;    // 0x98.0, 0x0a
	      uint8_t METER_CONSTANT_GAS2                   :8;    // 0x99.0, 0x00
	      uint8_t METER_CONSTANT_LED1                   :8;    // 0x9a.0, 0x10
	      uint8_t METER_CONSTANT_LED2                   :8;    // 0x9b.0, 0x27
	      uint8_t METER_SENSIBILITY_IR                 :8;    // 0x9c.0, 0x00
	   } lstCnl;

/*	struct s_lstCnl {
		// 0x08, 0x0b, 0x0d, 0x0f, 0x10, 0x30, 0x57,
		uint8_t  AES_ACTIVE                        :1;       // 0x08, s:0, e:1
		uint8_t                                    :7;       // 0x08, s:1, e:8
		uint16_t REFERENCE_RUNNING_TIME_TOP_BOTTOM :16;      // 0x0b, s:0, e:16
		uint16_t REFERENCE_RUNNING_TIME_BOTTOM_TOP :16;      // 0x0d, s:0, e:16
		uint8_t  CHANGE_OVER_DELAY                 :8;       // 0x0F, s:0, e:8
		uint8_t  REFERENCE_RUN_COUNTER             :8;       // 0x10, s:0, e:8
		uint8_t  TRANSMIT_TRY_MAX                  :8;       // 0x30, s:0, e:8
		uint8_t  STATUSINFO_MINDELAY               :5;       // 0x57, s:0, e:5
		uint8_t  STATUSINFO_RANDOM                 :3;       // 0x57, s:5, e:8
	} lstCnl;*/

	/*
	 * Contains all register from list 3
	 */
	struct s_lstPeer {
		// 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x11, 0x1c,
		// 0x1d, 0x1e, 0x1f, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d,
		// 0x8f, 0x91, 0x9c, 0x9d, 0x9e, 0x9f,
		uint8_t SHORT_CT_RAMPON           :4;       // 0x01, s:0, e:4
		uint8_t SHORT_CT_RAMPOFF          :4;       // 0x01, s:4, e:8
		uint8_t SHORT_CT_ONDELAY          :4;       // 0x02, s:0, e:4
		uint8_t SHORT_CT_OFFDELAY         :4;       // 0x02, s:4, e:8
		uint8_t SHORT_CT_ON               :4;       // 0x03, s:0, e:4
		uint8_t SHORT_CT_OFF              :4;       // 0x03, s:4, e:8
		uint8_t SHORT_COND_VALUE_LO       :8;       // 0x04, s:0, e:8
		uint8_t SHORT_COND_VALUE_HI       :8;       // 0x05, s:0, e:8
		uint8_t SHORT_ONDELAY_TIME        :8;       // 0x06, s:0, e:8
		uint8_t SHORT_ON_TIME             :8;       // 0x07, s:0, e:8
		uint8_t SHORT_OFFDELAY_TIME       :8;       // 0x08, s:0, e:8
		uint8_t SHORT_OFF_TIME            :8;       // 0x09, s:0, e:8
		uint8_t SHORT_ACTION_TYPE         :2;       // 0x0a, s:0, e:2
		uint8_t                           :4;       // 0x0a, s:2, e:6
		uint8_t SHORT_OFF_TIME_MODE       :1;       // 0x0a, s:6, e:7
		uint8_t SHORT_ON_TIME_MODE        :1;       // 0x0a, s:7, e:8
		uint8_t SHORT_JT_ON               :4;       // 0x0b, s:0, e:4
		uint8_t SHORT_JT_OFF              :4;       // 0x0b, s:4, e:8
		uint8_t SHORT_JT_ONDELAY          :4;       // 0x0c, s:0, e:4
		uint8_t SHORT_JT_OFFDELAY         :4;       // 0x0c, s:4, e:8
		uint8_t SHORT_JT_RAMPON           :4;       // 0x0d, s:0, e:4
		uint8_t SHORT_JT_RAMPOFF          :4;       // 0x0d, s:4, e:8
		uint8_t SHORT_OFF_LEVEL           :8;       // 0x0f, s:0, e:8
		uint8_t SHORT_ON_LEVEL            :8;       // 0x11, s:0, e:8
		uint8_t SHORT_CT_REFON            :4;       // 0x1c, s:0, e:4
		uint8_t SHORT_CT_REFOFF           :4;       // 0x1c, s:4, e:8
		uint8_t SHORT_MAX_TIME_FIRST_DIR  :8;       // 0x1d, s:0, e:8
		uint8_t SHORT_JT_REFON            :4;       // 0x1e, s:0, e:4
		uint8_t SHORT_JT_REFOFF           :4;       // 0x1e, s:4, e:8
		uint8_t SHORT_DRIVING_MODE        :8;       // 0x1f, s:0, e:8
		uint8_t LONG_CT_RAMPON            :4;       // 0x81, s:0, e:4
		uint8_t LONG_CT_RAMPOFF           :4;       // 0x81, s:4, e:8
		uint8_t LONG_CT_ONDELAY           :4;       // 0x82, s:0, e:4
		uint8_t LONG_CT_OFFDELAY          :4;       // 0x82, s:4, e:8
		uint8_t LONG_CT_ON                :4;       // 0x83, s:0, e:4
		uint8_t LONG_CT_OFF               :4;       // 0x83, s:4, e:8
		uint8_t LONG_COND_VALUE_LO        :8;       // 0x84, s:0, e:8
		uint8_t LONG_COND_VALUE_HI        :8;       // 0x85, s:0, e:8
		uint8_t LONG_ONDELAY_TIME         :8;       // 0x86, s:0, e:8
		uint8_t LONG_ON_TIME              :8;       // 0x87, s:0, e:8
		uint8_t LONG_OFFDELAY_TIME        :8;       // 0x88, s:0, e:8
		uint8_t LONG_OFF_TIME             :8;       // 0x89, s:0, e:8
		uint8_t LONG_ACTION_TYPE          :2;       // 0x8a, s:0, e:2
		uint8_t                           :3;       // 0x8a, s:2, e:5
		uint8_t LONG_MULTIEXECUTE         :1;       // 0x8a, s:5, e:6
		uint8_t LONG_OFF_TIME_MODE        :1;       // 0x8a, s:6, e:7
		uint8_t LONG_ON_TIME_MODE         :1;       // 0x8a, s:7, e:8
		uint8_t LONG_JT_ON                :4;       // 0x8b, s:0, e:4
		uint8_t LONG_JT_OFF               :4;       // 0x8b, s:4, e:8
		uint8_t LONG_JT_ONDELAY           :4;       // 0x8c, s:0, e:4
		uint8_t LONG_JT_OFFDELAY          :4;       // 0x8c, s:4, e:8
		uint8_t LONG_JT_RAMPON            :4;       // 0x8d, s:0, e:4
		uint8_t LONG_JT_RAMPOFF           :4;       // 0x8d, s:4, e:8
		uint8_t LONG_OFF_LEVEL            :8;       // 0x8f, s:0, e:8
		uint8_t LONG_ON_LEVEL             :8;       // 0x91, s:0, e:8
		uint8_t LONG_CT_REFON             :4;       // 0x9c, s:0, e:4
		uint8_t LONG_CT_REFOFF            :4;       // 0x9c, s:4, e:8
		uint8_t LONG_MAX_TIME_FIRST_DIR   :8;       // 0x9d, s:0, e:8
		uint8_t LONG_JT_REFON             :4;       // 0x9e, s:0, e:4
		uint8_t LONG_JT_REFOFF            :4;       // 0x9e, s:4, e:8
		uint8_t LONG_DRIVING_MODE         :8;       // 0x9f, s:0, e:8
	} lstPeer;

	 /*
	  * Contains all register from list 3 without short or long prefix
	  */
	struct s_l3 {
		uint8_t CT_RAMPON           :4;       // 0x01, s:0, e:4
		uint8_t CT_RAMPOFF          :4;       // 0x01, s:4, e:8
		uint8_t CT_ONDELAY          :4;       // 0x02, s:0, e:4
		uint8_t CT_OFFDELAY         :4;       // 0x02, s:4, e:8
		uint8_t CT_ON               :4;       // 0x03, s:0, e:4
		uint8_t CT_OFF              :4;       // 0x03, s:4, e:8
		uint8_t COND_VALUE_LO       :8;       // 0x04, s:0, e:8
		uint8_t COND_VALUE_HI       :8;       // 0x05, s:0, e:8
		uint8_t ONDELAY_TIME        :8;       // 0x06, s:0, e:8
		uint8_t ON_TIME             :8;       // 0x07, s:0, e:8
		uint8_t OFFDELAY_TIME       :8;       // 0x08, s:0, e:8
		uint8_t OFF_TIME            :8;       // 0x09, s:0, e:8
		uint8_t ACTION_TYPE         :2;       // 0x0a, s:0, e:2
		uint8_t                     :4;       // 0x0a, s:2, e:6
		uint8_t OFF_TIME_MODE       :1;       // 0x0a, s:6, e:7
		uint8_t ON_TIME_MODE        :1;       // 0x0a, s:7, e:8
		uint8_t JT_ON               :4;       // 0x0b, s:0, e:4
		uint8_t JT_OFF              :4;       // 0x0b, s:4, e:8
		uint8_t JT_ONDELAY          :4;       // 0x0c, s:0, e:4
		uint8_t JT_OFFDELAY         :4;       // 0x0c, s:4, e:8
		uint8_t JT_RAMPON           :4;       // 0x0d, s:0, e:4
		uint8_t JT_RAMPOFF          :4;       // 0x0d, s:4, e:8
		uint8_t OFF_LEVEL           :8;       // 0x0f, s:0, e:8
		uint8_t ON_LEVEL            :8;       // 0x11, s:0, e:8
		uint8_t CT_REFON            :4;       // 0x1c, s:0, e:4
		uint8_t CT_REFOFF           :4;       // 0x1c, s:4, e:8
		uint8_t MAX_TIME_FIRST_DIR  :8;       // 0x1d, s:0, e:8
		uint8_t JT_REFON            :4;       // 0x1e, s:0, e:4
		uint8_t JT_REFOFF           :4;       // 0x1e, s:4, e:8
		uint8_t DRIVING_MODE        :8;       // 0x1f, s:0, e:8
	} *l3;

	void (*fInit)(uint8_t);																	// pointer to init function in main sketch
	//void (*fUpdateState)(uint8_t, uint16_t, uint16_t, uint8_t, uint8_t);													// pointer to updateState function in main sketch, first value is state level
	uint8_t   stateToSend;																	// is there a status to be send, 1 indicates an ACK, 2 a status message
	uint16_t  msgDelay;
	waitTimer msgTmr;																		// message timer for sending status

	waitTimer delayTmr;																		// delay timer for on,off and delay time
	uint16_t  modRampTime;

	uint16_t  modReferenceTimeTopBottom;
	uint16_t  modReferenceTimeBottomTop;

	uint8_t   setState;																		// status to set
	uint32_t  adjDlyPWM;																	// timer to follow in updateState function
	uint16_t  characteristicStat;															// depends on list1 characteristic setting
	waitTimer adjTmr;																		// timer for adjustment of PWM

	//uint8_t   oldStat;																	// remember modStat in delay off blink function
	uint8_t   directionDim :1;																// used in toogleDim function

	uint8_t   fMsgCount;																	// message counter for type 40 message
	uint8_t   curState;																		// current state of the module
	uint8_t   nxtState;																		// next state of the modul

	uint32_t power = 0l;
	uint32_t energyCount = 0; // max 23bits, addToEnergyCount takes care of overflow
	uint32_t energySignals = 0; // 2 byte wide counter constant * 1000 for W/100 precision
	unsigned long lastReportedTs = 0;
	unsigned long lastSignalTs =0;
	uint32_t lastReportedEnergyCount = 0l;
	int len = 16;
	InfraRedSignalDetector infraDetector;

  public://----------------------------------------------------------------------------------------------------------------
  //- user defined functions ----------------------------------------------------------------------------------------------

	void     config(void init(uint8_t));				// configures the module, jump addresses, etc

  private://---------------------------------------------------------------------------------------------------------------
	/*inline void     toggleDim(void);																// dim up or down with one key
	void     upDim(void);																	// up dim procedure
	void     downDim(void);																	// down dim procedure

	inline void     updateState(void);														// adjusts the actor state value*/
	//inline void     sendState(void);														// send status function
	void     poll(void);																	// blind polling function
	void pollSensor(void);	// poll the power reading
	void addToEnergyCount(int tenthKw);
	void sendPowerEvent(uint32_t eCountToSend, uint32_t powerToSend);
	//- helpers defined functions -------------------------------------------------------------------------------------------
	inline void     showStruct(void);
	uint8_t convertMeterSensibility(uint8_t sensibility);

  public://----------------------------------------------------------------------------------------------------------------
  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
	uint8_t  modState;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t  regCnl;																		// holds the channel for the module

	//AS       *hm;																			// pointer to HM class instance

	inline void     setToggle(void);																// toggle the module initiated by config button
	void     configCngEvent(void);															// list1 on registered channel had changed
	void     pairSetEvent(uint8_t *data, uint8_t len);										// pair message to specific channel, handover information for value, ramp time and so on
	inline void     pairStatusReq(void);															// event on status request
	//void     peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);						// peer message was received on the registered channel, handover the message bytes and length

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void     regInHM(uint8_t cnl, uint8_t lst);								// register this module in HM on the specific channel
	void     hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
	void     peerAddEvent(uint8_t *data, uint8_t len);										// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
	inline void     firstStart(void);																// first start detection, to write list1

	uint8_t stateMachine_setNextState(uint8_t curState);
};


#endif
