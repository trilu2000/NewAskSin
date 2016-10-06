/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Maintenance -------------------------------------------------------------------------------------
* - list 0 handling for self created devices
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"


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



cmMaintenance::cmMaintenance(const uint8_t peer_max) : cmMaster(peer_max ) {

	lstC.lst = 0;
	lstC.reg = cmMaintenance_ChnlReg;
	lstC.def = cmMaintenance_ChnlDef;
	lstC.len = cmMaintenance_ChnlLen;

	lstP.lst = 255;																			// in principle not needed...
	//lstP.reg = cmMaintenance_PeerReg;														// while channel 0 has only list 0
	//lstP.def = cmMaintenance_PeerDef;
	//lstP.len = sizeof(cmMaintenance_PeerReg);

	DBG(MN, F("MN. cnl: "), lstC.cnl, '\n');
}


/*
* @brief Here we are informed of every change in list0 and we can get the defaults
*        like MasterID or resend time and counter if applicable
*/
void cmMaintenance::info_config_change(void) {
	// get the master id by finding the pointer in progmem cnlAddr and placing the pointer of MAID to it
	uint8_t *t = lstC.ptr_to_val(0x0a);
	if (t) MAID = t;
	
	snd_msg.max_retr = 3;		//or set TRANSMIT_DEV_TRY_MAX
	snd_msg.max_time = 300;

	DBG(MN, F("MN:config_change - MAID:"), _HEX(MAID,3), '\n' );

}
