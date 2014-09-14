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
void CB::config(uint8_t mode, uint8_t pcIntByte, uint8_t pcIntBit) {
	scn = mode;
	pciByte = pcIntByte;
	pciBit = pcIntBit;
}

// private:		//---------------------------------------------------------------------------------------------------------
CB::CB() {
}
void CB::init(AS *ptrMain) {
	#ifdef CB_DBG																			// only if ee debug is set
	dbgStart();																				// serial setup
	dbg << F("CB.\n");																		// ...and some information
	#endif

	pHM = ptrMain;
}
void CB::poll(void) {
	#define detectLong      3000
	#define timeoutDouble   1000
	
	if (!scn) return;
	
	// 0 for button is pressed, 1 for released, 2 for falling and 3 for rising edge
	btn = chkPCINT(pciByte, pciBit);														// check input pin

	// check button status
	switch (btn) {
		case 0:		// button is still pressed
		if (btnTmr.done() )	{															// timed out, seems to be a long
			btnTmr.set(detectLong);														// set timer to detect next long
			if (lstLng) keyLongRepeat();												// last key state was a long, now it is a double
			else keyLongSingle();														// first time detect a long
		}
		break;

		case 1:		// button is released, check if the double flags timed out
		if (btnTmr.done() ) {															// check for double timed out
			rptFlg = 0;																	// clear the repeat flag
			lstLng = 0;
		}
		break;

		case 2:		// button was just pressed
		btnTmr.set(detectLong);															// set timer to detect a long
		break;

		case 3:		// button was just released, was a long while timed out, or a short while timer is running
		if      ((lstLng) && (rptFlg)) keyLongRelease();								// check for long double
		else if (rptFlg)  keyShortDouble();												// check for short double
		else if (!lstLng) keyShortSingle();												// otherwise it was a short single
		
		btnTmr.set(timeoutDouble);														// set timer to clear the repeated flags
		break;
	}
}
void CB::keyShortSingle(void) {
	rptFlg = 1;																				// remember last key press

	#ifdef CB_DBG																			// only if ee debug is set
	dbg << F("keyShortSingle\n");															// ...and some information
	#endif

	if (scn == 1) pHM->sendDEVICE_INFO();													// send pairing string
	if ((scn == 2) && (modTbl[0].cnl)) modTbl[0].mDlgt(0,1,0,NULL,0);						// send toggle to user module registered on channel 1
}
void CB::keyShortDouble(void) {
	rptFlg = 0;																				// clear last key press

	#ifdef CB_DBG																			// only if ee debug is set
	dbg << F("keyShortDouble\n");															// ...and some information
	#endif
}
void CB::keyLongSingle(void) {
	lstLng = 1;																				// remember long key press
	
	#ifdef CB_DBG																			// only if ee debug is set
	dbg << F("keyLongSingle\n");															// ...and some information
	#endif

	if (scn == 2) pHM->sendDEVICE_INFO();													// send pairing string
}
void CB::keyLongRepeat(void) {
	rptFlg = 1;																				// remember last key press

	#ifdef CB_DBG																			// only if ee debug is set
	dbg << F("keyLongRepeat\n");															// ...and some information
	#endif
}
void CB::keyLongRelease(void) {
	rptFlg = 0;																				// clear the repeat flag
	lstLng = 0;																				// remember last key press
	
	// 0x18 localResDis available, take care of it
	uint8_t localResDis = pHM->ee.getRegAddr(0,0,0,0x18);									// get register address
	
	#ifdef CB_DBG																			// only if ee debug is set
	dbg << F("keyLongRelease\n");															// ...and some information
	#endif

	if (localResDis) return;
	pHM->ee.clearPeers();
	pHM->ee.clearRegs();
	pHM->ee.getMasterID();
}
