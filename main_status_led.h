//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------



#ifndef _LD_H
#define _LD_H

#include "HAL.h"
#include "wait_timer.h"

/**
* @brief LED patterns are defined in HAL.h and HAL_extern.h while we use different patterns depending on
* the LED's connected to the hardware. LED's are defined in hardware.h in user sketch area...
*
* DEV_BATTERY     LED blinks 1 x long, 2 x short, break (2 repeats) battery low
*
* PAIR_WAIT       LED blinks slowly orange	- pairing mode, wait for communication with master
* PAIR_ACTIVE     LED blinks fast green - pairing mode, communication is active
* PAIR_SUCCESS    LED on for 2s green - pair success
* PAIR_ERROR      LED on for 2s red - pair failure
*
* SEND_MSG        LED blinks 3 x fast orange - send message
* GOT_ACK         LED blinks 2 x fast green - ACK received
* GOT_NACK        LED blinks 2 x fast red - NACK or no answer received
*
* RESET_SLOW      LED blinks slowly red - start of device reset seq (wait for another long keypress, or a short keypress to terminate the sequence).
* RESET_FAST      LED blinkt schnell rot - reset to factory defaults
*
* WELCOME         LED blinks 3 x slow green - device ready
* EEPROM_ERROR    LED 1 x long, 6 x short red - checksum of eeprom wrong, device reset
*
* https://www.homematic-inside.de/tecbase/homematic/generell/item/fehlercodes-und-ihre-bedeutungen
*/
namespace LED_STAT {
	enum E : uint8_t { NONE = 0, DEV_BATTERY, PAIR_WAIT, PAIR_SUCCESS, PAIR_ERROR, SEND_MSG, GOT_ACK, GOT_NACK, RESET_SLOW, RESET_FAST, WELCOME, EEPROM_ERROR,};
};

struct s_blink_pattern {					// struct for defining the blink pattern
	uint8_t prio    : 2;					// 0 can be overwritten, 1 overwritten but restored, 2 cannot be overwritten
	uint8_t repeat  : 4;					// how often the pattern has to be repeated, 0 for endless
	uint8_t led_red : 1;					// red
	uint8_t led_grn : 1;					// green, if you like orange, set led_red and led_grn at the same time
	const uint8_t *pat;						// the pattern it self, pattern starts always with the on time, followed by off time.
};

/* definition of blink pattern, first byte indicates the length, followed by a sequence of on, off times; values are multiplied by 10ms */
const uint8_t pat00[] PROGMEM =   {  6,  50, 10, 10, 10, 10, 200, };// DEV_BATTERY     LED blinks 1 x long, 2 x short, break (2 repeats) - battery low
const uint8_t pat01[] PROGMEM =   {  2,  50, 50, };					// PAIR_WAIT       LED blinks slowly orange - pairing mode, wait for communication with master
const uint8_t pat02[] PROGMEM =   {  2,  10, 10, };					// PAIR_ACTIVE     LED blinks fast green - pairing mode, communication is active
const uint8_t pat03[] PROGMEM =   {  2, 200, 10, };					// PAIR_SUCCESS    LED on for 2s green - pair success
//const uint8_t pat04[] PROGMEM = {  2, 200, 10, };					// PAIR_ERROR      LED on for 2s red - pair failure
//const uint8_t pat05[] PROGMEM = {  2,  10, 10, };					// SEND_MSG        LED blinks 3 x fast orange - send message
//const uint8_t pat06[] PROGMEM = {  2,  10, 10, };					// GOT_ACK         LED blinks 2 x fast green - ACK received
//const uint8_t pat07[] PROGMEM = {  2,  10, 10, };					// GOT_NACK        LED blinks 2 x fast red - NACK or no answer received
//const uint8_t pat08[] PROGMEM = {  2,  50, 50, };					// RESET_SLOW      LED blinks slowly red - start of device reset seq (wait for another long keypress, or a short keypress to terminate the sequence)
//const uint8_t pat09[] PROGMEM = {  2,  10, 10, };					// RESET_FAST      LED blinkt schnell rot - reset to factory defaults
//const uint8_t pat10[] PROGMEM = {  2,  50, 50, };					// WELCOME         LED blinks 3 x slow green - device ready
const uint8_t pat11[] PROGMEM =   { 14,  50, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, };	// EEPROM_ERROR    LED 1 x long, 6 x short red - checksum of eeprom wrong, device reset


const struct s_blink_pattern ptr_pat[] PROGMEM = {
/* prio, repeat, red, green, ptr to pat */
	{ 2, 2, 1, 0, pat00, },	//  1 - DEV_BATTERY
	{ 1, 0, 1, 1, pat01, },	//  2 - PAIR_WAIT
//	{ 0, 0, 0, 1, pat02, },	//      PAIR_ACTIVE, same as SEND_MSG
	{ 2, 1, 0, 1, pat03, },	//  3 - PAIR_SUCCESS
	{ 2, 1, 1, 0, pat03, },	//  4 - PAIR_ERROR
	{ 0, 3, 1, 1, pat02, },	//  5 - SEND_MSG
	{ 0, 2, 0, 1, pat02, },	//  6 - GOT_ACK
	{ 0, 2, 1, 0, pat02, },	//  7 - GOT_NACK
	{ 2, 0, 1, 0, pat01, },	//  8 - RESET_SLOW
	{ 2, 0, 1, 0, pat02, },	//  9 - RESET_FAST
	{ 0, 3, 0, 1, pat01, },	// 10 - WELCOME
	{ 0, 1, 1, 0, pat11, },	// 11 - EEPROM_ERROR
};


class LED {
public:		//---------------------------------------------------------------------------------------------------------

	const s_pin_def *pin_red;												// pointer to pin definition
	const s_pin_def *pin_grn;

	waitTimer timer;														// blink timer functionality

	struct s_op_pat {
		LED_STAT::E stat;													// pattern selector
		uint8_t slot_cnt;													// counter for positioning inside of blink string
		uint8_t repeat_cnt;													// duration counter
		s_blink_pattern sline;												// space to copy in the respective pattern line
	};
	s_op_pat op_pat[2];														// array of two, 0 for current, 1 to restore previous

	LED(const s_pin_def *ptr_pin_red, const s_pin_def *ptr_pin_grn);		// class constructor
	void init(void);														// init function to make hardware ready

	void set(LED_STAT::E stat);												// function to set the blink pattern
	void poll(void);														// poll function to process blink pattern
};


#endif
