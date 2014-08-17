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

	initMillis();																		// start the millis counter

	// everything is setuped, enable RF functionality
	_enableGDO0Int;																		// enable interrupt to get a signal while receiving data
}

// - poll functions --------------------------------
void AS::poll(void) {

	// check if something received
	if (ccGDO0()) {																		// check if something was received
		cc.rcvData(rv.buf);																// copy the data into the receiver module
		if (rv.hasData) {
			rv.decode();																// decode the string
			received();																	// and jump in the received function
		}
	}

	sendSlcList();																		// poll the slice list send function
	
	// check if something is to send

	// check if we could go to standby
	
	// some sanity poll routines
	
}
void AS::sendSlcList(void) {
	if (!slcList.active) return;
	
	// check if send function has a free slot, otherwise return
	uint8_t cnt;

	if        (slcList.peer) {															// INFO_PEER_LIST
		cnt = ee.getPeerListSlc(slcList.cnl,slcList.curSlc,sn.buf);						// get the slice and the amount of bytes
		slcList.curSlc++;																// increase slice counter
		dbg << "peer slc: " << pHex(sn.buf,cnt) << '\n';								// write to send buffer

		} else if (slcList.reg2) {														// INFO_PARAM_RESPONSE_PAIRS
		cnt = ee.getRegListSlc(slcList.cnl,slcList.lst,slcList.idx,slcList.curSlc,sn.buf);// get the slice and the amount of bytes
		slcList.curSlc++;																// increase slice counter
		dbg << "reg2 slc: " << pHex(sn.buf,cnt) << '\n';								// write to send buffer
		
		} else if (slcList.reg3) {														// INFO_PARAM_RESPONSE_SEQ

	}

	if (slcList.curSlc == slcList.totSlc) {												// if everything is send, we could empty the struct
		memset((void*)&slcList,0,6);													// by memset
		//dbg << "end: " << slcList.active << slcList.peer << slcList.reg2 << slcList.reg3 << '\n';
	}
}

