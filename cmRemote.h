//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin relay class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _cmRemote_H
#define _cmRemote_H

#include "AS.h"
#include "HAL.h"

// default settings for list4
const uint8_t peerSingle[] = {
	// Default actor single: 01:00 
	0x00, 
};


class cmRemote {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------

	struct s_lstCnl {
		// 0x01, 0x02, 0x0a, 0x0b, 0x0c, 0x18,
		uint8_t LONG_PRESS_TIME            :4;      // 0x04.4, 0x01
		uint8_t AES_FLAG                   :1;      // 0x08.0, 0x00
		uint8_t                            :7;      // 0x08.1
		uint8_t DBL_PRESS_TIME             :4;      // 0x09.0, 0x00
		uint8_t                            :4;      // 0x09.4
	} lstCnl;

	struct s_lstPeer {
		// 0x01,
		uint8_t PEER_NEEDS_BURST           :1;      // 0x01.0, 0x00
		uint8_t                            :6;      // 0x01.1
		uint8_t EXPECT_AES                 :1;      // 0x01.7, 0x00
	} lstPeer;



  public://----------------------------------------------------------------------------------------------------------------
  //- user defined functions ----------------------------------------------------------------------------------------------
	


  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
	uint8_t modStat;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t modDUL;																			// module down up low battery byte
	uint8_t regCnl;																			// holds the channel for the module

	AS      *hm;																			// pointer to HM class instance

	void    setToggle(void);																// toggle the module initiated by config button
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
