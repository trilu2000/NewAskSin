/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Dimmer ------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmDimmer_H
#define _cmDimmer_H

#include "cmMaster.h"

// default settings are defined in cmSwitch.cpp - updatePeerDefaults


const uint8_t cmDimmer_ChnlReg[] PROGMEM = { 0x08,0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59, };
const uint8_t cmDimmer_ChnlDef[] PROGMEM = { 0x00,0x06,0x50,0x4b,0x50,0x00,0x24,0x01,0x01, };

const uint8_t cmDimmer_PeerReg[] PROGMEM = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa6,0xa7,0xa8,0xa9, };
const uint8_t cmDimmer_PeerDef[] PROGMEM = { 0x00,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x01,0x11,0x11,0x11,0x20,0x00,0x14,0xc8,0x0a,0x05,0x05,0x00,0xc8,0x0a,0x0a,0x04,0x04,0x00,0x11,0x11,0x11,0x00,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x00,0x11,0x11,0x11,0x20,0x00,0x14,0xc8,0x0a,0x05,0x05,0x00,0xc8,0x0a,0x0a,0x04,0x04,0x00,0x11,0x11,0x11, };

#define NOT_USED 255
namespace ACTION {
	enum E : uint8_t { INACTIVE, JUMP_TO_TARGET, TOGGLE_TO_COUNTER, TOGGLE_INV_TO_COUNTER, UPDIM, DOWNDIM, TOOGLEDIM, TOGGLEDIM_TO_COUNTER, TOGGLEDIM_INVERS_TO_COUNTER, };
};
namespace JT {
	enum E : uint8_t { NO_JUMP_IGNORE_COMMAND = 0x00, ONDELAY = 0x01, RAMPON = 0x02, ON = 0x03, OFFDELAY = 0x04, RAMPOFF = 0x05, OFF = 0x06 };
};
namespace CT {
	enum E : uint8_t { X_GE_COND_VALUE_LO, X_GE_COND_VALUE_HI, X_LT_COND_VALUE_LO, X_LT_COND_VALUE_HI, COND_VALUE_LO_LE_X_LT_COND_VALUE_HI, X_LT_COND_VALUE_LO_OR_X_GE_COND_VALUE_HI };
};

class cmDimmer : public cmMaster {
private:  //---------------------------------------------------------------------------------------------------------------

	struct s_l1 {
		uint8_t AES_ACTIVE               : 1;  // 0x08.0, s:1   d: false  
		uint8_t                          : 7;  // 0x08.1, s:7   d:   
		uint8_t TRANSMIT_TRY_MAX         : 8;  // 0x30.0, s:8   d: 6  
		uint8_t OVERTEMP_LEVEL           : 8;  // 0x32.0, s:8   d: 80 °C 
		uint8_t REDUCE_TEMP_LEVEL        : 8;  // 0x34.0, s:8   d: 75 °C 
		uint8_t REDUCE_LEVEL             : 8;  // 0x35.0, s:8   d: 0.4 % 
		uint8_t POWERUP_ACTION           : 1;  // 0x56.0, s:1   d: POWERUP_OFF  
		uint8_t                          : 7;  // 0x56.1, s:7   d:   
		uint8_t STATUSINFO_MINDELAY      : 5;  // 0x57.0, s:5   d: 2.0 s 
		uint8_t STATUSINFO_RANDOM        : 3;  // 0x57.5, s:3   d: 1.0 s 
		uint8_t CHARACTERISTIC           : 1;  // 0x58.0, s:1   d: CHARACTERISTIC_SQUARE  
		uint8_t                          : 7;  // 0x58.1, s:7   d:   
		uint8_t LOGIC_COMBINATION        : 5;  // 0x59.0, s:5   d: LOGIC_OR  
		uint8_t                          : 3;  // 0x59.5, s:3   d:   
	} *l1; // 9 byte

