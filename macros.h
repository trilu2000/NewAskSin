//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- collection of macros  -------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _MACROS_H
#define _MACROS_H


//- some macros and definitions -------------------------------------------------------------------------------------------
#define _PGM_BYTE(x) pgm_read_byte(&x)										// short hand for PROGMEM read
#define _PGM_WORD(x) pgm_read_word(&x)

#define _SET_PIN_AS_OUTPUT(DDR,PIN)   ((DDR)  |=  _BV(PIN))					// pin functions
#define _SET_PIN_AS_INPUT(DDR,PIN)    ((DDR)  &= ~_BV(PIN))
#define _SET_PIN_HIGH(PORT,PIN)       ((PORT) |=  _BV(PIN))
#define _SET_PIN_LOW(PORT,PIN)        ((PORT) &= ~_BV(PIN))
#define _SET_PIN_TOOGLE(PORT,PIN)     ((PORT) ^=  _BV(PIN))
#define _GET_PIN_STATUS(PORT,PIN)     ((PORT) &   _BV(PIN))
//- -----------------------------------------------------------------------------------------------------------------------

#define _PORT_(port) PORT ## port
#define _DDR_(port)  DDR  ## port
#define _PIN_(port)  PIN  ## port

#define _PORT(port) _PORT_(port)
#define _DDR(port)  _DDR_(port)
#define _PIN(port)  _PIN_(port)


//- some macros for debugging ---------------------------------------------------------------------------------------------
#define dbg Serial

#define _HI_HEX_BITS(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)
#define _LO_HEX_BITS(x)  char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)

//- -----------------------------------------------------------------------------------------------------------------------


#endif