// - received functions ----------------------------
void AS::received(void) {
	uint8_t bIntend = ee.getIntend(rv.reID,rv.toID);									// get the intend of the message

	// some debugs
	#ifdef AS_DBG																		// only if AS debug is set
	dbg << (char)bIntend << F("> ") << pHex(rv.buf,rv.len) << '\n';
	#endif
	#ifdef AS_DBG_EX																	// only if extended AS debug is set

	dbg << F("   ");																	// save some byte and send 3 blanks once, instead of having it in every if
	
	if        ((rv.msgTyp == 0x00)) {
		dbg << F("DEVICE_INFO; fw: ") << pHex((rv.buf+10),1) << F(", type: ") << pHex((rv.buf+11),2) << F(", serial: ") << pHex((rv.buf+13),10) << '\n';
		dbg << F("              , class: ") << pHexB(rv.buf[23]) << F(", pCnlA: ") << pHexB(rv.buf[24]) << F(", pCnlB: ") << pHexB(rv.buf[25]) << F(", na: ") << pHexB(rv.buf[26]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x01)) {
		dbg << F("CONFIG_PEER_ADD; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnlA: ") << pHexB(rv.buf[15]) << F(", pCnlB: ") << pHexB(rv.buf[16]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x02)) {
		dbg << F("CONFIG_PEER_REMOVE; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnlA: ") << pHexB(rv.buf[15]) << F(", pCnlB: ") << pHexB(rv.buf[16]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x03)) {
		dbg << F("CONFIG_PEER_LIST_REQ; cnl: ") << pHexB(rv.buf[10]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x04)) {
		dbg << F("CONFIG_PARAM_REQ; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnl: ") << pHexB(rv.buf[15]) << F(", lst: ") << pHexB(rv.buf[16]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x05)) {
		dbg << F("CONFIG_START; cnl: ") << pHexB(rv.buf[10]) << F(", peer: ") << pHex((rv.buf+12),3) << F(", pCnl: ") << pHexB(rv.buf[15]) << F(", lst: ") << pHexB(rv.buf[16]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x06)) {
		dbg << F("CONFIG_END; cnl: ") << pHexB(rv.buf[10]);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x08)) {
		dbg << F("CONFIG_WRITE_INDEX; cnl: ") << pHexB(rv.buf[10]) << F(", data: ") << pHex((rv.buf+12),(rv.len-11));

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x09)) {
		dbg << F("CONFIG_SERIAL_REQ");
		
		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x0A)) {
		dbg << F("PAIR_SERIAL, serial: ") << pHex((rv.buf+12),10);

		} else if ((rv.msgTyp == 0x01) && (rv.by11 == 0x0E)) {
		dbg << F("CONFIG_STATUS_REQUEST, cnl: ") << pHexB(rv.buf[10]);

		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x00)) {
		if (rv.len == 0x0A) dbg << F("ACK");
		else dbg << F("ACK; data: ") << pHex((rv.buf+11),rv.len-10);

		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x01)) {
		dbg << F("ACK_STATUS; cnl: ") << pHexB(rv.buf[11]) << F(", status: ") << pHexB(rv.buf[12]) << F(", down/up/loBat: ") << pHexB(rv.buf[13]);
		if (rv.len > 13) dbg << F(", rssi: ") << pHexB(rv.buf[14]);

		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x02)) {
		dbg << F("ACK2");
		
		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x04)) {
		dbg << F("ACK_PROC; para1: ") << pHex((rv.buf+11),2) << F(", para2: ") << pHex((rv.buf+13),2) << F(", para3: ") << pHex((rv.buf+15),2) << F(", para4: ") << pHexB(rv.buf[17]);

		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x80)) {
		dbg << F("NACK");

		} else if ((rv.msgTyp == 0x02) && (rv.by10 == 0x84)) {
		dbg << F("NACK_TARGET_INVALID");
		
		} else if ((rv.msgTyp == 0x03)) {
		dbg << F("AES_REPLY; data: ") << pHex((rv.buf+10),rv.len-9);
		
		} else if ((rv.msgTyp == 0x04) && (rv.by10 == 0x01)) {
		dbg << F("TOpHMLAN:SEND_AES_CODE; cnl: ") << pHexB(rv.buf[11]);

		} else if ((rv.msgTyp == 0x04)) {
		dbg << F("TO_ACTOR:SEND_AES_CODE; code: ") << pHexB(rv.buf[11]);
		
		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x00)) {
		dbg << F("INFO_SERIAL; serial: ") << pHex((rv.buf+11),10);

		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x01)) {
		dbg << F("INFO_PEER_LIST; peer1: ") << pHex((rv.buf+11),4);
		if (rv.len >= 19) dbg << F(", peer2: ") << pHex((rv.buf+15),4);
		if (rv.len >= 23) dbg << F(", peer3: ") << pHex((rv.buf+19),4);
		if (rv.len >= 27) dbg << F(", peer4: ") << pHex((rv.buf+23),4);

		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x02)) {
		dbg << F("INFO_PARAM_RESPONSE_PAIRS; data: ") << pHex((rv.buf+11),rv.len-10);

		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x03)) {
		dbg << F("INFO_PARAM_RESPONSE_SEQ; offset: ") << pHexB(rv.buf[11]) << F(", data: ") << pHex((rv.buf+12),rv.len-11);

		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x04)) {
		dbg << F("INFO_PARAMETER_CHANGE; cnl: ") << pHexB(rv.buf[11]) << F(", peer: ") << pHex((rv.buf+12),4) << F(", pLst: ") << pHexB(rv.buf[16]) << F(", data: ") << pHex((rv.buf+17),rv.len-16);

		} else if ((rv.msgTyp == 0x10) && (rv.by10 == 0x06)) {
		dbg << F("INFO_ACTUATOR_STATUS; cnl: ") << pHexB(rv.buf[11]) << F(", status: ") << pHexB(rv.buf[12]) << F(", na: ") << pHexB(rv.buf[13]);
		if (rv.len > 13) dbg << F(", rssi: ") << pHexB(rv.buf[14]);
		
		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x02)) {
		dbg << F("SET; cnl: ") << pHexB(rv.buf[11]) << F(", value: ") << pHexB(rv.buf[12]) << F(", rampTime: ") << pHex((rv.buf+13),2) << F(", duration: ") << pHex((rv.buf+15),2);

		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x03)) {
		dbg << F("STOP_CHANGE; cnl: ") << pHexB(rv.buf[11]);

		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x04) && (rv.by11 == 0x00)) {
		dbg << F("RESET");

		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x80)) {
		dbg << F("LED; cnl: ") << pHexB(rv.buf[11]) << F(", color: ") << pHexB(rv.buf[12]);

		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x81) && (rv.by11 == 0x00)) {
		dbg << F("LED_ALL; Led1To16: ") << pHex((rv.buf+12),4);
		
		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x81)) {
		dbg << F("LED; cnl: ") << pHexB(rv.buf[11]) << F(", time: ") << pHexB(rv.buf[12]) << F(", speed: ") << pHexB(rv.buf[13]);
		
		} else if ((rv.msgTyp == 0x11) && (rv.by10 == 0x82)) {
		dbg << F("SLEEPMODE; cnl: ") << pHexB(rv.buf[11]) << F(", mode: ") << pHexB(rv.buf[12]);
		
		} else if ((rv.msgTyp == 0x12)) {
		dbg << F("HAVE_DATA");
		
		} else if ((rv.msgTyp == 0x3E)) {
		dbg << F("SWITCH; dst: ") << pHex((rv.buf+10),3) << F(", na: ") << pHexB(rv.buf[13]) << F(", cnl: ") << pHexB(rv.buf[14]) << F(", counter: ") << pHexB(rv.buf[15]);
		
		} else if ((rv.msgTyp == 0x3F)) {
		dbg << F("TIMESTAMP; na: ") << pHex((rv.buf+10),2) << F(", time: ") << pHex((rv.buf+12),2);
		
		} else if ((rv.msgTyp == 0x40)) {
		dbg << F("REMOTE; button: ") << pHexB((rv.buf[10] & 0x3F)) << F(", long: ") << (rv.buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (rv.buf[10] & 0x80 ? 1:0) << F(", counter: ") << pHexB(rv.buf[11]);
		
		} else if ((rv.msgTyp == 0x41)) {
		dbg << F("SENSOR_EVENT; button: ") <<pHexB((rv.buf[10] & 0x3F)) << F(", long: ") << (rv.buf[10] & 0x40 ? 1:0) << F(", lowBatt: ") << (rv.buf[10] & 0x80 ? 1:0) << F(", value: ") << pHexB(rv.buf[11]) << F(", next: ") << pHexB(rv.buf[12]);
		
		} else if ((rv.msgTyp == 0x53)) {
		dbg << F("SENSOR_DATA; cmd: ") << pHexB(rv.buf[10]) << F(", fld1: ") << pHexB(rv.buf[11]) << F(", val1: ") << pHex((rv.buf+12),2) << F(", fld2: ") << pHexB(rv.buf[14]) << F(", val2: ") << pHex((rv.buf+15),2) << F(", fld3: ") << pHexB(rv.buf[17]) << F(", val3: ") << pHex((rv.buf+18),2) << F(", fld4: ") << pHexB(rv.buf[20]) << F(", val4: ") << pHex((rv.buf+21),2);
		
		} else if ((rv.msgTyp == 0x58)) {
		dbg << F("CLIMATE_EVENT; cmd: ") << pHexB(rv.buf[10]) << F(", valvePos: ") << pHexB(rv.buf[11]);
		
		} else if ((rv.msgTyp == 0x70)) {
		dbg << F("WEATHER_EVENT; temp: ") << pHex((rv.buf+10),2) << F(", hum: ") << pHexB(rv.buf[12]);

		} else {
		dbg << F("Unknown Message, please report!");
	}
	dbg << F("\n\n");
	#endif

	// filter out unknown or not for us
	if ((bIntend == 'l') || (bIntend == 'u')) {											// not for us, or sender unknown
		rv.buf[0] = 0;																	// clear receive buffer
		return;
	}
	
	// filter out repeated messages
	
	
	// check which type of message was received
	if         ((rv.msgTyp == 0x01) && (rv.by11 == 0x01)) {								// CONFIG_PEER_ADD
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 01 02 04 01   01  1F A6 5C  06         05
		// do something with the information ----------------------------------

		// first call remPeer to avoid doubles
		uint8_t ret = ee.addPeer(rv.buf[10],rv.buf+12);									// send to addPeer function
		
		// let module registrations know of the change

		if ((ret) && (rv.ackRq)) sendACK();												// send appropriate answer
		else if (rv.ackRq) sendNACK();


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x02)) {								// CONFIG_PEER_REMOVE
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 01 02 04 01   02  1F A6 5C  06         05
		// do something with the information ----------------------------------
		
		uint8_t ret = ee.remPeer(rv.buf[10],rv.buf+12);									// call the remPeer function
		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x03)) {								// CONFIG_PEER_LIST_REQ
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 05 A0 01 63 19 63 01 02 04 01  03
		// do something with the information ----------------------------------
		
		slcList.totSlc = ee.countPeerSlc(rv.buf[10]);									// how many slices are need
		slcList.cnl = rv.buf[10];														// send input to the send peer function
		slcList.peer = 1;																// set the type of answer
		slcList.active = 1;																// start the send function

		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x04)) {								// CONFIG_PARAM_REQ
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 04 A0 01 63 19 63 01 02 04 01  04 00 00 00  00       01
		// do something with the information ----------------------------------

		slcList.idx = ee.getIdxByPeer(rv.buf[10], rv.buf+12);							// fill struct
		slcList.totSlc = ee.countRegListSlc(rv.buf[10], rv.buf[16]);					// how many slices are need
		slcList.cnl = rv.buf[10];														// send input to the send peer function
		slcList.lst = rv.buf[16];														// send input to the send peer function
		slcList.reg2 = 1;																// set the type of answer
		
		if ((slcList.idx != 0xff) && (slcList.totSlc > 0)) slcList.active = 1;			// only send register content if something is to send															// start the send function
		else memset((void*)&slcList,0,6);												// otherwise empty variable
		
		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x05)) {								// CONFIG_START
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 01 A0 01 63 19 63 01 02 04 00  05 00 00 00  00       00
		// do something with the information ----------------------------------

		cnfFlag.idx = ee.getIdxByPeer(rv.buf[10], rv.buf+12);							// fill structure to remember where to write
		cnfFlag.cnl = rv.buf[10];
		cnfFlag.lst = rv.buf[16];
		if (cnfFlag.idx != 0xff) {
			cnfFlag.active = 1;															// set active if there is no error on index
			// set message id flag to config in send module
		}
		
		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x06)) {								// CONFIG_END
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 01 A0 01 63 19 63 01 02 04 00  06
		// do something with the information ----------------------------------

		cnfFlag.active = 0;																// set inactive
		// remove message id flag to config in send module
		
		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x07)) {								// CONFIG_WRITE_INDEX
		// sample needed

		
	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x08)) {								// CONFIG_WRITE_INDEX
		// description --------------------------------------------------------
		//                                  Cnl    Data
		// l> 13 02 A0 01 63 19 63 01 02 04 00  08 02 01 0A 63 0B 19 0C 63
		// do something with the information ----------------------------------

		if ((cnfFlag.active) && (cnfFlag.cnl == rv.buf[10])) {							// check if we are in config mode and if the channel fit
			ee.setListArray(cnfFlag.cnl, cnfFlag.lst, cnfFlag.idx, rv.len-11, rv.buf+12);// write the string to eeprom
		}
		
		if (rv.ackRq) sendACK();														// send appropriate answer


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x09)) {								// CONFIG_SERIAL_REQ


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x0A)) {								// PAIR_SERIAL
		//SERIALNO       => '04,,$val=pack("H*",$val)', } },


	} else if  ((rv.msgTyp == 0x01) && (rv.by11 == 0x0E)) {								// CONFIG_STATUS_REQUEST
		//CHANNEL => "0,2", } },


	} else if  ((rv.msgTyp == 0x02) && (rv.by10 == 0x00)) {								// ACK

		
	} else if  ((rv.msgTyp == 0x02) && (rv.by10 == 0x01)) {								// ACK_STATUS
		//CHANNEL        => "02,2",
		//STATUS         => "04,2",
		//DOWN           => '06,02,$val=(hex($val)&0x20)?1:0',
		//UP             => '06,02,$val=(hex($val)&0x10)?1:0',
		//LOWBAT         => '06,02,$val=(hex($val)&0x80)?1:0',
		//RSSI           => '08,02,$val=(-1)*(hex($val))', }},


	} else if  ((rv.msgTyp == 0x02) && (rv.by10 == 0x02)) {								// ACK2 - smokeDetector pairing only?


	} else if  ((rv.msgTyp == 0x02) && (rv.by10 == 0x04)) {								// ACK-proc - connected to AES??
		//Para1          => "02,4",
		//Para2          => "06,4",
		//Para3          => "10,4",
		//Para4          => "14,2",}}, # remote?

		
	} else if  ((rv.msgTyp == 0x02) && (rv.by11 == 0x80)) {								// NACK

		
	} else if  ((rv.msgTyp == 0x02) && (rv.by11 == 0x84)) {								// NACK_TARGET_INVALID


	} else if  (rv.msgTyp == 0x12) {													// HAVE_DATA

		
	} else if  (rv.msgTyp == 0x3E) {													// SWITCH
		//DST      => "00,6",
		//UNKNOWN  => "06,2",
		//CHANNEL  => "08,2",
		//COUNTER  => "10,2", } },


	} else if  (rv.msgTyp == 0x3F) {													// TimeStamp
		//UNKNOWN  => "00,4",
		//TIME     => "04,2", } },


	} else if  (rv.msgTyp == 0x40) {													// REMOTE
		//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
		//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
		//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
		//COUNTER  => "02,2", } },


	} else if  (rv.msgTyp == 0x41) {													// Sensor_event
		//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
		//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
		//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
		//NBR      => '02,2,$val=(hex($val))',
		//VALUE    => '04,2,$val=(hex($val))',} },


	} else if  (rv.msgTyp == 0x53) {													// SensorData
		//CMD => "00,2",
		//Fld1=> "02,2",
		//Val1=> '04,4,$val=(hex($val))',
		//Fld2=> "08,2",
		//Val2=> '10,4,$val=(hex($val))',
		//Fld3=> "14,2",
		//Val3=> '16,4,$val=(hex($val))',
		//Fld4=> "20,2",
		//Val4=> '24,4,$val=(hex($val))'} },


	} else if  (rv.msgTyp == 0x58) {													// ClimateEvent
		//CMD      => "00,2",
		//ValvePos => '02,2,$val=(hex($val))', } },
		
		
	} else if  (rv.msgTyp == 0x59) {													// setTeamTemp
		//CMD      => "00,2",
		//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
		//mode     => '02,2,$val=(hex($val) & 0x3)',} },


	} else if  (rv.msgTyp == 0x70) {													// WeatherEvent
		//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
		//HUM      => '04,2,$val=(hex($val))', } },


	}

	rv.buf[0] = 0;																		// message progressed, nothing do to any more
}

