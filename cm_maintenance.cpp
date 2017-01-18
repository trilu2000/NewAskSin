/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Maintenance -------------------------------------------------------------------------------------
* - list 0 handling for self created devices
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


/**------------------------------------------------------------------------------------------------------------------------
*- mandatory functions for every new module to communicate within HM protocol stack -
* -------------------------------------------------------------------------------------------------------------------------
*
* @brief Constructor for channel module maintenance
*        pointer to channel table are forwarded to the master class.
*        Constructor of master class is processed first.
*        Setup of class specific things is done here
*/



CM_MAINTENANCE::CM_MAINTENANCE(const uint8_t peer_max) : CM_MASTER(peer_max ) {
	//static uint8_t lstC_val[cm_maintenance_ChnlLen];

	lstC.lst = 0;
	lstC.reg = cm_maintenance_ChnlReg;
	lstC.def = cm_maintenance_ChnlDef;
	lstC.len = cm_maintenance_ChnlLen;
	lstC.val = new uint8_t[lstC.len];														// create and allign the value arrays
	//lstC.val = &lstC_val;

	lstP.lst = 255;																			// lstP doesn't exist...
}


/*
* @brief Here we are informed of every change in list0 and we can get the defaults
*        like MasterID or resend time and counter if applicable
*/
void CM_MAINTENANCE::info_config_change(void) {
	/* get the master id by finding the pointer in progmem cnlAddr and placing the pointer of MAID to it 
	*  if register 0x0a didn't exist, we are getting a NULL pointer with a null value, writing into it 
	* will reset the device, failure needs not to be solved, while a device where the pair address is not valid
	* has probably more issues */
	uint8_t *ptr_pair = lstC.ptr_to_val(0x0a);
	dev_operate.MAID = ptr_pair;

	/* handle the aes flag (0x08) in list0 - flag does probably not exist therefore we check the pointer and if it is a 
	*  NULL pointer, we create a byte and allign it with the AES flag */
	static uint8_t aes = 0;
	dev_operate.AES_FLAG = lstC.ptr_to_val(0x08);
	if (!dev_operate.AES_FLAG) dev_operate.AES_FLAG = &aes;

	snd_msg.max_retr = 3;		//or set TRANSMIT_DEV_TRY_MAX
	snd_msg.max_time = 300;

	DBG(MN, F("MN:config_change - MAID:"), _HEX(dev_operate.MAID,3), '\n' );

}

void CM_MAINTENANCE::cm_poll(void) {
}