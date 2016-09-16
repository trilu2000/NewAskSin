/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Maintenance -------------------------------------------------------------------------------------
* - list 0 handling for self created devices
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#ifdef MN_DBG
#define DBG(...) Serial ,__VA_ARGS__
#else
#define DBG(...) 
#endif


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module maintenance
*        pointer to channel table are forwarded to the master class.
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/
#include "cmMaintenance.h"

cmMaintenance::cmMaintenance(const s_cnlTbl *ptr_cnlTbl, const s_cnlTbl *ptr_peerTbl, const s_peerTbl *ptr_peerDB) : cmMaster(ptr_cnlTbl, ptr_peerTbl, ptr_peerDB) {
	DBG(F("cmMaint, cnl: "), cT->cnl, '\n');
}


/*
* @brief Here we are informed of every change in list0 and we can get the defaults
*        like MasterID or resend time and counter if applicable
*/
void cmMaintenance::info_config_change(void) {
	// get the master id by finding the pointer in progmem cnlAddr and placing the pointer of MAID to it
	for (uint8_t i = 0; i < cLT->sLen; i++) {
		if (_PGM_BYTE(cnlAddr[i + cLT->sIdx]) == 0x0a) MAID = &chnl_list[i];
	}

	DBG( F("cmMaint, config_change - MAID:"), _HEX(MAID,3), '\n' );

}
