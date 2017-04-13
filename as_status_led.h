/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin status led class -----------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/


#ifndef _LD_H
#define _LD_H

#include "HAL.h"
#include "waittimer.h"
#include "as_type_defs.h"


/* definition of blink pattern, first byte indicates the length, followed by a sequence of on, off times; values are multiplied by 10ms */
// DEV_BATTERY                                  LED blinks 1 x long, 2 x short, break (2 repeats)
const uint8_t pat00[] PROGMEM = {  6,  50, 10, 10, 10, 10, 200, };
// PAIR_WAIT, RESET_SLOW, WELCOME               LED blinks slow
const uint8_t pat01[] PROGMEM = {  2,  50, 50, };
// PAIR_ACTIVE, GOT_ACK, GOT_NACK, RESET_FAST   LED blinks fast
const uint8_t pat02[] PROGMEM = {  2,   5, 10, };	
// PAIR_SUCCESS, PAIR_ERROR                     LED on for 2 seconds
const uint8_t pat03[] PROGMEM = {  2, 200, 10, };	
// EEPROM_ERROR    LED 1 x long, 6 x short red
const uint8_t pat04[] PROGMEM = { 14,  50, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, };
// LED_RED_L       LED 1 x long
const uint8_t pat05[] PROGMEM = {  2,  50, 10, }; 

// enum: DEV_BATTERY, PAIR_WAIT, PAIR_SUCCESS, PAIR_ERROR, SEND_MSG, GOT_ACK, GOT_NACK, RESET_SLOW, RESET_FAST, WELCOME, EEPROM_ERROR, LED_RED_L, NONE = 0xFF
const s_blink_pattern pattern_table[] PROGMEM = {
/* prio, repeat, red, green, ptr to pat */
	{ 2, 2, 1, 0, pat00, },	//  0 - DEV_BATTERY
	{ 1, 0, 1, 1, pat01, },	//  1 - PAIR_WAIT
	{ 2, 1, 0, 1, pat03, },	//  2 - PAIR_SUCCESS
	{ 2, 1, 1, 0, pat03, },	//  3 - PAIR_ERROR
	{ 0, 3, 1, 1, pat02, },	//  4 - SEND_MSG
	{ 0, 2, 0, 1, pat02, },	//  5 - GOT_ACK
	{ 0, 2, 1, 0, pat02, },	//  6 - GOT_NACK
	{ 2, 0, 1, 0, pat01, },	//  7 - RESET_SLOW
	{ 2, 0, 1, 0, pat02, },	//  8 - RESET_FAST
	{ 0, 3, 0, 1, pat01, },	//  9 - WELCOME
	{ 0, 1, 1, 0, pat04, },	// 10 - EEPROM_ERROR
	{ 0, 1, 1, 0, pat05, },	// 11 - LED_RED_L
};


class LED {
public:		//---------------------------------------------------------------------------------------------------------

	uint8_t def_red;														// store the led pins
	uint8_t def_grn;
	const s_blink_pattern *pat_table;										// pointer to pattern table

	waittimer timer;														// blink timer functionality

	struct s_operate_pattern {
		LED_STAT::E line;													// pattern selector
		uint8_t slot_cnt;													// counter for positioning inside of blink string
		uint8_t repeat_cnt;													// duration counter
	};
	s_operate_pattern cur_pat;												// array of two, 0 for current, 1 to restore previous
	s_operate_pattern svd_pat;

	LED(uint8_t pin_def_red, uint8_t pin_def_grn, const s_blink_pattern *table = pattern_table);// class constructor for two leds
	LED(uint8_t pin_def_red, const s_blink_pattern *table = pattern_table);// class constructor for one led

	void init(void);														// init function to make hardware ready

	/*
	*  DEV_BATTERY     LED blinks 1 x long, 2 x short, break (2 repeats) - battery low
	*  PAIR_WAIT       LED blinks slowly orange - pairing mode, wait for communication with master
	*  PAIR_ACTIVE     LED blinks fast green - pairing mode, communication is active
	*  PAIR_SUCCESS    LED on for 2s green - pair success
	*  PAIR_ERROR      LED on for 2s red - pair failure
	*  SEND_MSG        LED blinks 3 x fast orange - send message
	*  GOT_ACK         LED blinks 2 x fast green - ACK received
	*  GOT_NACK        LED blinks 2 x fast red - NACK or no answer received
	*  RESET_SLOW      LED blinks slowly red - start of device reset seq (wait for another long keypress, or a short keypress to terminate the sequence)
	*  RESET_FAST      LED blinkt schnell rot - reset to factory defaults
	*  WELCOME         LED blinks 3 x slow green - device ready
	*  EEPROM_ERROR    LED 1 x long, 6 x short red - checksum of eeprom wrong, device reset
	*  LED_RED_L       LED 1 x long - key press
	* https://www.homematic-inside.de/tecbase/homematic/generell/item/fehlercodes-und-ihre-bedeutungen
	*/
	void set(LED_STAT::E activity);											// function to set the blink pattern
	void stop(void);														// stop led 
	void poll(void);														// poll function to process blink pattern

	void led_on(uint8_t &def_led);
	void leds_off(void);
	void leds_output(void);
};

void set_led_timer(uint32_t millis);										// helper function to set the button timer and the stay_awake timer


#endif
