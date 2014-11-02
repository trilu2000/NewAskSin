//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin dimmer class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _DIMMER_H
#define _DIMMER_H

#include "AS.h"
#include "HAL.h"

// default settings for list3 or list4
const uint8_t peerOdd[] =    {		// cnl 2, 4, 6
	// Default actor dual 1: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:64 0C:66 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:64 8C:66
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x64, 0x66,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x64, 0x66
};
const uint8_t peerEven[] =   {		// cnl 1, 3, 5
	// Default actor dual 2: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:13 0C:33 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:13 8C:33
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x13, 0x33,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x13, 0x33
};
const uint8_t peerSingle[] = {
	// Default actor single: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:14 0C:63 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:14 8C:63
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x14, 0x63,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x14, 0x63,
};


class Dimmer {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------
	waitTimer delayTmr;																			// delay timer for on,off and delay time
	waitTimer msgTmr;																			// message timer for sending status

	struct s_lstCnl {
		// 0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
		uint8_t  transmitTryMax;             // 0x30, s:0, e:0
		uint8_t  ovrTempLvl;                 // 0x32, s:0, e:0
		uint8_t  redTempLvl;                 // 0x34, s:0, e:0
		uint8_t  redLvl;                     // 0x35, s:0, e:0
		uint8_t  powerUpAction       :1;     // 0x56, s:0, e:1
		uint8_t                      :7;     //       l:7, s:0
		uint8_t  statusInfoMinDly    :5;     // 0x57, s:0, e:5
		uint8_t  statusInfoRandom    :3;     // 0x57, s:5, e:8
		uint8_t  characteristic      :1;     // 0x58, s:0, e:1
		uint8_t                      :7;     //       l:7, s:0
		uint8_t  logicCombination    :5;     // 0x59, s:0, e:5
		uint8_t                      :3;     //
	} lstCnl;

