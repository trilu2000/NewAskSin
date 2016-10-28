/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Switch ------------------------------------------------------------------------------------------
* - with a lot of support from martin876 at FHEM forum
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmSwitch_H
#define _cmSwitch_H

#include "cmMaster.h"

// default settings are defined in cmSwitch.cpp - updatePeerDefaults


const uint8_t cmSwitch_ChnlReg[] PROGMEM = { 0x08, };
const uint8_t cmSwitch_ChnlDef[] PROGMEM = { 0x00, };

const uint8_t cmSwitch_PeerReg[] PROGMEM = { 0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c, };
const uint8_t cmSwitch_PeerDef[] PROGMEM = { 0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x01,0x44,0x44,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x21,0x44,0x44, };

#define NOT_USED 255
namespace ACTION {
	enum E : uint8_t { INACTIVE, JUMP_TO_TARGET, TOGGLE_TO_COUNTER, TOGGLE_INV_TO_COUNTER };
};
namespace JT {
	enum E : uint8_t { NO_JUMP_IGNORE_COMMAND = 0x00, ONDELAY = 0x01, ON = 0x03, OFFDELAY = 0x04, OFF = 0x06 };
};
namespace CT {
	enum E : uint8_t { X_GE_COND_VALUE_LO, X_GE_COND_VALUE_HI, X_LT_COND_VALUE_LO, X_LT_COND_VALUE_HI, COND_VALUE_LO_LE_X_LT_COND_VALUE_HI, X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI };
};

class cmSwitch : public cmMaster {
private:  //---------------------------------------------------------------------------------------------------------------

	struct s_l1 {
		uint8_t AES_ACTIVE           : 1;  // 0x08.0, s:1   d: false  
		uint8_t                      : 7;  // 0x08.1, s:7   d:   
	} *l1;  

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
		uint8_t SHORT_JT_OFFDELAY    : 4;  // 0x0c.4, s:4   d: OFF                 ----------------------------------------
		uint8_t LONG_CT_ONDELAY      : 4;  // 0x82.0, s:4   d: X GE COND_VALUE_LO  ---------------------------------------- 
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
		uint8_t LONG_JT_OFFDELAY     : 4;  // 0x8c.4, s:4   d: OFF                 ----------------------------------------
	} *l3F;

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

	static void initSwitch(uint8_t channel);												// functions in user sketch needed
	static void switchSwitch(uint8_t channel, uint8_t status);

public:  //----------------------------------------------------------------------------------------------------------------

	cmSwitch(const uint8_t peer_max);														// constructor

	struct s_tr11 {
		uint8_t  active;																	// trigger 11 active
		uint8_t  value;																		// trigger 11 set value
		uint16_t ramp_time;																	// time store for trigger 11
		uint16_t dura_time;																
	} tr11;

	struct s_tr40 {
		uint8_t   cnt;																		// counter store for type 40/41 messages to detect a repeated long
		uint8_t   cur;																		// current state and next state
		uint8_t	  nxt;
		waitTimer delay;																	// delay timer for relay
	} tr40;


	inline void adjustStatus(void);															// setting of relay status

	virtual void request_peer_defaults(uint8_t idx, s_m01xx01 *buf);						// add peer channel defaults to list3/4

	virtual void cm_poll(void);																// poll function, driven by HM loop
	virtual void set_toggle(void);															// toggle the module initiated by config button

	/* receive functions to handle requests forwarded by AS:processMessage
	*  only channel module related requests are forwarded, majority of requests are handled within main AS class */
	virtual void CONFIG_STATUS_REQUEST(s_m01xx0e *buf);										// master is asking for channel status
	virtual void INSTRUCTION_SET(s_m1102xx *buf);											// master wants to set channel status
	virtual void SWITCH(s_m3Exxxx *buf);													// switch message from master to test a peer setup
	virtual void REMOTE(s_m40xxxx *buf);													// remote peer message
	virtual void SENSOR_EVENT(s_m41xxxx *buf);												// sensor peer message

};

#endif
