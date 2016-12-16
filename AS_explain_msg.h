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



#endif