	struct s_lstPeer {
		// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
		uint8_t  shCtRampOn          :4;     // 0x01, s:0, e:4
		uint8_t  shCtRampOff         :4;     // 0x01, s:4, e:8
		uint8_t  shCtDlyOn           :4;     // 0x02, s:0, e:4
		uint8_t  shCtDlyOff          :4;     // 0x02, s:4, e:8
		uint8_t  shCtOn              :4;     // 0x03, s:0, e:4
		uint8_t  shCtOff             :4;     // 0x03, s:4, e:8
		uint8_t  shCtValLo;                  // 0x04, s:0, e:0
		uint8_t  shCtValHi;                  // 0x05, s:0, e:0
		uint8_t  shOnDly;                    // 0x06, s:0, e:0
		uint8_t  shOnTime;                   // 0x07, s:0, e:0
		uint8_t  shOffDly;                   // 0x08, s:0, e:0
		uint8_t  shOffTime;                  // 0x09, s:0, e:0
		uint8_t  shActionTypeDim     :4;     // 0x0a, s:0, e:4
		uint8_t                      :2;     //
		uint8_t  shOffTimeMode       :1;     // 0x0a, s:6, e:7
		uint8_t  shOnTimeMode        :1;     // 0x0a, s:7, e:8
		uint8_t  shDimJtOn           :4;     // 0x0b, s:0, e:4
		uint8_t  shDimJtOff          :4;     // 0x0b, s:4, e:8
		uint8_t  shDimJtDlyOn        :4;     // 0x0c, s:0, e:4
		uint8_t  shDimJtDlyOff       :4;     // 0x0c, s:4, e:8
		uint8_t  shDimJtRampOn       :4;     // 0x0d, s:0, e:4
		uint8_t  shDimJtRampOff      :4;     // 0x0d, s:4, e:8
		uint8_t                      :5;     //       l:0, s:5
		uint8_t  shOffDlyBlink       :1;     // 0x0e, s:5, e:6
		uint8_t  shOnLvlPrio         :1;     // 0x0e, s:6, e:7
		uint8_t  shOnDlyMode         :1;     // 0x0e, s:7, e:8
		uint8_t  shOffLevel;                 // 0x0f, s:0, e:0
		uint8_t  shOnMinLevel;               // 0x10, s:0, e:0
		uint8_t  shOnLevel;                  // 0x11, s:0, e:0
		uint8_t  shRampSstep;                // 0x12, s:0, e:0
		uint8_t  shRampOnTime;               // 0x13, s:0, e:0
		uint8_t  shRampOffTime;              // 0x14, s:0, e:0
		uint8_t  shDimMinLvl;                // 0x15, s:0, e:0
		uint8_t  shDimMaxLvl;                // 0x16, s:0, e:0
		uint8_t  shDimStep;                  // 0x17, s:0, e:0
		uint8_t  shOffDlyNewTime;            // 0x19, s:0, e:0
		uint8_t  shOffDlyOldTime;            // 0x1a, s:0, e:0
		uint8_t  shDimElsActionType  :4;     // 0x26, s:0, e:4
		uint8_t                      :2;     //
		uint8_t  shDimElsOffTimeMd   :1;     // 0x26, s:6, e:7
		uint8_t  shDimElsOnTimeMd    :1;     // 0x26, s:7, e:8
		uint8_t  shDimElsJtOn        :4;     // 0x27, s:0, e:4
		uint8_t  shDimElsJtOff       :4;     // 0x27, s:4, e:8
		uint8_t  shDimElsJtDlyOn     :4;     // 0x28, s:0, e:4
		uint8_t  shDimElsJtDlyOff    :4;     // 0x28, s:4, e:8
		uint8_t  shDimElsJtRampOn    :4;     // 0x29, s:0, e:4
		uint8_t  shDimElsJtRampOff   :4;     // 0x29, s:4, e:8
		uint8_t  lgCtRampOn          :4;     // 0x81, s:0, e:4
		uint8_t  lgCtRampOff         :4;     // 0x81, s:4, e:8
		uint8_t  lgCtDlyOn           :4;     // 0x82, s:0, e:4
		uint8_t  lgCtDlyOff          :4;     // 0x82, s:4, e:8
		uint8_t  lgCtOn              :4;     // 0x83, s:0, e:4
		uint8_t  lgCtOff             :4;     // 0x83, s:4, e:8
		uint8_t  lgCtValLo;                  // 0x84, s:0, e:0
		uint8_t  lgCtValHi;                  // 0x85, s:0, e:0
		uint8_t  lgOnDly;                    // 0x86, s:0, e:0
		uint8_t  lgOnTime;                   // 0x87, s:0, e:0
		uint8_t  lgOffDly;                   // 0x88, s:0, e:0
		uint8_t  lgOffTime;                  // 0x89, s:0, e:0
		uint8_t  lgActionTypeDim     :4;     // 0x8a, s:0, e:4
		uint8_t                      :1;     //
		uint8_t  lgMultiExec         :1;     // 0x8a, s:5, e:6
		uint8_t  lgOffTimeMode       :1;     // 0x8a, s:6, e:7
		uint8_t  lgOnTimeMode        :1;     // 0x8a, s:7, e:8
		uint8_t  lgDimJtOn           :4;     // 0x8b, s:0, e:4
		uint8_t  lgDimJtOff          :4;     // 0x8b, s:4, e:8
		uint8_t  lgDimJtDlyOn        :4;     // 0x8c, s:0, e:4
		uint8_t  lgDimJtDlyOff       :4;     // 0x8c, s:4, e:8
		uint8_t  lgDimJtRampOn       :4;     // 0x8d, s:0, e:4
		uint8_t  lgDimJtRampOff      :4;     // 0x8d, s:4, e:8
		uint8_t                      :5;     //       l:0, s:5
		uint8_t  lgOffDlyBlink       :1;     // 0x8e, s:5, e:6
		uint8_t  lgOnLvlPrio         :1;     // 0x8e, s:6, e:7
		uint8_t  lgOnDlyMode         :1;     // 0x8e, s:7, e:8
		uint8_t  lgOffLevel;                 // 0x8f, s:0, e:0
		uint8_t  lgOnMinLevel;               // 0x90, s:0, e:0
		uint8_t  lgOnLevel;                  // 0x91, s:0, e:0
		uint8_t  lgRampSstep;                // 0x92, s:0, e:0
		uint8_t  lgRampOnTime;               // 0x93, s:0, e:0
		uint8_t  lgRampOffTime;              // 0x94, s:0, e:0
		uint8_t  lgDimMinLvl;                // 0x95, s:0, e:0
		uint8_t  lgDimMaxLvl;                // 0x96, s:0, e:0
		uint8_t  lgDimStep;                  // 0x97, s:0, e:0
		uint8_t  lgOffDlyNewTime;            // 0x99, s:0, e:0
		uint8_t  lgOffDlyOldTime;            // 0x9a, s:0, e:0
		uint8_t  lgDimElsActionType  :4;     // 0xa6, s:0, e:4
		uint8_t                      :2;     //
		uint8_t  lgDimElsOffTimeMd   :1;     // 0xa6, s:6, e:7
		uint8_t  lgDimElsOnTimeMd    :1;     // 0xa6, s:7, e:8
		uint8_t  lgDimElsJtOn        :4;     // 0xa7, s:0, e:4
		uint8_t  lgDimElsJtOff       :4;     // 0xa7, s:4, e:8
		uint8_t  lgDimElsJtDlyOn     :4;     // 0xa8, s:0, e:4
		uint8_t  lgDimElsJtDlyOff    :4;     // 0xa8, s:4, e:8
		uint8_t  lgDimElsJtRampOn    :4;     // 0xa9, s:0, e:4
		uint8_t  lgDimElsJtRampOff   :4;     // 0xa9, s:4, e:8
	} lstPeer;

