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

#define TIMEOUT_DBL_LONG        5000
#define TIMEOUT_REPEATED_LONG   300


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
		// 0x04:0x10, 0x08:0x00, 0x09:0x00,
		uint8_t                            :4;    // 0x04.0
		uint8_t LONG_PRESS_TIME            :4;    // 0x04.4, 0x01
		uint8_t AES_FLAG                   :1;    // 0x08.0, 0x00
		uint8_t                            :7;    // 0x08.1
		uint8_t DBL_PRESS_TIME             :4;    // 0x09.0, 0x00
		uint8_t                            :4;    // 0x09.4
	} lstCnl;

	struct s_lstPeer {
		// 0x01,
		uint8_t PEER_NEEDS_BURST           :1;    // 0x01.0, 0x00
		uint8_t                            :6;    // 0x01.1
		uint8_t EXPECT_AES                 :1;    // 0x01.7, 0x00
	} lstPeer;

	waitTimer wt_long_press;
	waitTimer wt_rpt_long;
	waitTimer wt_dbl_short;
	waitTimer wt_dbl_long;

	uint8_t stat_curr;																			// variable to store current status in polling function
	uint8_t stat_last;																			// same, to remember on last status

	struct s_check_repeat {
		uint8_t poll                       :1;													// if this is set to 1, poll function should be entered
		uint8_t last_short                 :1;													// if the last key press was a short to detect a double short
		uint8_t last_long                  :1;													// if the last keypress was a long to detect a double long
		uint8_t last_dbl_long              :1;													// last key was a double_long, to detect the end of a double long
		uint8_t last_rpt_long              :1;													// if the key is still pressed for a certain time to detect repeat status over a timer
		uint8_t                            :3;
	} check_repeat;


  public://----------------------------------------------------------------------------------------------------------------
  //- user defined functions ----------------------------------------------------------------------------------------------
	
	struct s_buttonInfo {
		uint8_t channel      :6;																// set in regInHM function, will not change at runtime
		uint8_t longpress    :1;																// will be set in buttonAction function
		uint8_t lowbat       :1;																// placeholder here, will be set in as module
		uint8_t counter      :8;																// will be increased in buttonAction function
	} buttonInfo;																				// holds the details for the send event message

	void    buttonAction(uint8_t);																// send message according given event
	

  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
	uint8_t regCnl;																				// holds the channel for the module

	AS      *hm;																				// pointer to HM class instance

	void    setToggle(void);																	// toggle the module initiated by config button
	void    configCngEvent(void);																// list1 on registered channel had changed
	void    pairSetEvent(uint8_t *data, uint8_t len);											// pair message to specific channel, handover information for value, ramp time and so on
	void    pairStatusReq(void);																// event on status request
	void    peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);								// peer message was received on the registered channel, handover the message bytes and length

	void    poll(void);																			// poll function, driven by HM loop

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void    regInHM(uint8_t cnl, uint8_t lst, AS *instPtr);										// register this module in HM on the specific channel
	void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);	// call back address for HM for informing on events
	void    peerAddEvent(uint8_t *data, uint8_t len);											// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
};

#endif