	struct s_lstPeer {
		uint8_t SHORT_CT_RAMPON          : 4;  // 0x01.0, s:4   d: X GE COND_VALUE_LO    ----------------------------------------
		uint8_t SHORT_CT_RAMPOFF         : 4;  // 0x01.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t SHORT_CT_ONDELAY         : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO  
		uint8_t SHORT_CT_OFFDELAY        : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO              30 Byte for a short struct to
		uint8_t SHORT_CT_ON              : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO              differentiate between short
		uint8_t SHORT_CT_OFF             : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO              and long values
		uint8_t SHORT_COND_VALUE_LO      : 8;  // 0x04.0, s:8   d: 50  
		uint8_t SHORT_COND_VALUE_HI      : 8;  // 0x05.0, s:8   d: 100  
		uint8_t SHORT_ONDELAY_TIME       : 8;  // 0x06.0, s:8   d: 0 s 
		uint8_t SHORT_ON_TIME            : 8;  // 0x07.0, s:8   d: 111600.0 s 
		uint8_t SHORT_OFFDELAY_TIME      : 8;  // 0x08.0, s:8   d: 0 s 
		uint8_t SHORT_OFF_TIME           : 8;  // 0x09.0, s:8   d: 111600.0 s 
		uint8_t SHORT_ACTION_TYPE        : 4;  // 0x0a.0, s:4   d: INACTIVE  
		uint8_t                          : 2;  // 0x0a.4, s:2   d:   
		uint8_t SHORT_OFF_TIME_MODE      : 1;  // 0x0a.6, s:1   d: ABSOLUTE  
		uint8_t SHORT_ON_TIME_MODE       : 1;  // 0x0a.7, s:1   d: ABSOLUTE  
		uint8_t SHORT_JT_ON              : 4;  // 0x0b.0, s:4   d: ONDELAY  
		uint8_t SHORT_JT_OFF             : 4;  // 0x0b.4, s:4   d: ONDELAY  
		uint8_t SHORT_JT_ONDELAY         : 4;  // 0x0c.0, s:4   d: ONDELAY  
		uint8_t SHORT_JT_OFFDELAY        : 4;  // 0x0c.4, s:4   d: ONDELAY  
		uint8_t SHORT_JT_RAMPON          : 4;  // 0x0d.0, s:4   d: ONDELAY  
		uint8_t SHORT_JT_RAMPOFF         : 4;  // 0x0d.4, s:4   d: ONDELAY  
		uint8_t                          : 5;
		uint8_t SHORT_OFFDELAY_BLINK     : 1;  // 0x0e.5, s:1   d: ON  
		uint8_t SHORT_ON_LEVEL_PRIO      : 1;  // 0x0e.6, s:1   d: HIGH  
		uint8_t SHORT_ONDELAY_MODE       : 1;  // 0x0e.7, s:1   d: SET_TO_OFF  
		uint8_t SHORT_OFF_LEVEL          : 8;  // 0x0f.0, s:8   d: 0.0 % 
		uint8_t SHORT_ON_MIN_LEVEL       : 8;  // 0x10.0, s:8   d: 0.1 % 
		uint8_t SHORT_ON_LEVEL           : 8;  // 0x11.0, s:8   d: 1.0 % 
		uint8_t SHORT_RAMP_START_STEP    : 8;  // 0x12.0, s:8   d: 0.05 % 
		uint8_t SHORT_RAMPON_TIME        : 8;  // 0x13.0, s:8   d: 0 s 
		uint8_t SHORT_RAMPOFF_TIME       : 8;  // 0x14.0, s:8   d: 0 s 
		uint8_t SHORT_DIM_MIN_LEVEL      : 8;  // 0x15.0, s:8   d: 0.0 % 
		uint8_t SHORT_DIM_MAX_LEVEL      : 8;  // 0x16.0, s:8   d: 1.0 % 
		uint8_t SHORT_DIM_STEP           : 8;  // 0x17.0, s:8   d: 0.0 % 
		uint8_t SHORT_OFFDELAY_STEP      : 8;  // 0x18.0, s:8   d: 0.05 % 
		uint8_t SHORT_OFFDELAY_NEWTIME   : 8;  // 0x19.0, s:8   d: 0.5 s 
		uint8_t SHORT_OFFDELAY_OLDTIME   : 8;  // 0x1a.0, s:8   d: 0.5 s 
		uint8_t SHORT_ELSE_ACTION_TYPE   : 4;  // 0x26.0, s:4   d: INACTIVE  
		uint8_t                          : 2;  // 0x26.4, s:2   d:   
		uint8_t SHORT_ELSE_OFF_TIME_MODE : 1;  // 0x26.6, s:1   d: ABSOLUTE  
		uint8_t SHORT_ELSE_ON_TIME_MODE  : 1;  // 0x26.7, s:1   d: ABSOLUTE  
		uint8_t SHORT_ELSE_JT_ON         : 4;  // 0x27.0, s:4   d: ONDELAY  
		uint8_t SHORT_ELSE_JT_OFF        : 4;  // 0x27.4, s:4   d: ONDELAY  
		uint8_t SHORT_ELSE_JT_ONDELAY    : 4;  // 0x28.0, s:4   d: ONDELAY  
		uint8_t SHORT_ELSE_JT_OFFDELAY   : 4;  // 0x28.4, s:4   d: ONDELAY  
		uint8_t SHORT_ELSE_JT_RAMPON     : 4;  // 0x29.0, s:4   d: ONDELAY  
		uint8_t SHORT_ELSE_JT_RAMPOFF    : 4;  // 0x29.4, s:4   d: ONDELAY               ----------------------------------------
		uint8_t LONG_CT_RAMPON           : 4;  // 0x81.0, s:4   d: X GE COND_VALUE_LO    ---------------------------------------- 
		uint8_t LONG_CT_RAMPOFF          : 4;  // 0x81.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_CT_ONDELAY          : 4;  // 0x82.0, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_CT_OFFDELAY         : 4;  // 0x82.4, s:4   d: X GE COND_VALUE_LO              another 30 byte for the long
		uint8_t LONG_CT_ON               : 4;  // 0x83.0, s:4   d: X GE COND_VALUE_LO  			   message portion 
		uint8_t LONG_CT_OFF              : 4;  // 0x83.4, s:4   d: X GE COND_VALUE_LO  
		uint8_t LONG_COND_VALUE_LO       : 8;  // 0x84.0, s:8   d: 50  
		uint8_t LONG_COND_VALUE_HI       : 8;  // 0x85.0, s:8   d: 100  
		uint8_t LONG_ONDELAY_TIME        : 8;  // 0x86.0, s:8   d: 0 s 
		uint8_t LONG_ON_TIME             : 8;  // 0x87.0, s:8   d: 111600.0 s 
		uint8_t LONG_OFFDELAY_TIME       : 8;  // 0x88.0, s:8   d: 0 s 
		uint8_t LONG_OFF_TIME            : 8;  // 0x89.0, s:8   d: 111600.0 s 
		uint8_t LONG_ACTION_TYPE         : 4;  // 0x8a.0, s:4   d: INACTIVE  
		uint8_t                          : 1;  // 0x8a.4, s:1   d:   
		uint8_t LONG_MULTIEXECUTE        : 1;  // 0x8a.5, s:1   d: OFF  
		uint8_t LONG_OFF_TIME_MODE       : 1;  // 0x8a.6, s:1   d: ABSOLUTE  
		uint8_t LONG_ON_TIME_MODE        : 1;  // 0x8a.7, s:1   d: ABSOLUTE  
		uint8_t LONG_JT_ON               : 4;  // 0x8b.0, s:4   d: ONDELAY  
		uint8_t LONG_JT_OFF              : 4;  // 0x8b.4, s:4   d: ONDELAY  
		uint8_t LONG_JT_ONDELAY          : 4;  // 0x8c.0, s:4   d: ONDELAY  
		uint8_t LONG_JT_OFFDELAY         : 4;  // 0x8c.4, s:4   d: ONDELAY  
		uint8_t LONG_JT_RAMPON           : 4;  // 0x8d.0, s:4   d: ONDELAY  
		uint8_t LONG_JT_RAMPOFF          : 4;  // 0x8d.4, s:4   d: ONDELAY  
		uint8_t                          : 5;
		uint8_t LONG_OFFDELAY_BLINK      : 1;  // 0x8e.5, s:1   d: ON  
		uint8_t LONG_ON_LEVEL_PRIO       : 1;  // 0x8e.6, s:1   d: HIGH  
		uint8_t LONG_ONDELAY_MODE        : 1;  // 0x8e.7, s:1   d: SET_TO_OFF  
		uint8_t LONG_OFF_LEVEL           : 8;  // 0x8f.0, s:8   d: 0.0 % 
		uint8_t LONG_ON_MIN_LEVEL        : 8;  // 0x90.0, s:8   d: 0.1 % 
		uint8_t LONG_ON_LEVEL            : 8;  // 0x91.0, s:8   d: 1.0 % 
		uint8_t LONG_RAMP_START_STEP     : 8;  // 0x92.0, s:8   d: 0.05 % 
		uint8_t LONG_RAMPON_TIME         : 8;  // 0x93.0, s:8   d: 0 s 
		uint8_t LONG_RAMPOFF_TIME        : 8;  // 0x94.0, s:8   d: 0 s 
		uint8_t LONG_DIM_MIN_LEVEL       : 8;  // 0x95.0, s:8   d: 0.0 % 
		uint8_t LONG_DIM_MAX_LEVEL       : 8;  // 0x96.0, s:8   d: 1.0 % 
		uint8_t LONG_DIM_STEP            : 8;  // 0x97.0, s:8   d: 0.0 % 
		uint8_t LONG_OFFDELAY_STEP       : 8;  // 0x98.0, s:8   d: 0.05 % 
		uint8_t LONG_OFFDELAY_NEWTIME    : 8;  // 0x99.0, s:8   d: 0.5 s 
		uint8_t LONG_OFFDELAY_OLDTIME    : 8;  // 0x9a.0, s:8   d: 0.5 s 
		uint8_t LONG_ELSE_ACTION_TYPE    : 4;  // 0xa6.0, s:4   d: INACTIVE  
		uint8_t                          : 1;  // 0xa6.4, s:1   d:   
		uint8_t LONG_ELSE_MULTIEXECUTE   : 1;  // 0xa6.5, s:1   d: OFF  
		uint8_t LONG_ELSE_OFF_TIME_MODE  : 1;  // 0xa6.6, s:1   d: ABSOLUTE  
		uint8_t LONG_ELSE_ON_TIME_MODE   : 1;  // 0xa6.7, s:1   d: ABSOLUTE  
		uint8_t LONG_ELSE_JT_ON          : 4;  // 0xa7.0, s:4   d: ONDELAY  
		uint8_t LONG_ELSE_JT_OFF         : 4;  // 0xa7.4, s:4   d: ONDELAY  
		uint8_t LONG_ELSE_JT_ONDELAY     : 4;  // 0xa8.0, s:4   d: ONDELAY  
		uint8_t LONG_ELSE_JT_OFFDELAY    : 4;  // 0xa8.4, s:4   d: ONDELAY  
		uint8_t LONG_ELSE_JT_RAMPON      : 4;  // 0xa9.0, s:4   d: ONDELAY  
		uint8_t LONG_ELSE_JT_RAMPOFF     : 4;  // 0xa9.4, s:4   d: ONDELAY               ----------------------------------------
	} *l3F; // 60 byte


