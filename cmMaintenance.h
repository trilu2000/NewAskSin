/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin channel module Maintenance -------------------------------------------------------------------------------------
* - list 0 handling for self created devices
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _cmMaintenance_H
#define _cmMaintenance_H

#include "cmMaster.h"


/**
* @brief Register definitions
* The values are adresses in relation to the start adress defines in cnlTbl
* Register values can found in related Device-XML-File.
*
* Spechial register list 0: 0x0A, 0x0B, 0x0C
* Spechial register list 1: 0x08
*
* Has to be defined within the channel module or externally from the class
*/

class cmMaintenance : public cmMaster {
public:  //----------------------------------------------------------------------------------------------------------------
	
	cmMaintenance(const uint8_t peer_max);													// constructor

	virtual void info_config_change(void);													// list1 on registered channel had changed
	virtual void cm_poll(void);																// poll function, driven by HM loop



};

extern const uint8_t cmMaintenance_ChnlLen;
extern const uint8_t cmMaintenance_ChnlReg[];
extern const uint8_t cmMaintenance_ChnlDef[];
extern uint8_t cmMaintenance_ChnlVal[];


#endif
