//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _HAL_H
#define _HAL_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/delay.h>
//#include <avr/power.h>
//#include <avr/sleep.h>
//#include <avr/wdt.h>


//- some macros and definitions -------------------------------------------------------------------------------------------
#define _pgmB pgm_read_byte																// short hand for PROGMEM read
#define _pgmW pgm_read_word


//- some macros for debugging ---------------------------------------------------------------------------------------------
#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }
#define pHexB(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)<< char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)
#define pHex(x,y) char(0);for(uint8_t i=0;i<y;i++)dbg<<pHexB(x[i])<<char(i+1<y?32:0);dbg<<char(0)
#define pLine     char(0);for(uint8_t i=0;i<40;i++)dbg<<'-';dbg<<'\n'


//- eeprom functions ------------------------------------------------------------------------------------------------------
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr);
void clearEEPromBlock(uint16_t addr, uint16_t len);



#endif 
