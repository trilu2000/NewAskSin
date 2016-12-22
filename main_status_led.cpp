//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#include "00_debug-flag.h"
#include "main_status_led.h"
#include "wait_timer.h"



																								
LED::LED(const s_pin_def *ptr_pin_red, const s_pin_def *ptr_pin_grn) {
	pin_red = ptr_pin_red;
	pin_grn = ptr_pin_grn;
	op_pat[0].stat = LED_STAT::NONE;														// we need a clean start
	op_pat[1].stat = LED_STAT::NONE;
}

void LED::init() {
	set_pin_output(pin_red);
	set_pin_output(pin_grn);
}

/**
* @brief Here we are choosing and start the blink pattern. Processing is done via the poll function within 
* this class. Blinkpattern defined in HAL.H and declaration is done in HAL_extern.h
* @param   stat    pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long, nothing
*/
void LED::set(LED_STAT::E stat) {
	if (!ptr_pat) return;																	// check if we have something to do while a blink pattern structure is configured 

	/* if anything is active, we need to check if we are able to overrule or to remember for later use */
	if (op_pat[0].stat) {
		if (op_pat[0].sline.prio == 1) {													// active but can be be overwritten
			memcpy(&op_pat[1], &op_pat[0], sizeof(s_op_pat));								// store the scenario

		} else if (op_pat[0].sline.prio == 2) {												// active, but cannot be overwritten
			return;																			// leave set function
		}
	}

	/* prepare the next blink pattern, by copying the pattern content from progmem in our operational struct and fixing some values */
	memcpy_P(&op_pat[0].sline, &ptr_pat[stat-1], sizeof(s_blink_pattern));
	op_pat[0].stat = stat;
	op_pat[0].slot_cnt = 0;
	op_pat[0].repeat_cnt = 1;

	set_pin_low(pin_red);
	set_pin_low(pin_grn);
	//ledRed(0);																				// new program starts, so switch leds off
	//ledGrn(0);
	timer.set(5);																			// 50 ms all leds off before new sequence starts
	//dbg << "set  op0{ " << op_pat[0].stat << ", " << op_pat[0].slot_cnt << ", " << op_pat[0].repeat_cnt << ", sline{ " << op_pat[0].sline.prio << ", " << op_pat[0].sline.repeat << ", " << op_pat[0].sline.led_red << ", " << op_pat[0].sline.led_grn << ", *pat, },};\n";
}


/**
* @brief Poll function has to be called regulary to process blink patterns. Done by the AS main class
* by default.
*/
void LED::poll(void) {

	if (!op_pat[0].stat) return;															// no active profile
	if (!timer.done()) return;																// active but timer not done

	uint8_t slot_len = _PGM_BYTE(op_pat[0].sline.pat[0]);									// get the length byte for the pattern string

	/* check if we are done with the currently active pattern */
	if (op_pat[0].slot_cnt >= slot_len) {
		if ((!op_pat[0].sline.repeat) || (op_pat[0].sline.repeat > op_pat[0].repeat_cnt)) {	// we are in an endless loop, or not all repeats are done
			op_pat[0].repeat_cnt++;															// start the next round
			op_pat[0].slot_cnt = 0;															// from beginning

		} else if (op_pat[1].stat) {														// done, check for previous action
			memcpy(&op_pat[0], &op_pat[1], sizeof(s_op_pat));								// copy back the last status
			//dbg << "copy back\n";

		} else {																			// nothing to do any more, clean up
			op_pat[0].stat = LED_STAT::NONE;
		}
		return;																				// start again in the next poll call if necassary
	}

	/* adopt the new status, set counter, timer, leds */
	op_pat[0].slot_cnt++;
	timer.set( _PGM_BYTE(op_pat[0].sline.pat[op_pat[0].slot_cnt]) * 10);

	/* on even we switch off the respective led, odd means on */
	if (op_pat[0].slot_cnt % 2) {															// identify odd
		if (op_pat[0].sline.led_red) set_pin_high(pin_red);													// and set led accordingly
		if (op_pat[0].sline.led_grn) set_pin_high(pin_grn);	
		//ledGrn(op_pat[0].sline.led_grn);

	} else {
		set_pin_low(pin_red);
		set_pin_low(pin_grn);
		//ledRed(0);																			// or off
		//ledGrn(0);

	}
	//dbg << "poll op0{ " << op_pat[0].stat << ", " << op_pat[0].slot_cnt << ", " << op_pat[0].repeat_cnt << ", sline{ " << op_pat[0].sline.prio << ", " << op_pat[0].sline.repeat << ", " << op_pat[0].sline.led_red << ", " << op_pat[0].sline.led_grn << ", *pat, },};\n";
}
