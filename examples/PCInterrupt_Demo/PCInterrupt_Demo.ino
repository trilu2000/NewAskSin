/**
*
* @brief This is an example on how to work with the pin change interrupt library
*
* The pin change interrupt library makes it easier to use pin change interrupts while it takes
* care of the hardware, configures pins by itself and has a callback functionality for each registered pin.
* Further on it has a configurable denouncing functionality.
*
*/

/**
* Standard include to use the library
*/
#include <pcint.h>


/**
* Some sample function/class to demonstrate callback functionality
*/
class c_test_class {
public:
	void foo(uint8_t pcint, uint8_t status) {
		dbg << F("test class, PCINT: ") << pcint << ", " << ((status) ? F("raising") : F("falling")) << F(" edge \n");
	}
} test_class;

void test_function(uint8_t pcint, uint8_t status) {
	dbg << F("test function, PCINT: ") << pcint << ", " << ((status) ? F("raising") : F("falling")) << F(" edge \n");
}



void setup() {

	dbg.begin(57600);
	dbg << F("test to demonstrate pc interrupt functionallity...\n");

	/**
	* Sample on registering some PINS within the library
	*
	* @param _PCINT00 is a macro defined in pcint.h which reflects all necessary port and pin register definition
	* @param PIN_HIGH sets the input pin to high, first interrupt will be detected while pin goes low, options are PIN_HIGH/PIN_LOW
	* @param DEBOUNCE is set, if there is the need for debouncing. Debounce time is set within pcint.h, alternativ you can 
	* set DIRECT for a callback without debounce
	* @param DELEGATE (s_pcint_dlgt) here you cann register a class member or a function to get a call back. Important is only, that 
	* this function have to have two parameters, pcint - to get the pin change interrupt number, 
	* status - to receive the trigger for the interrupt (0) falling, (1) raising edge
	*/
	pcinit_register( _PCINT00, PIN_HIGH, DEBOUNCE, s_pcint_dlgt( &test_class, &c_test_class::foo ));
	pcinit_register( _PCINT01, PIN_HIGH, DEBOUNCE, s_pcint_dlgt( &test_class, &c_test_class::foo ));

	pcinit_register( _PCINT08, PIN_HIGH, DIRECT,   s_pcint_dlgt( &test_function ));
	pcinit_register( _PCINT09, PIN_HIGH, DIRECT,   s_pcint_dlgt( &test_function ));
	pcinit_register( _PCINT10, PIN_HIGH, DIRECT,   s_pcint_dlgt( &test_function ));
	pcinit_register( _PCINT11, PIN_HIGH, DEBOUNCE, s_pcint_dlgt( &test_function ));
	pcinit_register( _PCINT12, PIN_HIGH, DEBOUNCE, &test_function );
	pcinit_register( _PCINT13, PIN_HIGH, DEBOUNCE, &test_function );


}

void loop() {

	/**
	* Important for the debouncing functionality
	*/
	pcint_poll();



}


