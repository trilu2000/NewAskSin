/*- -----------------------------------------------------------------------------------------------------------------------

*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin debug flag register --------------------------------------------------------------------------------------------
* - to make debug more central and easier
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _DEBUG_FLAG_H
#define _DEBUG_FLAG_H


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "HAL.h"


/*
* @brief Remove the double slash in the respective line to enable debug messages 
*        for the module/class you want to see debug messages
*/

#define SER_DBG					// Main sketch debug function and messages
//#define AS_DBG					// AskSin class debug (AS.cpp)
//#define CC_DBG					// Communication class (CC1101.cpp)
#define SN_DBG					// Send class (Send.cpp)
#define RV_DBG					// Receive class (Receive.cpp)
//#define RV_DBG_EX				// Further explanation of received messages (Receive.cpp)

//#define EL_DBG				// EEprom list class (EEprom_list.cpp)
//#define EP_DBG				// EEprom peer class (EEprom_peer.cpp)

//#define CM_DBG					// Channel Master module (cmMaster.cpp)
//#define MN_DBG					// Maintenance channel module (cmMaintenance.cpp)
//#define SW_DBG				// Switsch channel module (cmSwitch.cpp)
#define DM_DBG					// Dimmer channel module (cmDimmer.cpp)


/*
* @brief This macro has to be in every .cpp file to enable the DBG() message.
*        Replace the XX_DBG with your choosen abbreviation.
*

#include "00_debug-flag.h"

*/


#define dbg Serial

#define PRIMITIVE_CAT(a, b, ...) a ## b(__VA_ARGS__)

#define DBG( MODULE, ...) PRIMITIVE_CAT(DBG_, MODULE, __VA_ARGS__)
#define DBG_START( MODULE, ...) PRIMITIVE_CAT(DBG_START_, MODULE, __VA_ARGS__)

#define _DBG_START(...)   power_usart0_enable();Serial.begin(57600);Serial ,__VA_ARGS__;

/* main sketch */
#ifdef SER_DBG
	#define DBG_START_SER(...) _DBG_START(__VA_ARGS__)
	#define DBG_SER(...) Serial ,__VA_ARGS__
#else
	#define DBG_START_SER(...)
	#define DBG_SER(...) 
#endif

/* asksin main class */
#ifdef AS_DBG
	#define DBG_AS(...) Serial ,__VA_ARGS__
#else
	#define DBG_AS(...) 
#endif

/* cc1101 class */
#ifdef CC_DBG
	#define DBG_CC(...) Serial ,__VA_ARGS__
#else
	#define DBG_CC(...) 
#endif

/* send module */
#ifdef SN_DBG
	#define DBG_SN(...) Serial ,__VA_ARGS__
#else
	#define DBG_SN(...) 
#endif

/* receive module */
#ifdef RV_DBG
	#define DBG_RV(...) Serial ,__VA_ARGS__
#else
	#define DBG_START_RV(...)
	#define DBG_RV(...) 
#endif

/* channel master class */
#ifdef CM_DBG
	#define DBG_CM(...) Serial ,__VA_ARGS__
#else
	#define DBG_CM(...) 
#endif

/* maintenance channel module */
#ifdef MN_DBG
#define DBG_MN(...) Serial ,__VA_ARGS__
#else
#define DBG_MN(...) 
#endif

/* channel module switch */
#ifdef SW_DBG
	#define DBG_SW(...) Serial ,__VA_ARGS__
#else
	#define DBG_SW(...) 
#endif

/* channel module dimmer */
#ifdef DM_DBG
	#define DBG_DM(...) Serial ,__VA_ARGS__
#else
	#define DBG_DM(...) 
#endif


/**	- ---------------------------------------------------------------------------------------------------------------------
*
* @brief Template and some functions for debugging over serial interface
* http://aeroquad.googlecode.com/svn/branches/pyjamasam/WIFIReceiver/Streaming.h
*/

// template to enable DBG, "sometext", $a, '\n';
template<class T> inline Print &operator ,(Print &obj, T arg) { obj.print(arg); return obj; }
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }


#define _HI_HEX_BITS(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)
#define _LO_HEX_BITS(x)  char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)

struct _HEXB {
	uint8_t val;
	_HEXB(uint8_t v) : val(v) {}
};
inline Print &operator  ,(Print &obj, const _HEXB &arg) { obj.print(_HI_HEX_BITS(arg.val)); obj.print(_LO_HEX_BITS(arg.val)); return obj; }
inline Print &operator <<(Print &obj, const _HEXB &arg) { obj.print(_HI_HEX_BITS(arg.val)); obj.print(_LO_HEX_BITS(arg.val)); return obj; }

struct _HEX {
	uint8_t *val;
	uint8_t len;
	_HEX(uint8_t *v, uint8_t l) : val(v), len(l) {}
};
inline Print &operator  ,(Print &obj, const _HEX &arg) { for (uint8_t i = 0; i<arg.len; i++) { obj.print(_HI_HEX_BITS(arg.val[i])); obj.print(_LO_HEX_BITS(arg.val[i])); if (i<=arg.len) obj.print(' '); }; return obj; }
inline Print &operator <<(Print &obj, const _HEX &arg) { for (uint8_t i = 0; i<arg.len; i++) { obj.print(_HI_HEX_BITS(arg.val[i])); obj.print(_LO_HEX_BITS(arg.val[i])); if (i<=arg.len) obj.print(' '); }; return obj; }

enum _eTIME { _TIME };
inline Print &operator  ,(Print &obj, _eTIME arg) { obj.print('('); obj.print(getMillis()); obj.print(')'); return obj; }
inline Print &operator <<(Print &obj, _eTIME arg) { obj.print('('); obj.print(getMillis()); obj.print(')'); return obj; }



#endif