	struct s_l3 {
		uint8_t CT_RAMPON          : 4;  // 0x01.0, s:4   d: X GE COND_VALUE_LO    // 0x81.0, s:4   d: X GE COND_VALUE_LO
		uint8_t CT_RAMPOFF         : 4;  // 0x01.4, s:4   d: X GE COND_VALUE_LO    // 0x81.4, s:4   d: X GE COND_VALUE_LO 
		uint8_t CT_ONDELAY         : 4;  // 0x02.0, s:4   d: X GE COND_VALUE_LO    // 0x82.0, s:4   d: X GE COND_VALUE_LO
		uint8_t CT_OFFDELAY        : 4;  // 0x02.4, s:4   d: X GE COND_VALUE_LO    // 0x82.4, s:4   d: X GE COND_VALUE_LO 
		uint8_t CT_ON              : 4;  // 0x03.0, s:4   d: X GE COND_VALUE_LO    // 0x83.0, s:4   d: X GE COND_VALUE_LO
		uint8_t CT_OFF             : 4;  // 0x03.4, s:4   d: X GE COND_VALUE_LO    // 0x83.4, s:4   d: X GE COND_VALUE_LO   
		uint8_t COND_VALUE_LO      : 8;  // 0x04.0, s:8   d: 50                    // 0x84.0, s:8   d: 50
		uint8_t COND_VALUE_HI      : 8;  // 0x05.0, s:8   d: 100                   // 0x85.0, s:8   d: 100 
		uint8_t ONDELAY_TIME       : 8;  // 0x06.0, s:8   d: 0 s                   // 0x86.0, s:8   d: 0 s
		uint8_t ON_TIME            : 8;  // 0x07.0, s:8   d: 111600.0 s            // 0x87.0, s:8   d: 111600.0 s 
		uint8_t OFFDELAY_TIME      : 8;  // 0x08.0, s:8   d: 0 s                   // 0x88.0, s:8   d: 0 s
		uint8_t OFF_TIME           : 8;  // 0x09.0, s:8   d: 111600.0 s            // 0x89.0, s:8   d: 111600.0 s
		uint8_t ACTION_TYPE        : 4;  // 0x0a.0, s:4   d: INACTIVE              // 0x8a.0, s:4   d: INACTIVE
		uint8_t                    : 1;  // 0x0a.4, s:2   d:                       // 0x8a.4, s:1   d: 
		uint8_t MULTIEXECUTE       : 1;                                            // 0x8a.5, s:1   d: OFF
		uint8_t OFF_TIME_MODE      : 1;  // 0x0a.6, s:1   d: ABSOLUTE              // 0x8a.6, s:1   d: ABSOLUTE
		uint8_t ON_TIME_MODE       : 1;  // 0x0a.7, s:1   d: ABSOLUTE              // 0x8a.7, s:1   d: ABSOLUTE
		uint8_t JT_ON              : 4;  // 0x0b.0, s:4   d: ONDELAY               // 0x8b.0, s:4   d: ONDELAY 
		uint8_t JT_OFF             : 4;  // 0x0b.4, s:4   d: ONDELAY               // 0x8b.4, s:4   d: ONDELAY
		uint8_t JT_ONDELAY         : 4;  // 0x0c.0, s:4   d: ONDELAY               // 0x8c.0, s:4   d: ONDELAY
		uint8_t JT_OFFDELAY        : 4;  // 0x0c.4, s:4   d: ONDELAY               // 0x8c.4, s:4   d: ONDELAY
		uint8_t JT_RAMPON          : 4;  // 0x0d.0, s:4   d: ONDELAY               // 0x8d.0, s:4   d: ONDELAY 
		uint8_t JT_RAMPOFF         : 4;  // 0x0d.4, s:4   d: ONDELAY               // 0x8d.4, s:4   d: ONDELAY
		uint8_t                    : 5;  // 0x0e.0, s:5
		uint8_t OFFDELAY_BLINK     : 1;  // 0x0e.5, s:1   d: ON                    // 0x8e.5, s:1   d: ON  
		uint8_t ON_LEVEL_PRIO      : 1;  // 0x0e.6, s:1   d: HIGH                  // 0x8e.6, s:1   d: HIGH  
		uint8_t ONDELAY_MODE       : 1;  // 0x0e.7, s:1   d: SET_TO_OFF            // 0x8e.7, s:1   d: SET_TO_OFF  
		uint8_t OFF_LEVEL          : 8;  // 0x0f.0, s:8   d: 0.0 %                 // 0x8f.0, s:8   d: 0.0 % 
		uint8_t ON_MIN_LEVEL       : 8;  // 0x10.0, s:8   d: 0.1 %                 // 0x90.0, s:8   d: 0.1 %
		uint8_t ON_LEVEL           : 8;  // 0x11.0, s:8   d: 1.0 %                 // 0x91.0, s:8   d: 1.0 % 
		uint8_t RAMP_START_STEP    : 8;  // 0x12.0, s:8   d: 0.05 %                // 0x92.0, s:8   d: 0.05 %
		uint8_t RAMPON_TIME        : 8;  // 0x13.0, s:8   d: 0 s                   // 0x93.0, s:8   d: 0 s 
		uint8_t RAMPOFF_TIME       : 8;  // 0x14.0, s:8   d: 0 s                   // 0x94.0, s:8   d: 0 s 
		uint8_t DIM_MIN_LEVEL      : 8;  // 0x15.0, s:8   d: 0.0 %                 // 0x95.0, s:8   d: 0.0 %
		uint8_t DIM_MAX_LEVEL      : 8;  // 0x16.0, s:8   d: 1.0 %                 // 0x96.0, s:8   d: 1.0 %
		uint8_t DIM_STEP           : 8;  // 0x17.0, s:8   d: 0.0 %                 // 0x97.0, s:8   d: 0.0 % 
		uint8_t OFFDELAY_STEP      : 8;  // 0x18.0, s:8   d: 0.05 %                // 0x98.0, s:8   d: 0.05 %  
		uint8_t OFFDELAY_NEWTIME   : 8;  // 0x19.0, s:8   d: 0.5 s                 // 0x99.0, s:8   d: 0.5 s 
		uint8_t OFFDELAY_OLDTIME   : 8;  // 0x1a.0, s:8   d: 0.5 s                 // 0x9a.0, s:8   d: 0.5 s
		uint8_t ELSE_ACTION_TYPE   : 4;  // 0x26.0, s:4   d: INACTIVE              // 0xa6.0, s:4   d: INACTIVE 
		uint8_t                    : 1;  // 0x26.4, s:2   d:                       // 0xa6.4, s:1   d:
		uint8_t ELSE_MULTIEXECUTE  : 1;                                            // 0xa6.5, s:1   d: OFF                                                         
		uint8_t ELSE_OFF_TIME_MODE : 1;  // 0x26.6, s:1   d: ABSOLUTE              // 0xa6.6, s:1   d: ABSOLUTE
		uint8_t ELSE_ON_TIME_MODE  : 1;  // 0x26.7, s:1   d: ABSOLUTE              // 0xa6.7, s:1   d: ABSOLUTE
		uint8_t ELSE_JT_ON         : 4;  // 0x27.0, s:4   d: ONDELAY               // 0xa7.0, s:4   d: ONDELAY
		uint8_t ELSE_JT_OFF        : 4;  // 0x27.4, s:4   d: ONDELAY               // 0xa7.4, s:4   d: ONDELAY
		uint8_t ELSE_JT_ONDELAY    : 4;  // 0x28.0, s:4   d: ONDELAY               // 0xa8.0, s:4   d: ONDELAY
		uint8_t ELSE_JT_OFFDELAY   : 4;  // 0x28.4, s:4   d: ONDELAY               // 0xa8.4, s:4   d: ONDELAY
		uint8_t ELSE_JT_RAMPON     : 4;  // 0x29.0, s:4   d: ONDELAY               // 0xa9.0, s:4   d: ONDELAY
		uint8_t ELSE_JT_RAMPOFF    : 4;  // 0x29.4, s:4   d: ONDELAY               // 0xa9.4, s:4   d: ONDELAY
	} *l3; // 30 byte

