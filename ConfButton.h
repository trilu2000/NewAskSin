//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin config key function --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------


#ifndef _CB_H
#define _CB_H

#include "HAL.h"

// - scenario 01 ----------------------------
// short press         = pair
// long press          = nothing
// double long         = nothing
// double long release = reset
//
// - scenario 02 ----------------------------
// short press         = toggle channel one
// long press          = pair
// double long         = nothing
// double long release = reset
//
// if 0x18 localResDis available, take care of it and enable or disable double long


class CB {
  public:		//---------------------------------------------------------------------------------------------------------
  protected:	//---------------------------------------------------------------------------------------------------------
	class AS *pHM;							// pointer to main class for function calls

	uint8_t scn     :3;						// scenario indicator
	uint8_t btn     :3;						// result of chkPCINT
	uint8_t rptFlg  :1;						// remember last key press to detect double or long
	uint8_t lstLng  :1;						// remember a long press
	uint8_t pciByte;						// PCI byte, indicate interrupt channel 
	uint8_t pciBit;							// PCI bit indicate bit number in PCI channel byte
	
  private:		//---------------------------------------------------------------------------------------------------------

  public:		//---------------------------------------------------------------------------------------------------------
	CB();
	void init(AS *ptrMain);
	void config(uint8_t mode, uint8_t pcIntByte, uint8_t pcIntBit);
	void poll(void);
	void keyShortSingle(void);
	void keyShortDouble(void);
	void keyLongSingle(void);
	void keyLongRepeat(void);
	void keyLongRelease(void);
			
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------

};

#endif 
