/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin send function --------------------------------------------------------------------------------------------------
* - with some support from martin876 at FHEM forum, AES implementation from Dirk
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _RV_H
#define _RV_H

#include "HAL.h"														// hardware abstraction layer
#include "AS_typedefs.h"


class RV  {
	friend class AS;

public:		//---------------------------------------------------------------------------------------------------------
	/* all used and neccassary variables are in rcv_msg, which is defined external */

	RV() {}																// constructor
	void    poll(void);													// poll function
	void    getIntend(void);											// checks the received string if addresses are known
};

/*
* @brief Struct to hold the buffer for any received string with some flags for further processing
*/
extern s_recv rcv_msg;

extern RV rcv;															// declaration of receive class in AS.cpp

#endif 
