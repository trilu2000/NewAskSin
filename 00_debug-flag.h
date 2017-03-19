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


#include "HAL.h"


/*
* @brief Remove the double slash in the respective line to enable debug messages 
*        for the module/class you want to see debug messages
*/

#define SER_DBG					// Main sketch debug function and messages
#define DMP_DBG					// EEprom dump function, only in combination with SER_DBG
//#define AS_DBG					// AskSin class debug (AS.cpp)
//#define CC_DBG					// Communication class (AS_communication.cpp)
#define SN_DBG					// Send function (AS.cpp)
//#define RV_DBG					// Receive function (AS.cpp)
//#define CB_DBG					// Config button class (AS_conf_button.cpp)
//#define LD_DBG					// Status led class (AS_status_led.cpp)

//#define CM_DBG					// Channel Master module (cmMaster.cpp)
//#define MN_DBG					// Maintenance channel module (cmMaintenance.cpp)
//#define SW_DBG				// Switsch channel module (cmSwitch.cpp)
#define DM_DBG					// Dimmer channel module (cmDimmer.cpp)
//#define RE_DBG					// Remote channel module (cmRemote.cpp)


/*
* @brief This macro has to be in every .cpp file to enable the DBG() message.
*        Replace the XX_DBG with your choosen abbreviation.
*

#include "00_debug-flag.h"

*/


template<class T> inline Print &operator ,(Print &obj, T arg) { return obj << arg; }

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
	#define DBG_RV(...) 
#endif

/* config button class */
#ifdef CB_DBG
	#define DBG_CB(...) Serial ,__VA_ARGS__
#else
	#define DBG_CB(...) 
#endif

/* status led class */
#ifdef LD_DBG
	#define DBG_LD(...) Serial ,__VA_ARGS__
#else
	#define DBG_LD(...) 
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

/* channel module remote */
#ifdef RE_DBG
	#define DBG_RE(...) Serial ,__VA_ARGS__
#else
	#define DBG_RE(...) 
#endif





#endif



