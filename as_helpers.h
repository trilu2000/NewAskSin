/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin helper functions -----------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _AS_HELPERS_H
#define _AS_HELPERS_H


//- asksin related helpers ------------------------------------------------------------------------------------------------
uint32_t byteTimeCvt(uint8_t tTime);														// asksin, converts a byte time value to a 32bit milliseconds value
uint32_t intTimeCvt(uint16_t  iTime);														// asksin, converts an int time value to a 32bit milliseconds value

void hm_decode(uint8_t *buf);																// decodes the message
void hm_encode(uint8_t *buf);																// encodes the message
//- -----------------------------------------------------------------------------------------------------------------------


//- byte array related helpers --------------------------------------------------------------------------------------------
uint8_t isEmpty(void *ptr, uint8_t len);													// check if a byte array is empty
uint8_t isEqual(void *p1, void *p2, uint8_t len);											// check if a byte array is equal
//- -----------------------------------------------------------------------------------------------------------------------






//- -----------------------------------------------------------------------------------------------------------------------




#endif
