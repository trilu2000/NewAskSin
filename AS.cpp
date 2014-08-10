//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define AS_DBG
#include "AS.h"


// public:		//---------------------------------------------------------------------------------------------------------
AS::AS() {
}

void AS::init(void) {
	#ifdef AS_DBG																		// only if cc debug is set
	dbg.begin(57600);																	// dbg setup
	dbg << F("\n....\n");																// ...and some information
	dbg << F("AS.\n");																	// ...and some information
	#endif
	
	ee.init();																			// eeprom init
	cc.init();																			// init the rf module

	rv.HMID = HMID;																		// hand over the pointer to HMID for checking up if a message is for us
	rv.MAID = ee.MAID;																	// hand over the pointer to Master ID for checking if a message comes from Master

	// everything is setuped, enable RF functionality
	_enableGDO0Int;																		// enable interrupt to get a signal while receiving data
}
void AS::poll(void) {

	if (ccGDO0()) {																		// check if something was received
		cc.rcvData(rv.buf);																// copy the data into the receiver module
		if (rv.hasData) {
			rv.decode();																// decode the string
			received();
		}
	}
}
void AS::received(void) {
	#ifdef AS_DBG																		// only if AS debug is set
	if (ee.isBroadCast(rv.reID)) dbg << 'b';
	else if (ee.isPairValid(rv.reID)) dbg << 'm';
	else if (ee.isPeerValid(rv.reID)) dbg << 'p';
	else dbg << '-';
	dbg << F("> ") << pHex(rv.buf,rv.buf[0]+1) << '\n';
	#endif



	//dbg << ee.isBroadCast(rv.reID) << '\n';
}




AS hm;

