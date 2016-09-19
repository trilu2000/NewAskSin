/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin debug flag register --------------------------------------------------------------------------------------------
* - to make debug more central and easier
* - -----------------------------------------------------------------------------------------------------------------------
*/


/*
* @brief Remove the double slash in the respective line to enable debug messages 
*        for the module/class you want to see debug messages
*/

#define SER_DBG					// Main sketch debug function and messages
//#define CC_DBG				// Communication class (CC1101.cpp)
#define SN_DBG					// Send class (Send.cpp)
#define RV_DBG					// Receive class (Receive.cpp)
//#define RV_DBG_EX				// Further explanation of received messages (Receive.cpp)

//#define EL_DBG				// EEprom list class (EEprom_list.cpp)
//#define EP_DBG				// EEprom peer class (EEprom_peer.cpp)
//#define EE_DBG				// EEprom class (EEprom.cpp)

//#define CM_DBG				// Channel Master module (cmMaster.cpp)
//#define MN_DBG				// Maintenance channel module (cmMaintenance.cpp)
//#define SW_DBG				// Switsch channel module (cmSwitch.cpp)


/*
* @brief This macro has to be in every .cpp file to enable the DBG() message.
*        Replace the XX_DBG with your choosen abbreviation.
*

#include "00_debug-flag.h"
#ifdef XX_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif

*/





