//- debug functionallity --------------------------------------------------------------------------------------------------
#include "00_debug-flag.h"


//- load library's --------------------------------------------------------------------------------------------------------
#include <AS.h>																					// ask sin framework
#include "register.h"																			// configuration sheet


//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	EIMSK = 0;																					// disable external interrupts
	ADCSRA = 0;																					// ADC off
	power_all_disable();																		// and everything else

	//DDRB = DDRC = DDRD = 0x00;																// everything as input
	//PORTB = PORTC = PORTD = 0x00;																// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																			// enable only needed functions

	DBG_START(SER, F("HM_PB_6_WM55\n"));
	DBG(SER, F(LIB_VERSION_STRING));


	// - AskSin related ---------------------------------------
	hm.init();																					// init the asksin framework
	DBG(SER, F("HMID: "), _HEX(dev_ident.HMID, 3), F(", MAID: "), _HEX(dev_operate.MAID, 3), F(", CNL: "), cnl_max, F("\n\n"));	// some debug

	// - user related -----------------------------------------
	sei();																						// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																					// poll the homematic main loop

	// - user related -----------------------------------------

}


//- user functions --------------------------------------------------------------------------------------------------------
void cmRemote::initRemote(uint8_t channel) {
	// setting the essentials for the respective remote channel if necassary
	DBG(SER, F("initRemote: "), channel, '\n');

}




//- predefined functions --------------------------------------------------------------------------------------------------
#ifdef SER_DBG
/*
* @brief Serial debug function to enter byte strings in the serial console.
*        They are forwarded to the send/receive function and processed like
*		 the cc1101 buffer
*/
void serialEvent() {

	static uint8_t i = 0;																		// it is a high byte next time
	while (Serial.available()) {

		uint8_t inChar = (uint8_t)Serial.read();												// read a byte

		if (inChar == 'x') {
			dumpEEprom();
			Serial.flush();
			i = 0;
			return;
		}
		else if (inChar == 's') {
			DBG(SER, F("con: "), _HEX(snd_msg.buf, snd_msg.buf[0] + 1), '\n');
			snd_msg.temp_max_retr = 1;
			snd_msg.active = MSG_ACTIVE::DEBUG;
			i = 0;
			return;
		}

		if ((inChar>96) && (inChar<103)) inChar -= 87;											// a - f
		else if ((inChar>64) && (inChar<71))  inChar -= 55;										// A - F
		else if ((inChar>47) && (inChar<58))  inChar -= 48;										// 0 - 9
		else continue;

		if (i % 2 == 0) snd_msg.buf[i / 2] = inChar << 4;											// high byte
		else snd_msg.buf[i / 2] |= inChar;															// low byte

		i++;
	}
}

void dumpEEprom() {
	uint16_t pAddr;

	DBG(SER, F("\nEEPROM content\n\n"));
	uint8_t *e = new uint8_t[32];
	getEEPromBlock(0, 32, e);
	DBG(SER, F("Magic:"), _HEX(e, 2), F("("), *(uint16_t*)e, F("), HMID:"), _HEX(e + 2, 3), F(", SERIAL:"), _HEX(e + 5, 10), F("\nKEY_IDX:"), _HEX(e + 15, 1), F(", KEY:"), _HEX(e + 16, 16), F("\n\n"));

	for (uint8_t i = 0; i < cnl_max; i++) {														// stepping through channels

		for (uint8_t j = 0; j < 5; j++) {														// stepping through available lists
			s_list_table *list = ptr_CM[i]->list[j];											// short hand to list table
			s_peer_table *peer = &ptr_CM[i]->peerDB;											// short hand to peer db
			if (!list) continue;																// skip if pointer is empty

			uint8_t *x = new uint8_t[list->len];												// size an array as data buffer
			DBG(SER, F("cnl:"), _HEXB(list->cnl), F(", lst:"), _HEXB(list->lst), F(", sLen:"), _HEXB(list->len), F(", pAddr:"), list->ee_addr, '\n');

			memcpy_P(x, list->reg, list->len);
			DBG(SER, F("register:  "), _HEX(x, list->len), '\n');
			memcpy_P(x, list->def, list->len);
			DBG(SER, F("default:   "), _HEX(x, list->len), '\n');

			if (j == 3 || j == 4) {
				DBG(SER, F("cmModul:\n"));
				for (uint8_t k = 0; k < peer->max; k++) {
					uint8_t *p = peer->get_peer(k);												// process peer
					DBG(SER, F("peer   "), _HEXB(k), F(": "), _HEX(p, 4), F(" ("), peer->ee_addr + (k * 4), F(")\n"));
					pAddr = list->ee_addr + (k * list->len);									// process list
					getEEPromBlock(pAddr, list->len, x);
					DBG(SER, F("eeprom "), _HEXB(k), F(": "), _HEX(x, list->len), F(" ("), pAddr, F(")\n"));
				}

			}
			else {
				DBG(SER, F("cmModul:   "), _HEX(list->val, list->len), '\n');
				getEEPromBlock(list->ee_addr, list->len, x);
				DBG(SER, F("eeprom:    "), _HEX(x, list->len), '\n');

			}
			delete x;
			DBG(SER, '\n');
		}
	}
	delete e;
}



#endif