	struct s_l3 {
		// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
		uint8_t  ctRampOn       :4;     // 0x01,0x81, s:0, e:4
		uint8_t  ctRampOff      :4;     // 0x01,0x81, s:4, e:8
		uint8_t  ctDlyOn        :4;     // 0x02,0x82, s:0, e:4
		uint8_t  ctDlyOff       :4;     // 0x02,0x82, s:4, e:8
		uint8_t  ctOn           :4;     // 0x03,0x83, s:0, e:4
		uint8_t  ctOff          :4;     // 0x03,0x83, s:4, e:8
		uint8_t  ctValLo;               // 0x04,0x84, s:0, e:0
		uint8_t  ctValHi;               // 0x05,0x85, s:0, e:0

		uint8_t  onDly;                 // 0x06,0x86, s:0, e:0
		uint8_t  onTime;                // 0x07,0x87, s:0, e:0
		uint8_t  offDly;                // 0x08,0x88, s:0, e:0
		uint8_t  offTime;               // 0x09,0x89, s:0, e:0

		uint8_t  actionType     :4;     // 0x0a,0x8a, s:0, e:4
		uint8_t                 :1;     //
		uint8_t  lgMultiExec    :1;     //      0x8a, s:5, e:6

		uint8_t  offTimeMode    :1;     // 0x0a,0x8a, s:6, e:7
		uint8_t  onTimeMode     :1;     // 0x0a,0x8a, s:7, e:8

		uint8_t  jtOn           :4;     // 0x0b,0x8b, s:0, e:4
		uint8_t  jtOff          :4;     // 0x0b,0x8b, s:4, e:8
		uint8_t  jtDlyOn        :4;     // 0x0c,0x8c, s:0, e:4
		uint8_t  jtDlyOff       :4;     // 0x0c,0x8c, s:4, e:8
		uint8_t  jtRampOn       :4;     // 0x0d,0x8d, s:0, e:4
		uint8_t  jtRampOff      :4;     // 0x0d,0x8d, s:4, e:8

		uint8_t                 :5;     //            l:0, s:5

