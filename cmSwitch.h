/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin registrar functions -----------------------------------------------------------------------------------------------
* - with a lot of support from martin876 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmSwitch_H
#define _cmSwitch_H
#define CLASS_NAME cmSwitch

#include "AS.h"
#include "HAL.h"

// default settings are defined in cmSwitch.cpp - updatePeerDefaults


class cmSwitch {
//- user code here --------------------------------------------------------------------------------------------------------
public://------------------------------------------------------------------------------------------------------------------
	cmSwitch() {}																				// class constructor

	struct s_lstCnl {
		uint8_t AES_ACTIVE           : 1;  // 0x08.0, s:1   d: false  
		uint8_t                      : 7;  // 0x08.1, s:7   d:   
	  } lstCnl; 
	  
	struct s_lstPeer {
		uint8_t SHORT_CT_ONDELAY     : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO  ----------------------------------------
		uint8_t SHORT_CT_OFFDELAY    : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO   
		uint8_t SHORT_CT_ON          : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO  
		uint8_t SHORT_CT_OFF         : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t SHORT_COND_VALUE_LO  : 8;  // 0x04.0, s:8   d: 50  
		uint8_t SHORT_COND_VALUE_HI  : 8;  // 0x05.0, s:8   d: 100  
		uint8_t SHORT_ONDELAY_TIME   : 8;  // 0x06.0, s:8   d: 0 s                      11 Byte for a short struct to
		uint8_t SHORT_ON_TIME        : 8;  // 0x07.0, s:8   d: 111600.0 s               differentiate between short
		uint8_t SHORT_OFFDELAY_TIME  : 8;  // 0x08.0, s:8   d: 0 s                      and long values
		uint8_t SHORT_OFF_TIME       : 8;  // 0x09.0, s:8   d: 111600.0 s 
		uint8_t SHORT_ACTION_TYPE    : 2;  // 0x0a.0, s:2   d: JUMP_TO_TARGET  
		uint8_t                      : 4;  // 0x0a.2, s:4   d:   
		uint8_t SHORT_OFF_TIME_MODE  : 1;  // 0x0a.6, s:1   d: ABSOLUTE  
		uint8_t SHORT_ON_TIME_MODE   : 1;  // 0x0a.7, s:1   d: ABSOLUTE  
		uint8_t SHORT_JT_ON          : 4;  // 0x0b.0, s:4   d: OFF  
		uint8_t SHORT_JT_OFF         : 4;  // 0x0b.4, s:4   d: OFF  
		uint8_t SHORT_JT_ONDELAY     : 4;  // 0x0c.0, s:4   d: OFF  
		uint8_t SHORT_JT_OFFDELAY    : 4;  // 0x0c.4, s:4   d: OFF                 ----------------------------------------------
		uint8_t LONG_CT_ONDELAY      : 4;  // 0x82.0, s:4   d: X GE COND_VALUE_LO  ---------------------------------------------- 
		uint8_t LONG_CT_OFFDELAY     : 4;  // 0x82.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_CT_ON           : 4;  // 0x83.0, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_CT_OFF          : 4;  // 0x83.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_COND_VALUE_LO   : 8;  // 0x84.0, s:8   d: 50  
		uint8_t LONG_COND_VALUE_HI   : 8;  // 0x85.0, s:8   d: 100  
		uint8_t LONG_ONDELAY_TIME    : 8;  // 0x86.0, s:8   d: 0 s                      another 11 byte for the long
		uint8_t LONG_ON_TIME         : 8;  // 0x87.0, s:8   d: 111600.0 s				message portion 
		uint8_t LONG_OFFDELAY_TIME   : 8;  // 0x88.0, s:8   d: 0 s 
		uint8_t LONG_OFF_TIME        : 8;  // 0x89.0, s:8   d: 111600.0 s 
		uint8_t LONG_ACTION_TYPE     : 2;  // 0x8a.0, s:2   d: JUMP_TO_TARGET  
		uint8_t                      : 3;  // 0x8a.2, s:3   d:   
		uint8_t LONG_MULTIEXECUTE    : 1;  // 0x8a.5, s:1   d: ON  
		uint8_t LONG_OFF_TIME_MODE   : 1;  // 0x8a.6, s:1   d: ABSOLUTE  
		uint8_t LONG_ON_TIME_MODE    : 1;  // 0x8a.7, s:1   d: ABSOLUTE  
		uint8_t LONG_JT_ON           : 4;  // 0x8b.0, s:4   d: OFF  
		uint8_t LONG_JT_OFF          : 4;  // 0x8b.4, s:4   d: OFF  
		uint8_t LONG_JT_ONDELAY      : 4;  // 0x8c.0, s:4   d: OFF  
		uint8_t LONG_JT_OFFDELAY     : 4;  // 0x8c.4, s:4   d: OFF                 ----------------------------------------------
	} lstPeer;

