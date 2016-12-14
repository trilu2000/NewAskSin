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


// - scenario 00 ----------------------------
// no config button used
//
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

/**
 * @short Class for the handling the configuration key of the device
 * Right now there are three possible scenarios which could be configured via the mode flag.
 *
 * - scenario 00 ----------------------------
 * no config button used
 *
 * - scenario 01 ----------------------------
 * short press         = pair
 * long press          = nothing
 * double long         = nothing
 * double long release = reset
 *
 * - scenario 02 ----------------------------
 * short press         = toggle channel one
 * long press          = pair
 * double long         = nothing
 * double long release = reset
 *
 * Interface to the hardware is done via the register byte address of the port
 * and hand over of the respective bit number within this byte
 */
class CB  {
  public:		//---------------------------------------------------------------------------------------------------------

	uint8_t scn;															// scenario indicator
	uint8_t btn;															// result of chkPCINT

	uint8_t armFlg;															// armed flag, will be set on first touch

	uint8_t lstLng;															// remember a long press
	uint8_t lngRpt;															// remember last key press to detect double or long
	uint8_t dblLng;															// check for a double long

	uint8_t lstSht;															// remember that last key was a short

	CB();																	// constructor 
	void config(uint8_t mode);												// config of button mode
	void poll(void);														// poll to process status of button
	inline void outSignal(uint8_t mode);									// function to drive the output signal
};

extern CB btn;																// declaration done in AS.cpp

#endif 
