//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin th sensor class ------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _THSENSOR_H
#define _THSENSOR_H

#include "AS.h"
#include "HAL.h"

// default settings for list3 or list4
const uint8_t peerOdd[] =    {
};
const uint8_t peerEven[] =   {
};
const uint8_t peerSingle[] = {
};


class THSensor {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------
	struct s_lstCnl {
	} lstCnl;

	struct s_lstPeer {
		// 0x01,
		uint8_t  peerNeedsBurst      :1;     // 0x01, s:0, e:1
		uint8_t                      :7;     //
	} lstPeer;
	  
  
  //- user defined functions ----------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
	void (*fInit)(void);																	// pointer to init function in main sketch
	void (*fSwitch)(uint8_t,uint8_t);														// pointer to switch function (PWM) in main sketch, first value is PWM level, second the characteristics

	void    config();


  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
	uint8_t modStat;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t modDUL;																			// module down up low battery byte
	uint8_t regCnl;																			// holds the channel for the module

	AS      *hm;																			// pointer to HM class instance

	void    configCngEvent(void);															// list1 on registered channel had changed
	void    pairSetEvent(uint8_t *data, uint8_t len);										// pair message to specific channel, handover information for value, ramp time and so on
	void    pairStatusReq(void);															// event on status request
	void    peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);							// peer message was received on the registered channel, handover the message bytes and length

	void    poll(void);																		// poll function, driven by HM loop

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void    regInHM(uint8_t cnl, uint8_t lst, AS *instPtr);									// register this module in HM on the specific channel
	void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
	void    peerAddEvent(uint8_t *data, uint8_t len);										// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
};


#endif
