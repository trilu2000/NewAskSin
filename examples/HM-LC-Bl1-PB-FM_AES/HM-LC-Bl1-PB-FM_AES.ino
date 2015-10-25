
#define SER_DBG

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																// device configuration file
#include <AS.h>

/**
 * @brief This is the Arduino Setup-Function
 *
 * TODO: maybe we move the main parts to own setup/init function?
 */
void setup() {
	// TODO: Maybe we should enable timer0 and SPI internally?
	power_timer0_enable();
	power_spi_enable();																// enable only needed functions

	// enable only what is really needed
	#ifdef SER_DBG
		dbgStart();																	// serial setup
		dbg << F("Starting sketch for HM-LC-Bl1-FM_AES ...\n\n");
		dbg << F(LIB_VERSION_STRING);
	#endif
	
	hm.init();																		// init the asksin framework

	// TODO: Maybe we should enable global interrupts internally at AS::init?
	sei();																			// enable interrupts

	#ifdef SER_DBG
		dbg << F("HMID:  ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << "\n\n";		// some debug
		dbg << F("HmKey: ") << _HEX(HMKEY, 16) << '\n';
		dbg << F("KeyId: ") << _HEX(hmKeyIndex, 1) << '\n';

		for (uint8_t i = 1; i <= devDef.cnlNbr; i++) {											// check if AES activated for any channel
			if (hm.ee.getRegAddr(i, 1, 0, AS_REG_L1_AES_ACTIVE)) {
				dbg << F("AES active for channel: ") << _HEXB(i) << '\n';
			}
		}
		dbg << '\n';
	#endif

	/*
	 * At this point you can write your own code
	 */
}


/**
 * @brief This is the Arduino Forever-Loop
 */
void loop() {
	hm.poll();																		// poll the asksin main loop

	/*
	 * At this point you can write your own code
	 */
}

/**
 * @brief Init the blind channel modul
 */
void initBlind(uint8_t channel) {
	dbg << "init pwm\n";
}

/**
 * @brief This function was called at every action
 */

void blindUpdateState(uint8_t channel, uint8_t state) {
	#ifdef SER_DBG
		dbg << F("Ch: ") << channel << F(", State: ") << state << '\n';
	#endif
}
