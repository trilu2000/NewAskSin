/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin framework main class -------------------------------------------------------------------------------------------
* - with a lot of support from many people at FHEM forum
*   thanks a lot to martin876, dirk, pa-pa, martin, Dietmar63 and all i have not personal named here
*   special thank you to https://git.zerfleddert.de/hmcfgusb/AES/ for bidcos(R) AES explanation
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _NEWASKSIN_H
#define _NEWASKSIN_H

#include "as_type_defs.h"

#include "as_main.h"
#include "as_aes.h"
#include "as_communication.h"
#include "as_conf_button.h"
#include "as_status_led.h"
#include "as_battery.h"
#include "as_power.h"			// needs to be reworked

#include "cm_master.h"
#include "cm_maintenance.h" 
#include "cm_remote.h"
#include "cm_switch.h"
#include "cm_dimmer.h"

#include "version.h"
#include "waittimer.h"



/* forward declarations for all classes and structs needed by asksin; extern is used while majority is depending on device configuration and
*  therefor defined in register.h in user sketch folder...
*/

/*
* @brief Global definition of a struct to hold the device identification related information.
*
* First bytes of eeprom holds all device specific information for identification. Struct is used
* to hold this information in memory on hand.
*  2 byte - magic byte
*  3 byte - homematic id
* 10 byte - serial number
*  1 byte - aes key index
* 16 byte - homematic aes key
*/
extern s_dev_ident dev_ident;

/*
* @brief Global definition of a struct to hold all operational needed device variables
* MAID is valid after initialization of AS with AS::init(). While not paired to a central,
* MAID equals the broadcast address 000000. This is the case after first upload of a firmware
* when an unconfigured EEprom is encountered (EEprom magic number does not match) or after a
* reset of the device (RESET command sent by paired central or initiated by means of the
* config button).
*/
extern s_dev_operate dev_operate;

/*
* @brief Helper structure for keeping track of active pairing mode
*/
extern s_pair_mode pair_mode;
/*
* @brief Helper structure for keeping track of active config mode
*/
extern s_config_mode config_mode;

extern s_rcv_msg rcv_msg;
extern s_snd_msg snd_msg;


/*
* @brief Global definition of device HMSerialData. Must be declared in user space.
*
* The HMSerialData holds the default HMID, HMSerial and HMKEY.
* At every start, values of HMID and HMSerial was copied to related variables.
* The HKEY was only copied at initial sketch start in the EEprom
*/
extern const uint8_t HMSerialData[] PROGMEM;
/*
* @brief Settings of HM device
* firmwareVersion: The firmware version reported by the device
*                  Sometimes this value is important for select the related device-XML-File
*
* modelID:         Important for identification of the device.
*                  @See Device-XML-File /device/supported_types/type/parameter/const_value
*
* subType:         Identifier if device is a switch or a blind or a remote
* DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
*                  Other bytes not known.
*                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
*/
extern const uint8_t dev_static[] PROGMEM;

extern AS *hm;
extern AES *aes;
extern COM *com;
extern CBN *cbn;
extern LED *led;
extern BAT *bat;
extern POM *pom;

/* as there is no way to get the channel by setting up the pointer array for channel modules we use this
*  byte to identify the channel we are actually setting up, increased in the constructor...
the overall amount will be kept for runtime to step through the different instances. */
extern uint8_t cnl_max;

extern s_peer_msg peer_msg;
extern s_list_msg list_msg;

extern CM_MASTER *cmm[];

extern const uint8_t cm_maintenance_ChnlLen;
extern const uint8_t cm_maintenance_ChnlReg[];
extern const uint8_t cm_maintenance_ChnlDef[];
extern uint8_t cm_maintenance_ChnlVal[];



#endif