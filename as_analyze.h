/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin helper to explain messages  ------------------------------------------------------------------------------------
*
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "00_debug-flag.h"
#include "AS_type_defs.h"
#include "HAL.h"

#ifndef _EXMSG_H
#define _EXMSG_H


extern s_rcv_msg rcv_msg;
extern s_snd_msg snd_msg;


void explain_msg(void);
void serialEvent(void);
void dumpEEprom(void);


void explain_msg(void) {
	uint8_t *by03 = &rcv_msg.mBody.MSG_TYP;
	uint8_t *by10 = &rcv_msg.mBody.BY10;
	uint8_t *by11 = &rcv_msg.mBody.BY11;


	if (*by03 == BY03(MSG_TYPE::DEVICE_INFO)) {

	} else if (*by03 == BY03(MSG_TYPE::CONFIG_REQ)) {

		if        (*by11 == BY11(MSG_TYPE::CONFIG_PEER_ADD)) {

		} else if (*by11 == BY11(MSG_TYPE::CONFIG_PEER_REMOVE)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_PEER_LIST_REQ)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_PARAM_REQ)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_START)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_END)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX1)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_WRITE_INDEX2)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_SERIAL_REQ)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_PAIR_SERIAL)) {
		
		} else if (*by11 == BY11(MSG_TYPE::CONFIG_STATUS_REQUEST)) {
		
		} else {

		}

	} else if (*by03 == BY03(MSG_TYPE::ACK_MSG)) {

	} else if (*by03 == BY03(MSG_TYPE::AES_REPLY)) {

	} else if (*by03 == BY03(MSG_TYPE::SEND_AES)) {

	} else if (*by03 == BY03(MSG_TYPE::REPLY_MSG)) {

	} else if (*by03 == BY03(MSG_TYPE::INSTRUCTION_MSG)) {

		if        (*by10 == BY10(MSG_TYPE::INSTRUCTION_RESET)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_ADAPTION_DRIVE_SET)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_ENTER_BOOTLOADER2)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_OFF)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_INHIBIT_ON)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_SET)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_STOP_CHANGE)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_LED)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_LED_ALL)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_LEVEL)) {
		
		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_SLEEPMODE)) {

		} else if (*by10 == BY10(MSG_TYPE::INSTRUCTION_SET_TEMP)) {
		
		} else {

		}

	} else if (*by03 == BY03(MSG_TYPE::HAVE_DATA)) {

	} else if (*by03 == BY03(MSG_TYPE::SWITCH)) {

	} else if (*by03 == BY03(MSG_TYPE::TIMESTAMP)) {

	} else if (*by03 == BY03(MSG_TYPE::REMOTE)) {

	} else if (*by03 == BY03(MSG_TYPE::SENSOR_EVENT)) {

	} else if (*by03 == BY03(MSG_TYPE::SWITCH_LEVEL)) {

	} else if (*by03 == BY03(MSG_TYPE::SENSOR_DATA)) {

	} else if (*by03 == BY03(MSG_TYPE::GAS_EVENT)) {

	} else if (*by03 == BY03(MSG_TYPE::CLIMATE_EVENT)) {

	} else if (*by03 == BY03(MSG_TYPE::SET_TEAM_TEMP)) {

	} else if (*by03 == BY03(MSG_TYPE::THERMAL_CONTROL)) {

	} else if (*by03 == BY03(MSG_TYPE::POWER_EVENT_CYCLE)) {

	} else if (*by03 == BY03(MSG_TYPE::POWER_EVENT)) {

	} else if (*by03 == BY03(MSG_TYPE::WEATHER_EVENT)) {


	} else {
		//dbg << F("AS:message not known - please report: ") << _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1) << '\n';

	}

	DBG(RV, (char)rcv_msg.intend, F("> "), _HEX(rcv_msg.buf, rcv_msg.buf[0] + 1), ' ', _TIME, '\n');

}

/*
* @brief Serial debug function to enter byte strings in the serial console.
*        They are forwarded to the send/receive function and processed like
*		 the cc1101 buffer
*/
void serialEvent(void) {
#ifdef SER_DBG
	static uint8_t i = 0;																	// it is a high byte next time
	while (Serial.available()) {

		uint8_t inChar = (uint8_t)Serial.read();											// read a byte

		if (inChar == 'x') {
			dumpEEprom();
			Serial.flush();
			i = 0;
			return;
		} else if (inChar == 's') {
			DBG(SER, F("con: "), _HEX(snd_msg.buf, snd_msg.buf[0] + 1), '\n');
			snd_msg.temp_max_retr = 1;
			snd_msg.active = MSG_ACTIVE::DEBUG;
			i = 0;
			return;
		}

		if ((inChar>96) && (inChar<103)) inChar -= 87;										// a - f
		else if ((inChar>64) && (inChar<71))  inChar -= 55;									// A - F
		else if ((inChar>47) && (inChar<58))  inChar -= 48;									// 0 - 9
		else continue;

		if (i % 2 == 0) snd_msg.buf[i / 2] = inChar << 4;									// high byte
		else snd_msg.buf[i / 2] |= inChar;													// low byte

		i++;
	}
#endif
}

void dumpEEprom(void) {
#ifdef DMP_DBG
	uint16_t pAddr;

	dbg << F("\nEEPROM content\n\n");
	uint8_t *e = new uint8_t[32];
	get_eeprom(0, 32, e);
	dbg << F("Magic:") << _HEX(e, 2) << F("(") << *(uint16_t*)e << F("), HMID:") << _HEX(e + 2, 3) << F(", SERIAL:") << _HEX(e + 5, 10) << F("\nKEY_IDX:") << _HEX(e + 15, 1) << F(", KEY:") << _HEX(e + 16, 16) << F("\n\n");

	for (uint8_t i = 0; i < cnl_max; i++) {													// stepping through channels

		for (uint8_t j = 0; j < 5; j++) {													// stepping through available lists
			s_list_table *list = ptr_CM[i]->list[j];										// short hand to list table
			s_peer_table *peer = &ptr_CM[i]->peerDB;										// short hand to peer db
			if (!list) continue;															// skip if pointer is empty

			uint8_t *x = new uint8_t[list->len];											// size an array as data buffer
			dbg << F("cnl:") << _HEX(list->cnl) << F(", lst:") << _HEX(list->lst) << F(", sLen:") << _HEX(list->len) << F(", pAddr:") << list->ee_addr << '\n';

			memcpy_P(x, list->reg, list->len);
			dbg << F("register:  ") << _HEX(x, list->len) << '\n';
			memcpy_P(x, list->def, list->len);
			dbg << F("default:   ") << _HEX(x, list->len) << '\n';

			if (j == 3 || j == 4) {
				dbg << F("cmModul:\n");
				for (uint8_t k = 0; k < peer->max; k++) {
					uint8_t *p = peer->get_peer(k);											// process peer
					dbg << F("peer   ") << _HEX(k) << F(": ") << _HEX(p, 4) << F(" (") << peer->ee_addr + (k * 4) << F(")\n");
					pAddr = list->ee_addr + (k * list->len);								// process list
					get_eeprom(pAddr, list->len, x);
					dbg << F("eeprom ") << _HEX(k) << F(": ") << _HEX(x, list->len) << F(" (") << pAddr << F(")\n");
				}

			} else {
				dbg << F("cmModul:   ") << _HEX(list->val, list->len) << '\n';
				get_eeprom(list->ee_addr, list->len, x);
				dbg << F("eeprom:    ") << _HEX(x, list->len) << '\n';

			}
			delete x;
			dbg << '\n';
		}
	}
	delete e;
#endif
}




#endif