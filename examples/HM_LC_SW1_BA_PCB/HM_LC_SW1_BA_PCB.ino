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

	DBG_START( SER, F("HM_LC_SW1_BA_PCB\n") );	
	DBG(SER, F(LIB_VERSION_STRING) );


	// - AskSin related ---------------------------------------
	hm.init();																					// init the asksin framework

	// - user related -----------------------------------------
	DBG(SER, F("HMID: "), _HEX(dev_ident.HMID,3), F(", MAID: "), _HEX(MAID,3), F(", CNL: "), cnl_max, F("\n\n") );	// some debug




	//snd.prep_msg(MSG_REASON::INITIAL, MSG_INTENT::INTERN, MSG_TYPE::ACK, 9, 3);

	//dbg << "by03 " << _HEXB(BY03(MSG_TYPE::CONFIG_PARAM_REQ)) << ", by10 " << _HEXB(BY10(MSG_TYPE::CONFIG_PARAM_REQ)) << ", by11 " << _HEXB(BY11(MSG_TYPE::CONFIG_PARAM_REQ)) << ", mlen " << _HEXB(MLEN(MSG_TYPE::CONFIG_PARAM_REQ)) << '\n';
	//uint8_t *x = pcnlModule[0]->lstC.ptr_to_val(0x0a);
	//dbg << "test:" << _HEX(x, 3) << '\n';
	//x[0] = 0x63;
	//x[1] = 0x19;
	//x[2] = 0x64;
	//dbg << "test1:" << _HEX(pcnlModule[0]->lstC.val, pcnlModule[0]->lstC.len) << '\n';


	//dbg << "test: " << _HEX((uint8_t*)pcnlModule[0]->lstC.val, 5) << '\n';
	//DBG("test:", sizeof(pcnlModule) / sizeof(pcnlModule[0]), '\n'; );
	sei();																						// enable interrupts
}

void loop() {
	// - AskSin related ---------------------------------------
	hm.poll();																					// poll the homematic main loop
	
	// - user related -----------------------------------------
	
}


//- user functions --------------------------------------------------------------------------------------------------------
void cmSwitch::initSwitch(uint8_t channel) {
// setting the relay pin as output, could be done also by pinMode(3, OUTPUT)
	DBG(SER, F("initRly: "), channel, '\n' );
	
	SET_PIN_OUTPUT(PIN_D3);																		// init the relay pins
	SET_PIN_LOW(PIN_D3);																		// set relay pin to ground
}

void cmSwitch::switchSwitch(uint8_t channel, uint8_t status) {
// switching the relay, could be done also by digitalWrite(3,HIGH or LOW)
	DBG(SER, F("switchRly: "), channel, ", ", status, '\n' );

	if (status) SET_PIN_HIGH(PIN_D3);															// check status and set relay pin accordingly
	else SET_PIN_LOW(PIN_D3);
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
			i = 0;
			return;
		} else if (inChar == 's') {
			DBG(SER, F("con: "), _HEX(snd_msg.buf, snd_msg.buf[0]+1), '\n');
			snd_msg.temp_max_retr = 1;
			snd_msg.active = 1;
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

	DBG(SER, F("\nEEPROM content\n\n"));

	for (uint8_t i = 0; i < cnl_max; i++) {
		cmMaster *pCM = pcnlModule[i];															// shorthand to channel module

		printList( &pCM->lstC, &pCM->peer );
		if (!i) continue;
		printList( &pCM->lstP, &pCM->peer);
	}
}

void printList( s_list_table *lstP, s_peer_table *peer ) {
	uint8_t *x = new uint8_t[lstP->len];
	uint16_t pAddr;

	DBG(SER, F("cnl:"), _HEXB(lstP->cnl), F(", lst:"), _HEXB(lstP->lst), F(", sLen:"), _HEXB(lstP->len), F(", pAddr:"), lstP->ee_addr, '\n');

	memcpy_P(x, lstP->reg , lstP->len );
	DBG(SER, F("register:  "), _HEX(x, lstP->len), '\n');

	memcpy_P(x, lstP->def, lstP->len );
	DBG(SER, F("default:   "), _HEX(x, lstP->len), '\n');

	DBG(SER, F("cmModul:   "), _HEX(lstP->val, lstP->len), '\n');

	if ((lstP->lst == 3) || (lstP->lst == 4)) {
		DBG(SER, '\n');

		for (uint8_t i = 0; i < peer->max; i++) {

			// process peer
			uint8_t *p = peer->get_peer(i);
			DBG(SER, F("peer   "), _HEXB(i), F(": "), _HEX(p, 4), F(" ("), peer->ee_addr+(i*4), F(")\n"));

			// process list
			pAddr = lstP->ee_addr + (i * lstP->len);
			getEEPromBlock(pAddr, lstP->len, x);
			DBG(SER, F("eeprom "), _HEXB(i), F(": "), _HEX(x, lstP->len), F(" ("), pAddr, F(")\n"));
		}

	} else {
		getEEPromBlock(lstP->ee_addr, lstP->len, x);
		DBG(SER, F("eeprom:    "), _HEX(x, lstP->len), '\n');

	}
	DBG(SER, '\n');
}


#endif

