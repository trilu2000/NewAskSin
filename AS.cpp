//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#define AS_DBG
//#define AS_DBG_EX
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
	uint8_t bIntend = ee.getIntend(rv.reID,rv.toID);

	#ifdef AS_DBG																		// only if AS debug is set
	dbg << (char)bIntend << F("> ") << pHex(rv.buf,rv.len) << '\n';
	#endif

	#ifdef AS_DBG_EX
	#define b_len			rv.buf[0]
	#define b_msgTp			rv.buf[3]
	#define b_by10			rv.buf[10]
	#define b_by11			rv.buf[11]

	dbg << F("   ");																	// save some byte and send 3 blanks once, instead of having it in every if
	
	if        ((b_msgTp == 0x00)) {
		dbg << F("DEVICE_INFO; fw: ") << pHex((rv.buf+10),1) << F(", type: ") << pHex((rv.buf+11),2) << F(", serial: ") << pHex((rv.buf+13),10) << '\n';
		dbg << F("              , class: ") << pHexB(rv.buf[23]) << F(", pCnlA: ") << pHexB(rv.buf[24]) << F(", pCnlB: ") << pHexB(rv.buf[25]) << F(", na: ") << pHexB(rv.buf[26]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x01)) {
		dbg << F("CONFIG_PEER_ADD; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnlA: ") << pHexB(rv.buf[15]) << F(", pCnlB: ") << pHexB(rv.buf[16]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x02)) {
		dbg << F("CONFIG_PEER_REMOVE; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnlA: ") << pHexB(rv.buf[15]) << F(", pCnlB: ") << pHexB(rv.buf[16]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x03)) {
		dbg << F("CONFIG_PEER_LIST_REQ; cnl: ") << pHexB(rv.buf[10]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x04)) {
		dbg << F("CONFIG_PARAM_REQ; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnl: ") << pHexB(rv.buf[15]) << F(", lst: ") << pHexB(rv.buf[16]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x05)) {
		dbg << F("CONFIG_START; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnl: ") << pHexB(rv.buf[15]) << F(", lst: ") << pHexB(rv.buf[16]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x06)) {
		dbg << F("CONFIG_END; cnl: ") << pHexB(rv.buf[10]);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x08)) {
		dbg << F("CONFIG_WRITE_INDEX; cnl: ") << pHexB(rv.buf[10]) << F(", data: ") << pHex((rv.buf+12),(b_len-11));

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x09)) {
		dbg << F("CONFIG_SERIAL_REQ");
		
		} else if ((b_msgTp == 0x01) && (b_by11 == 0x0A)) {
		dbg << F("PAIR_SERIAL, serial: ") << pHex((rv.buf+12),10);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x0E)) {
		dbg << F("CONFIG_STATUS_REQUEST, cnl: ") << pHexB(rv.buf[10]);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x00)) {
		if (b_len == 0x0A) dbg << F("ACK");
		else dbg << F("ACK; data: ") << pHex((rv.buf+11),b_len-10);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x01)) {
		dbg << F("ACK_STATUS; cnl: ") << pHexB(rv.buf[11]) << F(", status: ") << pHexB(rv.buf[12]) << F(", down/up/loBat: ") << pHexB(rv.buf[13]);
		if (b_len > 13) dbg << F(", rssi: ") << pHexB(rv.buf[14]);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x02)) {
		dbg << F("ACK2");
		
		} else if ((b_msgTp == 0x02) && (b_by10 == 0x04)) {
		dbg << F("ACK_PROC; para1: ") << pHex((rv.buf+11),2) << F(", para2: ") << pHex((rv.buf+13),2) << F(", para3: ") << pHex((rv.buf+15),2) << F(", para4: ") << pHexB(rv.buf[17]);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x80)) {
		dbg << F("NACK");

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x84)) {
		dbg << F("NACK_TARGET_INVALID");
		
		} else if ((b_msgTp == 0x03)) {
		dbg << F("AES_REPLY; data: ") << pHex((rv.buf+10),b_len-9);
		
		} else if ((b_msgTp == 0x04) && (b_by10 == 0x01)) {
		dbg << F("TOpHMLAN:SEND_AES_CODE; cnl: ") << pHexB(rv.buf[11]);

		} else if ((b_msgTp == 0x04)) {
		dbg << F("TO_ACTOR:SEND_AES_CODE; code: ") << pHexB(rv.buf[11]);
		
		} else if ((b_msgTp == 0x10) && (b_by10 == 0x00)) {
		dbg << F("INFO_SERIAL; serial: ") << pHex((rv.buf+11),10);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x01)) {
		dbg << F("INFO_PEER_LIST; peer1: ") << pHex((rv.buf+11),4);
		if (b_len >= 19) dbg << F(", peer2: ") << pHex((rv.buf+15),4);
		if (b_len >= 23) dbg << F(", peer3: ") << pHex((rv.buf+19),4);
		if (b_len >= 27) dbg << F(", peer4: ") << pHex((rv.buf+23),4);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x02)) {
		dbg << F("INFO_PARAM_RESPONSE_PAIRS; data: ") << pHex((rv.buf+11),b_len-10);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x03)) {
		dbg << F("INFO_PARAM_RESPONSE_SEQ; offset: ") << pHexB(rv.buf[11]) << F(", data: ") << pHex((rv.buf+12),b_len-11);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x04)) {
		dbg << F("INFO_PARAMETER_CHANGE; cnl: ") << pHexB(rv.buf[11]) << F(", peer: ") << pHex((rv.buf+12),4) << F(", pLst: ") << pHexB(rv.buf[16]) << F(", data: ") << pHex((rv.buf+17),b_len-16);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x06)) {
		dbg << F("INFO_ACTUATOR_STATUS; cnl: ") << pHexB(rv.buf[11]) << F(", status: ") << pHexB(rv.buf[12]) << F(", na: ") << pHexB(rv.buf[13]);
		if (b_len > 13) dbg << F(", rssi: ") << pHexB(rv.buf[14]);
		
		} else if ((b_msgTp == 0x11) && (b_by10 == 0x02)) {
		dbg << F("SET; cnl: ") << pHexB(rv.buf[11]) << F(", value: ") << pHexB(rv.buf[12]) << F(", rampTime: ") << pHex((rv.buf+13),2) << F(", duration: ") << pHex((rv.buf+15),2);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x03)) {
		dbg << F("STOP_CHANGE; cnl: ") << pHexB(rv.buf[11]);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x04) && (b_by11 == 0x00)) {
		dbg << F("RESET");

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x80)) {
		dbg << F("LED; cnl: ") << pHexB(rv.buf[11]) << F(", color: ") << pHexB(rv.buf[12]);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x81) && (b_by11 == 0x00)) {
		dbg << F("LED_ALL; Led1To16: ") << pHex((rv.buf+12),4);
		
		} else if ((b_msgTp == 0x11) && (b_by10 == 0x81)) {
		dbg << F("LED; cnl: ") << pHexB(rv.buf[11]) << F(", time: ") << pHexB(rv.buf[12]) << F(", speed: ") << pHexB(rv.buf[13]);
		
		} else if ((b_msgTp == 0x11) && (b_by10 == 0x82)) {
		dbg << F("SLEEPMODE; cnl: ") << pHexB(rv.buf[11]) << F(", mode: ") << pHexB(rv.buf[12]);
		
		} else if ((b_msgTp == 0x12)) {
		dbg << F("HAVE_DATA");
		
		} else if ((b_msgTp == 0x3E)) {
		dbg << F("SWITCH; dst: ") << pHex((rv.buf+10),3) << F(", na: ") << pHexB(rv.buf[13]) << F(", cnl: ") << pHexB(rv.buf[14]) << F(", counter: ") << pHexB(rv.buf[15]);
		
		} else if ((b_msgTp == 0x3F)) {
		dbg << F("TIMESTAMP; na: ") << pHex((rv.buf+10),2) << F(", time: ") << pHex((rv.buf+12),2);
		
		} else if ((b_msgTp == 0x40)) {
		dbg << F("REMOTE; button: ") << pHexB((rv.buf[10] & 0x3F)) << F(", long: ") << (rv.buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (rv.buf[10] & 0x80 ? 1:0) << F(", counter: ") << pHexB(rv.buf[11]);
		
		} else if ((b_msgTp == 0x41)) {
		dbg << F("SENSOR_EVENT; button: ") <<pHexB((rv.buf[10] & 0x3F)) << F(", long: ") << (rv.buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (rv.buf[10] & 0x80 ? 1:0) << F(", value: ") << pHexB(rv.buf[11]) << F(", next: ") << pHexB(rv.buf[12]);
		
		} else if ((b_msgTp == 0x53)) {
		dbg << F("SENSOR_DATA; cmd: ") << pHexB(rv.buf[10]) << F(", fld1: ") << pHexB(rv.buf[11]) << F(", val1: ") << pHex((rv.buf+12),2) << F(", fld2: ") << pHexB(rv.buf[14]) << F(", val2: ") << pHex((rv.buf+15),2) << F(", fld3: ") << pHexB(rv.buf[17]) << F(", val3: ") << pHex((rv.buf+18),2) << F(", fld4: ") << pHexB(rv.buf[20]) << F(", val4: ") << pHex((rv.buf+21),2);
		
		} else if ((b_msgTp == 0x58)) {
		dbg << F("CLIMATE_EVENT; cmd: ") << pHexB(rv.buf[10]) << F(", valvePos: ") << pHexB(rv.buf[11]);
		
		} else if ((b_msgTp == 0x70)) {
		dbg << F("WEATHER_EVENT; temp: ") << pHex((rv.buf+10),2) << F(", hum: ") << pHexB(rv.buf[12]);

		} else {
		dbg << F("Unknown Message, please report!");
	}
	dbg << F("\n\n");
	#endif



	//dbg << ee.isBroadCast(rv.reID) << '\n';
}


AS hm;



// public:		//---------------------------------------------------------------------------------------------------------
uint8_t  MilliTimer::poll(uint16_t ms) {
	uint8_t ready = 0;
	if (armed) {
		uint16_t remain = next - getMillis();
		if (remain <= 60000) return 0;	
		ready = -remain;
	}
	set(ms);
	return ready;
}
uint16_t MilliTimer::remaining() const {
	uint16_t remain = armed ? next - getMillis() : 0;
	return remain <= 60000 ? remain : 0;
}
void     MilliTimer::set(uint16_t ms) {
	armed = ms != 0;
	if (armed)
	next = getMillis() + ms - 1;
}

