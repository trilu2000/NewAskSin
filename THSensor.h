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
	void     (*fInit)(void);																// pointer to init function in main sketch
	void     (*fMeas)(void);																// pointer to measurement function in main sketch
	uint8_t  *ptrVal;																		// pointer to byte which holds the measured value

	uint8_t  mMode   :1;																	// 0 timer based, 1 level of changed based transmition
	uint8_t  mLevelChange;																	// value change 
	uint32_t mSendDelay;																	// delay for transmition or minimum delay while value changed
	
	uint8_t  sState  :1;																	// indicates if we are in measuring or transmition state
	uint8_t  msgCnt;																		// message counter of sensor module
	uint8_t  sensVal[2];																	// sensor value, byte 1 is message counter, byte 2 is sensor value
	
	void     config(void Init(), void Measure(), uint8_t *Val);								// configure the sensor module from outside
	void     timing(uint8_t mode, uint32_t sendDelay, uint8_t levelChange);					// mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change

	void     sensPoll(void);																// polling function for tasks done on a regular manner
	uint32_t calcSendSlot(void);															// calculate next send slot based on HMID
	
  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
	uint8_t  modStat;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t  modDUL;																		// module down up low battery byte
	uint8_t  regCnl;																		// holds the channel for the module

	AS      *hm;																			// pointer to HM class instance

	void    configCngEvent(void);															// list1 on registered channel had changed
	void    pairSetEvent(uint8_t *data, uint8_t len);										// pair message to specific channel, handover information for value, ramp time and so on
	void    pairStatusReq(void);															// event on status request
	void    peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);							// peer message was received on the registered channel, handover the message bytes and length

	void    poll(void);																		// poll function, driven by HM loop

  //- predefined, no reason to touch --------------------------------------------------------------------------------------
	void    regInHM(uint8_t cnl, uint8_t lst, AS *instPtr);									// register this module in HM on the specific channel
	void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
	void    peerAddEvent(uint8_t *data, uint8_t len);										// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
};


#endif