		uint8_t  offDlyBlink    :1;     // 0x0e,0x8e, s:5, e:6
		uint8_t  onLvlPrio      :1;     // 0x0e,0x8e, s:6, e:7
		uint8_t  onDlyMode      :1;     // 0x0e,0x8e, s:7, e:8
		uint8_t  offLevel;              // 0x0f,0x8f, s:0, e:0
		uint8_t  onMinLevel;            // 0x10,0x90, s:0, e:0
		uint8_t  onLevel;               // 0x11,0x91, s:0, e:0

		uint8_t  rampSstep;             // 0x12,0x92, s:0, e:0
		uint8_t  rampOnTime;            // 0x13,0x93, s:0, e:0
		uint8_t  rampOffTime;           // 0x14,0x94, s:0, e:0

		uint8_t  dimMinLvl;             // 0x15,0x95, s:0, e:0
		uint8_t  dimMaxLvl;             // 0x16,0x96, s:0, e:0
		uint8_t  dimStep;               // 0x17,0x97, s:0, e:0
		uint8_t  offDlyNewTime;         // 0x19,0x99, s:0, e:0
		uint8_t  offDlyOldTime;         // 0x1a,0x9a, s:0, e:0

		uint8_t  elsActionType  :4;     // 0x26,0xa6, s:0, e:4
		uint8_t                 :2;     //
		uint8_t  elsOffTimeMd   :1;     // 0x26,0xa6, s:6, e:7
		uint8_t  elsOnTimeMd    :1;     // 0x26,0xa6, s:7, e:8
		uint8_t  elsJtOn        :4;     // 0x27,0xa7, s:0, e:4
		uint8_t  elsJtOff       :4;     // 0x27,0xa7, s:4, e:8
		uint8_t  elsJtDlyOn     :4;     // 0x28,0xa8, s:0, e:4
		uint8_t  elsJtDlyOff    :4;     // 0x28,0xa8, s:4, e:8
		uint8_t  elsJtRampOn    :4;     // 0x29,0xa9, s:0, e:4
		uint8_t  elsJtRampOff   :4;     // 0x29,0xa9, s:4, e:8
	} *l3;
	
	uint8_t sendStat       :1;  

  public://----------------------------------------------------------------------------------------------------------------
  //- user defined functions ----------------------------------------------------------------------------------------------
	
	void (*fInit)(void);
	void (*fSwitch)(uint8_t);

	uint8_t   setStat;																			// status to set on the PWM channel
	uint32_t  adjDlyPWM;																		// timer to follow in adjPWM function
	waitTimer adjTmr;																			// timer for adjustment of PWM
	
	uint8_t  minDly;																			// remember delay for send status information
	uint8_t  cnt;																				// message counter for type 40 message
	uint8_t  curStat:4, nxtStat:4;																// current state and next state
	uint16_t rampTme, duraTme;																	// time store for trigger 11

	void     config(void Init(), void Switch(uint8_t), uint8_t minDelay);
	void     trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime);
	void     trigger40(uint8_t msgLng, uint8_t msgCnt);
	void     trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal);
	void     adjPWM(void);

	void     upDim(void);
	void     downDim(void);
	void     showStruct(void);
	
  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
	uint8_t  modStat;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t  modDUL;																		// module down up low battery byte
	uint8_t  regCnl;																		// holds the channel for the module

	AS       *hm;																			// pointer to HM class instance

	void     setToggle(void);																// toggle the module initiated by config button
	void     configCngEvent(void);															// list1 on registered channel had changed
	void     pairSetEvent(uint8_t *data, uint8_t len);										// pair message to specific channel, handover information for value, ramp time and so on
	void     pairStatusReq(void);															// event on status request
	void     peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);						// peer message was received on the registered channel, handover the message bytes and length

	void     poll(void);																	// poll function, driven by HM loop

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void     regInHM(uint8_t cnl, uint8_t lst, AS *instPtr);								// register this module in HM on the specific channel
	void     hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
	void     peerAddEvent(uint8_t *data, uint8_t len);										// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
};


#endif
