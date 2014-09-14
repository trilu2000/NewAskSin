//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin config key function --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

//#define CB_DBG
#include "ConfButton.h"
#include "AS.h"

waitTimer btnTmr;																			// button timer functionality

// public:		//---------------------------------------------------------------------------------------------------------
CB::CB() {
} 

void CB::init(AS *ptrMain) {
	#ifdef CB_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("CB.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void CB::config(uint8_t mode, uint8_t pcIntByte, uint8_t pcIntBit) {
	scn = mode;
	pciByte = pcIntByte;
	pciBit = pcIntBit;
}
void CB::poll(void) {
	//if (!scn) return;
	
	// 2 for falling and 3 for rising edge
	uint8_t chkKey = chkPCINT(pciByte, pciBit);												// check input pin
	if      (chkKey == 2) led0_on();														// set led accordingly
	else if (chkKey == 3) led0_off();

}
