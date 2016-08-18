/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin registrar functions -----------------------------------------------------------------------------------------------
* - With support from http://blog.coldflake.com/posts/C++-delegates-on-steroids/
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "Registrar.h"
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------

/**
* @brief Polls simply through the channel module array and calls
*        the delegate funktion to the hmEventCol function with
*        0 parameters, which means - poll the channel module
*
*/
void	RG::poll(void) {

	for (uint8_t i = 0; i <= cnl_max; i++) {											// poll through the module table
		RG::s_modTable *pModTbl = &modTbl[i];											// pointer to the respective line in the module table
		if (pModTbl->isActive) pModTbl->mDlgt(POLL);
	}
}

