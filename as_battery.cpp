/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin battery measurement class --------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "as_battery.h"
#include "HAL.h" 



//public:  //--------------------------------------------------------------------------------------------------------------
void BAT::set(uint32_t check_interval, uint8_t tenth_volt) {
	interval = check_interval;
	default_value = tenth_volt;
}

void BAT::poll(void) {
	if (!interval) return;																	// no interval value, nothing to check
	if (!timer.done() ) return;																// timer still running
	do_measure();																			// function is virtual and will be overwritten by the initial setup
	timer.set(interval);																	// set next check time
}

/* @brief - returns the battery status
*  returns 1 if battery is low, otherwise it returns 0 
*/
uint8_t BAT::get_status(void) {
	return (measure_value < default_value) ? 1 : 0;
}


//public:  //--------------------------------------------------------------------------------------------------------------
NO_BAT::NO_BAT() {
	interval = default_value = measure_value = 0;
}


//public:  //--------------------------------------------------------------------------------------------------------------
INT_BAT::INT_BAT(uint32_t check_interval, uint8_t tenth_volt) {
	interval = check_interval;
	default_value = tenth_volt;
}

//protected:  //-----------------------------------------------------------------------------------------------------------
	void INT_BAT::do_measure() {
	measure_value = get_internal_voltage();
}


//public:  //--------------------------------------------------------------------------------------------------------------
EXT_BAT::EXT_BAT(uint32_t check_interval, uint8_t tenth_volt, const s_pin_def *ptr_pin_enable, const s_pin_def *ptr_pin_measure, uint8_t z1, uint8_t z2) {
	interval = check_interval;
	default_value = tenth_volt;

	ptr_enable = ptr_pin_enable;
	ptr_measure = ptr_pin_measure;
	res1 = z1;
	res2 = z2;
}

//protected:  //-----------------------------------------------------------------------------------------------------------
void EXT_BAT::do_measure() {
	measure_value = get_external_voltage(ptr_enable, ptr_measure, res1, res2);
}
