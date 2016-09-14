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

class cmMaintenance : public cmMaster {
private:  //---------------------------------------------------------------------------------------------------------------

public:  //----------------------------------------------------------------------------------------------------------------

	cmMaintenance(const EE::s_cnlTbl *ptr_cnlTbl, const EE::s_cnlTbl *ptr_peerTbl, const EE::s_peerTbl *ptr_peerDB);// constructor

	virtual void info_config_change(void);													// list1 on registered channel had changed
	//virtual void poll(void);																// poll function, driven by HM loop


};

#endif