	static void initDimmer(uint8_t channel);												// functions in user sketch needed
	static void switchDimmer(uint8_t channel, uint8_t status);

public:  //----------------------------------------------------------------------------------------------------------------

	cmDimmer(const uint8_t peer_max);														// constructor

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
	} tr40;

	s_cm_status cm_status;																	// defined in cmMaster.h, holds current status and set_satatus

	inline void adjustStatus(void);															// setting of relay status

	virtual void request_peer_defaults(uint8_t idx, s_m01xx01 *buf);						// add peer channel defaults to list3/4

	virtual void cm_init(void);																// init function, called after AS initialisation
	virtual void cm_poll(void);																// poll function, driven by HM loop
	virtual void set_toggle(void);															// toggle the module initiated by config button

	/* receive functions to handle requests forwarded by AS:processMessage
	*  only channel module related requests are forwarded, majority of requests are handled within main AS class */
	virtual void CONFIG_STATUS_REQUEST(s_m01xx0e *buf);										// master is asking for channel status
	virtual void INSTRUCTION_SET(s_m1102xx *buf);											// master wants to set channel status
	virtual void INSTRUCTION_INHIBIT_OFF(s_m1100xx *buf);									// deny access to switch, set_toogle and Remote 
	virtual void INSTRUCTION_INHIBIT_ON(s_m1101xx *buf);									// grant access to switch
	virtual void SWITCH(s_m3Exxxx *buf);													// switch message from master to test a peer setup
	virtual void REMOTE(s_m40xxxx *buf);													// remote peer message
	virtual void SENSOR_EVENT(s_m41xxxx *buf);												// sensor peer message

};

#endif