	struct s_l3 {
		uint8_t CT_ONDELAY           : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO   -- ctDlyOn             :4;     // 0x02, 0x82, s:0, e:4
		uint8_t CT_OFFDELAY          : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO   -- ctDlyOff            :4;     // 0x02, 0x82, s:4, e:8 
		uint8_t CT_ON                : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO   -- ctOn                :4;     // 0x03, 0x83, s:0, e:4
		uint8_t CT_OFF               : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO   -- ctOff               :4;     // 0x03, 0x83, s:4, e:8
		uint8_t COND_VALUE_LO        : 8;  // 0x04.0, s:8   d: 50                   -- ctValLo;                    // 0x04, 0x84, s:0, e:0
		uint8_t COND_VALUE_HI        : 8;  // 0x05.0, s:8   d: 100                  -- ctValHi;                    // 0x05, 0x85, s:0, e:0
		uint8_t ONDELAY_TIME         : 8;  // 0x06.0, s:8   d: 0 s                  -- onDly;                      // 0x06, 0x86, s:0, e:0
		uint8_t ON_TIME              : 8;  // 0x07.0, s:8   d: 111600.0 s           -- onTime;                     // 0x07, 0x87, s:0, e:0
		uint8_t OFFDELAY_TIME        : 8;  // 0x08.0, s:8   d: 0 s                  -- offDly;                     // 0x08, 0x88, s:0, e:0
		uint8_t OFF_TIME             : 8;  // 0x09.0, s:8   d: 111600.0 s           -- offTime;                    // 0x09, 0x89, s:0, e:0
		uint8_t ACTION_TYPE          : 2;  // 0x0a.0, s:2   d: JUMP_TO_TARGET       -- actionType          :2;     // 0x0a, 0x8a, s:0, e:2
		uint8_t                      : 3;  // 0x0a.2, s:3   d:   
		uint8_t                      : 1;  // 0x0a.5, s:1   d:                      -- multiExec           :1;     //       0x8a, s:5, e:6
		uint8_t OFF_TIME_MODE        : 1;  // 0x0a.6, s:1   d: ABSOLUTE             -- offTimeMode         :1;     // 0x0a, 0x8a, s:6, e:7
		uint8_t ON_TIME_MODE         : 1;  // 0x0a.7, s:1   d: ABSOLUTE	            -- onTimeMode          :1;     // 0x0a, 0x8a, s:7, e:8
		uint8_t JT_ON                : 4;  // 0x0b.0, s:4   d: OFF                  -- jtOn                :4;     // 0x0b, 0x8b, s:0, e:4
		uint8_t JT_OFF               : 4;  // 0x0b.4, s:4   d: OFF                  -- jtOff               :4;     // 0x0b, 0x8b, s:4, e:8
		uint8_t JT_ONDELAY           : 4;  // 0x0c.0, s:4   d: OFF                  -- jtDlyOn             :4;     // 0x0c, 0x8c, s:0, e:4
		uint8_t JT_OFFDELAY          : 4;  // 0x0c.4, s:4   d: OFF                  -- jtDlyOff            :4;     // 0x0c, 0x8c, s:4, e:8
	} *l3;
	

public://------------------------------------------------------------------------------------------------------------------
//- user defined functions ------------------------------------------------------------------------------------------------

	void (*fInit)(uint8_t);																	// pointer to init function in main sketch
	void (*fSwitch)(uint8_t,uint8_t);														// pointer to switch function in main sketch

	waitTimer delayTmr;																		// delay timer for relay

	uint8_t   cnt;																			// message counter for type 40 message
	uint8_t   curStat:4, nxtStat:4;															// current state and next state

	waitTimer msgTmr;																		// message timer for sending status
	uint16_t  msgDelay;																		// delay for sending initial status
	uint8_t	  sendStat  :2;																	// indicator for sendStatus function

	uint8_t   tr11      :1;																	// trigger 11 active
	uint8_t   tr11Value;																	// trigger 11 set value
	uint16_t  rampTme, duraTme;																// time store for trigger 11

	uint8_t   setStat;																		// status to set on the Relay channel

	void      config(void Init(uint8_t), void xSwitch(uint8_t,uint8_t));					// handover for jump addresses

	void      trigger11(uint8_t value, uint8_t *rampTime, uint8_t *duraTime);				// what happens while a trigger11 message arrive
	void      trigger40(uint8_t msgLng, uint8_t msgCnt);									// same for peer messages
	void      trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal);					// same for sensor messages

	void      adjRly(void);																	// setting of relay status
	void      sendStatus(void);																// help function to send status messages

	void      rlyPoll(void);																// polling function


//- mandatory functions for every new module to communicate within AS protocol stack --------------------------------------
	uint8_t     modStat;																	// module status byte, needed for list3 modules to answer status requests
	uint8_t     modDUL;																		// module down up low battery byte
	uint8_t     regCnl;																		// holds the channel for the module

	inline void setToggle(void);															// toggle the module initiated by config button
	inline void configCngEvent(void);														// list1 on registered channel had changed
	inline void pairSetEvent(uint8_t *data, uint8_t len);									// pair message to specific channel, handover information for value, ramp time and so on
	inline void pairStatusReq(void);														// event on status request
	inline void peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);						// peer message was received on the registered channel, handover the message bytes and length
	inline void peerAddEvent(uint8_t *data, uint8_t len);									// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index

	inline void poll(void);																	// poll function, driven by HM loop
	inline void updatePeerDefaults(uint8_t by11, uint8_t *data, uint8_t len);				// add peer channel defaults to list3/4

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void        regInHM(uint8_t cnl, uint8_t lst);											// register this module in HM on the specific channel
	void        hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
};

#endif
