#include <Arduino.h>


#define SER_DBG
#define USE_SERIAL

//- load library's --------------------------------------------------------------------------------------------------------
#include "register.h"																// device configuration file
#include <AS.h>
#include "helpers.h"

/**
 * @brief This is the Arduino Setup-Function
 *
 * TODO: maybe we move the main parts to own setup/init function?
 */
void setup() {
	// TODO: Maybe we should enable timer0 and SPI internally?
	power_timer0_enable();
	power_spi_enable();																// enable only needed functions

	#if defined(USE_SERIAL)
		Serial.begin(57600);														// starting serial messages
	#else
		Serial.end();
	#endif

	// enable only what is really needed
	#ifdef SER_DBG
		dbgStart();																	// serial setup
		dbg << F("Starting sketch for HM-ES-TX-WM_AES ...\n\n");
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
		pinMode(1, INPUT);
		digitalWrite(1, HIGH);




	#if defined(USE_SERIAL)
		// show help screen and config
		showHelp();																	// shows help screen on serial console
		showSettings();																// show device settings

		Serial << F("version 025") << '\n';															// show device settings
	#endif

	/*
	 * At this point you can write your own code
	 */


}

//- serial communication --------------------------------------------------------------------------------------------------
const char helptext1[] PROGMEM = {												// help text for serial console
	"\n"
	"Available commands:" "\n"
	"  p                - start pairing with master" "\n"
	"  b[0]  b[n]  s    - send a string, b[0] is length (50 bytes max)" "\n"
	"\n"
	"  i[0]. i[1]. e    - show eeprom content, i[0]. start address, i[1]. length" "\n"
	"  i[0]. b[1]  f    - write content to eeprom, i[0]. address, i[1] byte" "\n"
	"  c                - clear eeprom complete, write 0 from start to end" "\n"
	"\n"
	"  b[c]  b[l]  b    - send button event, b[c] channel, b[l] short 0 or long 1" "\n"
	"  a                - stay awake for TRX module (valid if power mode = 2)" "\n"
	"  t                - gives an overview of the device configuration" "\n"
	"\n"
	"  $nn for HEX input (e.g. $AB,$AC ); b[] = byte, i[]. = integer " "\n"
};
#if defined(USE_SERIAL)
const InputParser::Commands cmdTab[] PROGMEM = {
	{ 'h', 0, showHelp },
//	{ 'p', 0, sendPairing },
//	{ 's', 1, sendPowerEventWithInput },
	{ 'e', 0, showEEprom },
	{ 'f', 2, writeEEprom },
	{ 'c', 0, clearEEprom },
	/*	//{ 't', 0, testConfig },

	/*{ 'b', 1, buttonSend },
	{ 'a', 0, stayAwake },

	{ 'r', 0, resetDevice },*/
	{ 0 }
};
InputParser parser (50, (InputParser::Commands*) cmdTab);
#endif

/**
 * @brief This is the Arduino Forever-Loop
 */
void loop() {
	hm.poll();																		// poll the asksin main loop

	/*
	 * At this point you can write your own code
	 */

	#if defined(USE_SERIAL)
	parser.poll();																// handle serial input from console
	#endif
}


#if defined(USE_SERIAL)

void sendPowerEventWithInput(){
	uint8_t buf[6] = { 0, 0, 0, 0, 0, 0 };
	memcpy(buf, parser.buffer, 6);
	const char* v;
	parser >> v;
	int i;
	//Serial << F("s: ") << pHexB(buf, 6) << '\n';
	for (int i = 0; i < 6; i++) {
		Serial.print(buf[i], BIN);
		Serial.print(" - ");
	}
	Serial.print("\n");
	Serial.print(v);
	hm.sendINFO_POWER_EVENT(buf);
}


void sendPairing() {															// send the first pairing request
	//hm.startPairing();
}

void showEEprom() {
	uint16_t start, len;
	uint8_t buf[32];

	parser >> start >> len;
	if (len == 0) len = E2END - start;

	Serial << F("EEPROM listing, start: ") << start << F(", len: ") << len << '\n';

	for (uint16_t i = start; i < len; i+=32) {
		eeprom_read_block(buf,(void*)i,32);
		Serial  << F("   ") << _HEX(buf,32) << '\n';
	}
}

void writeEEprom() {
	uint16_t addr;
	uint8_t data;

	for (uint8_t i = 0; i < parser.count(); i+=3) {
		parser >> addr >> data;
		eeprom_write_byte((uint8_t*)addr,data);
		//Serial << F("Write EEprom, Address: ") << pHex(addr>>8) << pHex(addr&0xFF) << F(", Data: ") << pHex(data) << '\n';
	}
}
void clearEEprom() {															// clear settings
	Serial << F("Clear EEprom, size: ") << E2END+1 << F(" bytes") << '\n';
	for (uint16_t i = 0; i <= E2END; i++) {
		eeprom_write_byte((uint8_t*)i,0);
	}
	Serial << F("done") << '\n';
}

void showHelp() {																// display help on serial console
	//showPGMText(helptext1);
}
void showSettings() {															// shows device settings on serial console
	//hm.printSettings();															// print settings of own HM device
	//Serial << F("FreeMem: ") << freeMemory() << F(" byte's\n");					// displays the free memory
}

void testConfig() {																// shows the complete configuration of slice table and peer database
	//hm.printConfig();															// prints register and peer config
}

#endif
/**
 * @brief Init the blind channel modul
 */
void initPowerSens(uint8_t channel) {
	dbg << "init pwm\n";
}
