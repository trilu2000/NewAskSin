


/*-- timer functions ------------------------------------------------------------------------------------------------------
* as i don't want to depend on the arduino timer while it is not possible to add some time after the arduino was sleeping
* i defined a new timer. to keep it as flexible as possible you can configure the different timers in the arduino by handing
* over the number of the timer you want to utilize. as timer are very vendor related there is the need to have at least 
* timer 0 available for all different hardware.
*/
// https://github.com/zkemble/millis/blob/master/millis/
//extern void init_millis_timer0(int16_t correct_ms = 0);										// initialize the respective timer
//extern void init_millis_timer1(int16_t correct_ms = 0);
//extern void init_millis_timer2(int16_t correct_ms = 0);
//extern void init_millis_timer3(int16_t correct_ms = 0);
//uint32_t get_millis(void);																	// get the current time in millis
//void add_millis(uint32_t ms);																// add some time to the counter, mainly used while wake up from sleeping
//- -----------------------------------------------------------------------------------------------------------------------


/*-- battery measurement functions ----------------------------------------------------------------------------------------
* for arduino there are two options to measure the battery voltage - internal and external. internal can measure only a 
* directly connected battery, external can measure any voltage via a resistor network.
* the resistor network is connected with z1 to ground and the measure pin, z2 is connected to measure pin and VCC
* the external measurement function will enable both pins, measure the voltage and switch back the enable pin to input,
* otherwise the battery gets drained via the resistor network...
* http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
*/
//uint8_t get_internal_voltage(void);															// internal measurement
//uint8_t get_external_voltage(uint8_t pin_enable, uint8_t pin_measure, uint8_t z1, uint8_t z2); // external measurement
//inline uint16_t get_adc_value(uint8_t reg_admux);
//- -----------------------------------------------------------------------------------------------------------------------


/*-- power saving functions ----------------------------------------------------------------------------------------
* As power saving is very hardware and vendor related, we need a common scheme which is working similar on all
* supported hardware - needs to be reworked...
* http://donalmorrissey.blogspot.de/2010/04/sleeping-arduino-part-5-wake-up-via.html
* http://www.mikrocontroller.net/articles/Sleep_Mode#Idle_Mode
*/
//static uint16_t wdtSleep_TIME;																// variable to store the current mode, amount will be added after wakeup to the millis timer
//void startWDG32ms(void);																	// set watchdog to 32 ms
//void startWDG64ms(void);																	// 64 ms
//void startWDG250ms(void);																	// 256 ms
//void startWDG8000ms(void);																	// 8192 ms
//void setSleep(void);																		// set the cpu in sleep mode

//void startWDG();																			// start watchdog timer
//void stopWDG();																				// stop the watchdog timer
//void setSleepMode();																		// set the sleep mode, documentation to follow
//- -----------------------------------------------------------------------------------------------------------------------


//uint16_t freeRam(void);