// - send functions --------------------------------
void AS::sendACK(void) {
	// description --------------------------------------------------------
	//                reID      toID      ACK      
	// l> 0A 24 80 02 1F B7 4A  63 19 63  00
	// do something with the information ----------------------------------
		
	sn.buf[0] = 0x0a;
	sn.buf[1] = rv.reCnt;
	sn.buf[2] = 0x80;
	sn.buf[3] = 0x02;
	memcpy(sn.buf+4,HMID,3);
	memcpy(sn.buf+7,rv.reID,3);
	sn.buf[10];
	dbg << "<- " << pHex(sn.buf,sn.len) << '\n';
}
void AS::sendACK_STATUS(void) {
	//"02;p01=01"   => { txt => "ACK_STATUS",  params => {
	//CHANNEL        => "02,2",
	//STATUS         => "04,2",
	//DOWN           => '06,02,$val=(hex($val)&0x20)?1:0',
	//UP             => '06,02,$val=(hex($val)&0x10)?1:0',
	//LOWBAT         => '06,02,$val=(hex($val)&0x80)?1:0',
	//RSSI           => '08,02,$val=(-1)*(hex($val))', }},
}
void AS::sendNACK(void) {
	// description --------------------------------------------------------
	//                reID      toID      NACK
	// l> 0A 24 80 02 1F B7 4A  63 19 63  80
	// do something with the information ----------------------------------
	
	sn.buf[0] = 0x0a;
	sn.buf[1] = rv.reCnt;
	sn.buf[2] = 0x80;
	sn.buf[3] = 0x02;
	memcpy(sn.buf+4,HMID,3);
	memcpy(sn.buf+7,rv.reID,3);
	sn.buf[10];
	dbg << "<- " << pHex(sn.buf,sn.len) << '\n';
}
void AS::sendNACK_TARGET_INVALID(void) {
	//"02;p01=84"   => { txt => "NACK_TARGET_INVALID"},
}
void AS::sendINFO_SERIAL(void) {
	//"10;p01=00"   => { txt => "INFO_SERIAL", params => {
	//SERIALNO => '02,20,$val=pack("H*",$val)'},},
}
void AS::sendINFO_PEER_LIST(void) {
	//"10;p01=01"   => { txt => "INFO_PEER_LIST", params => {
	//PEER1 => '02,8,$val=CUL_HM_id2Name($val)',
	//PEER2 => '10,8,$val=CUL_HM_id2Name($val)',
	//PEER3 => '18,8,$val=CUL_HM_id2Name($val)',
	//PEER4 => '26,8,$val=CUL_HM_id2Name($val)'},},
}
void AS::sendINFO_PARAM_RESPONSE_PAIRS(void) {
	//"10;p01=02"   => { txt => "INFO_PARAM_RESPONSE_PAIRS", params => {
	//DATA => "2,", },},
}
void AS::sendINFO_PARAM_RESPONSE_SEQ(void) {
	//"10;p01=03"   => { txt => "INFO_PARAM_RESPONSE_SEQ", params => {
	//OFFSET => "2,2",
	//DATA   => "4,", },},
}
void AS::sendINFO_PARAMETER_CHANGE(void) {
	//"10;p01=04"   => { txt => "INFO_PARAMETER_CHANGE", params => {
	//CHANNEL => "2,2",
	//PEER    => '4,8,$val=CUL_HM_id2Name($val)',
	//PARAM_LIST => "12,2",
	//DATA => '14,,$val =~ s/(..)(..)/ $1:$2/g', } },
}
void AS::sendINFO_ACTUATOR_STATUS(void) {
	//"10;p01=06"   => { txt => "INFO_ACTUATOR_STATUS", params => {
	//CHANNEL => "2,2",
	//STATUS  => '4,2',
	//UNKNOWN => "6,2",
	//RSSI    => '08,02,$val=(-1)*(hex($val))' } },
}
void AS::sendINFO_TEMP(void) {
	//"10;p01=0A"   => { txt => "INFO_TEMP", params => {
	//SET     => '2,4,$val=(hex($val)>>10)&0x3F',
	//ACT     => '2,4,$val=hex($val)&0x3FF',
	//ERR     => "6,2",
	//VALVE   => "6,2",
	//MODE    => "6,2" } },
}
void AS::sendHAVE_DATA(void) {
	//"12"          => { txt => "HAVE_DATA"},
}
void AS::sendSWITCH(void) {
	//"3E"          => { txt => "SWITCH"      , params => {
	//DST      => "00,6",
	//UNKNOWN  => "06,2",
	//CHANNEL  => "08,2",
	//COUNTER  => "10,2", } },
}
void AS::sendTimeStamp(void) {
	//"3F"          => { txt => "TimeStamp"   , params => {
	//UNKNOWN  => "00,4",
	//TIME     => "04,2", } },
}
void AS::sendREMOTE(void) {
	//"40"          => { txt => "REMOTE"      , params => {
	//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
	//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
	//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
	//COUNTER  => "02,2", } },
}
void AS::sendSensor_event(void) {
	//"41"          => { txt => "Sensor_event", params => {
	//BUTTON   => '00,2,$val=(hex($val)&0x3F)',
	//LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
	//LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
	//NBR      => '02,2,$val=(hex($val))',
	//VALUE    => '04,2,$val=(hex($val))',} },
}
void AS::sendSensorData(void) {
	//"53"          => { txt => "SensorData"  , params => {
	//CMD => "00,2",
	//Fld1=> "02,2",
	//Val1=> '04,4,$val=(hex($val))',
	//Fld2=> "08,2",
	//Val2=> '10,4,$val=(hex($val))',
	//Fld3=> "14,2",
	//Val3=> '16,4,$val=(hex($val))',
	//Fld4=> "20,2",
	//Val4=> '24,4,$val=(hex($val))'} },
}
void AS::sendClimateEvent(void) {
	//"58"          => { txt => "ClimateEvent", params => {
	//CMD      => "00,2",
	//ValvePos => '02,2,$val=(hex($val))', } },
}
void AS::sendSetTeamTemp(void) {
	//"59"          => { txt => "setTeamTemp" , params => {
	//CMD      => "00,2",
	//desTemp  => '02,2,$val=((hex($val)>>2) /2)',
	//mode     => '02,2,$val=(hex($val) & 0x3)',} },
}
void AS::sendWeatherEvent(void) {
	//"70"          => { txt => "WeatherEvent", params => {
	//TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//HUM      => '04,2,$val=(hex($val))', } },
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

