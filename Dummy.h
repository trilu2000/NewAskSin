//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin dummy class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _DUMMY_H
#define _DUMMY_H

#include "AS.h"
#include "HAL.h"

// default settings for list3 or list4
const uint8_t peerOdd[] =    {
	// Default actor dual 1: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:64 0C:66 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:64 8C:66
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x64, 0x66,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x64, 0x66
};
const uint8_t peerEven[] =   {
	// Default actor dual 2: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:13 0C:33 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:13 8C:33
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x13, 0x33,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x13, 0x33
};
const uint8_t peerSingle[] = {
	// Default actor single: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01
	// 0B:14 0C:63 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:14 8C:63
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x14, 0x63,
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x14, 0x63,
};


class Dummy {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------
	struct s_lstCnl {
		// 0x08,
		uint8_t  sign                :1;     // 0x08, s:0, e:1
		uint8_t                      :7;     //
	} lstCnl;

	struct s_lstPeer {
		// 0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		uint8_t  shCtDlyOn           :4;     // 0x02, s:0, e:4
		uint8_t  shCtDlyOff          :4;     // 0x02, s:4, e:8
		uint8_t  shCtOn              :4;     // 0x03, s:0, e:4
		uint8_t  shCtOff             :4;     // 0x03, s:4, e:8
		uint8_t  shCtValLo;                  // 0x04, s:0, e:0
		uint8_t  shCtValHi;                  // 0x05, s:0, e:0
		uint8_t  shOnDly;                    // 0x06, s:0, e:0
		uint8_t  shOnTime;                   // 0x07, s:0, e:0
		uint8_t  shOffDly;                   // 0x08, s:0, e:0
		uint8_t  shOffTime;                  // 0x09, s:0, e:0
		uint8_t  shActionType        :2;     // 0x0a, s:0, e:2
		uint8_t                      :4;     //
		uint8_t  shOffTimeMode       :1;     // 0x0a, s:6, e:7
		uint8_t  shOnTimeMode        :1;     // 0x0a, s:7, e:8
		uint8_t  shSwJtOn            :4;     // 0x0b, s:0, e:4
		uint8_t  shSwJtOff           :4;     // 0x0b, s:4, e:8
		uint8_t  shSwJtDlyOn         :4;     // 0x0c, s:0, e:4
		uint8_t  shSwJtDlyOff        :4;     // 0x0c, s:4, e:8
		uint8_t  lgCtDlyOn           :4;     // 0x82, s:0, e:4
		uint8_t  lgCtDlyOff          :4;     // 0x82, s:4, e:8
		uint8_t  lgCtOn              :4;     // 0x83, s:0, e:4
		uint8_t  lgCtOff             :4;     // 0x83, s:4, e:8
		uint8_t  lgCtValLo;                  // 0x84, s:0, e:0
		uint8_t  lgCtValHi;                  // 0x85, s:0, e:0
		uint8_t  lgOnDly;                    // 0x86, s:0, e:0
		uint8_t  lgOnTime;                   // 0x87, s:0, e:0
		uint8_t  lgOffDly;                   // 0x88, s:0, e:0
		uint8_t  lgOffTime;                  // 0x89, s:0, e:0
		uint8_t  lgActionType        :2;     // 0x8a, s:0, e:2
		uint8_t                      :3;     //
		uint8_t  lgMultiExec         :1;     // 0x8a, s:5, e:6
		uint8_t  lgOffTimeMode       :1;     // 0x8a, s:6, e:7
		uint8_t  lgOnTimeMode        :1;     // 0x8a, s:7, e:8
		uint8_t  lgSwJtOn            :4;     // 0x8b, s:0, e:4
		uint8_t  lgSwJtOff           :4;     // 0x8b, s:4, e:8
		uint8_t  lgSwJtDlyOn         :4;     // 0x8c, s:0, e:4
		uint8_t  lgSwJtDlyOff        :4;     // 0x8c, s:4, e:8
	} lstPeer;
	  
  
  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------ 
  public://----------------------------------------------------------------------------------------------------------------
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
