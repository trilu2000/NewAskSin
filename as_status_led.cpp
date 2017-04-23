/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin status led class -----------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"


LED::LED(uint8_t pin_def_red, uint8_t pin_def_grn, const s_blink_pattern *table) {
	def_red = pin_def_red;																	// remember the led pins
	def_grn = pin_def_grn;
	pat_table = table;																		// assign the pattern table
	cur_pat.line = svd_pat.line = LED_STAT::NONE;											// we need a clean start
}

LED::LED(uint8_t pin_def_red, const s_blink_pattern *table) {
	def_red = pin_def_red;																	// remember the led pins
	def_grn = 255;
	pat_table = table;																		// assign the pattern table
	cur_pat.line = svd_pat.line = LED_STAT::NONE;											// we need a clean start
}

void LED::init() {
	leds_output();																			// set leds to output, but checks if the pin is configured
}

/**
* @brief Here we are choosing and start the blink pattern. Processing is done via the poll function within 
* this class. Blinkpattern defined in HAL.H and declaration is done in HAL_extern.h
* @param   stat    pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long, nothing
*/
void LED::set(LED_STAT::E activity) {
	DBG(LD, F("LED:SET - "));																	

	/* if anything is active, we need to check if we are able to overrule and/or to remember for later use */
	uint8_t prio = _PGM_BYTE(pat_table[cur_pat.line].prio);
	if (activity == LED_STAT::NONE) svd_pat.line = LED_STAT::NONE;
	if (cur_pat.line) {
		DBG(CB, F("active, "));

		if (prio == 1) {																	// active but can be be overwritten
			DBG(CB, F("overwrite, "));
			svd_pat = cur_pat;																// remember on the current activity

		} else if (prio == 2) {																// active, but cannot be overwritten
			DBG(CB, F("cannot overwrite\n"));
			return;																			// leave set function
		}
	}

	/* prepare the next blink pattern, by copying the pattern content from progmem in our operational struct and fixing some values */
	cur_pat.line = activity;
	cur_pat.slot_cnt = 0;
	cur_pat.repeat_cnt = 1;

	leds_off();																				// 10 ms all leds off before new sequence starts
	set_led_timer(10);
	DBG(CB, F("stat:"), cur_pat.stat, '\n');
}

void LED::stop(void) {
	DBG(LD, F("LED:STOP\n"));

	cur_pat.line = LED_STAT::NONE;
	set_led_timer(0);
	leds_off();
}

/**
* @brief Poll function has to be called regulary to process blink patterns. Done by the AS main class
* by default.
*/
void LED::poll(void) {

	if (cur_pat.line == LED_STAT::NONE) return;												// no active profile
	if (!timer.done()) return;																// active but timer not done

	/* copy the repective line into memory */
	s_blink_pattern tbl_line;
	memcpy_P(&tbl_line, &pat_table[cur_pat.line], sizeof(s_blink_pattern));

	uint8_t slot_len = _PGM_BYTE(tbl_line.pat[0]);											// get the length byte for the pattern string

	/* check if we are done with the current active pattern */
	if (cur_pat.slot_cnt >= slot_len) {

		if ((!tbl_line.repeat) || (tbl_line.repeat > cur_pat.repeat_cnt)) {					// we are in an endless loop, or not all repeats are done
			cur_pat.repeat_cnt++;															// start the next round
			cur_pat.slot_cnt = 0;															// from beginning

		} else if (svd_pat.line) {															// done, check for previous action
			cur_pat = svd_pat;																// copy back the last status
			//dbg << "copy back\n";

		} else {																			// nothing to do any more, clean up
			cur_pat.line = LED_STAT::NONE;
		}
		
		return;
	}

	/* adopt the new status, set counter, timer, leds */
	cur_pat.slot_cnt++;																		// increase the slot counter
	set_led_timer( _PGM_BYTE(tbl_line.pat[cur_pat.slot_cnt]) * 10 );						// set the led timer

	/* on even we switch off the respective led, odd means on */
	if (cur_pat.slot_cnt % 2) {																// identify odd
		if (tbl_line.led_red) led_on(def_red);												// and set led accordingly
		if (tbl_line.led_grn) led_on(def_grn);

	} else {
		leds_off();																			// all leds off

	}

}

void LED::led_on(uint8_t &def_led) {
	if (def_led != 255) set_pin_high(def_led);
}

void LED::leds_off(void) {
	if (def_red != 255) set_pin_low(def_red);
	if (def_grn != 255) set_pin_low(def_grn);
}

void LED::leds_output(void) {
	if (def_red != 255) set_pin_output(def_red);
	if (def_grn != 255) set_pin_output(def_grn);
}



void set_led_timer(uint32_t millis) {
	led.timer.set(millis);
	pom.stayAwake(millis + 100);
}