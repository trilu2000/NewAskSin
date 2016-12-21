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

#ifndef _NAS_H
#define _NAS_H

#include "AS_type_defs.h"
#include "main_aes.h"
#include "main_communication.h"
#include "main_conf_button.h"
#include "main_status_led.h"

#include "macros.h"
#include "version.h"
#include "wait_timer.h"

#include "cmMaster.h"
#include "Power.h"
#include "Battery.h"


/*
* @brief Struct to hold the buffer for any send or received string with some flags for further processing
*/
extern s_rcv_msg rcv_msg;
extern s_snd_msg snd_msg;

extern AES *aes;
extern COM *com;
extern CBN *cbn;
extern LED *led;

/**
 * @short Main class for implementation of the AskSin protocol stack.
 * Every device needs exactly one instance of this class.
 *
 * AS is responsible for maintaining the complete infrastructure from
 * device register representation, non-volatile storage of device configuration
 * in the eeprom, to providing interfaces for user modules that implement
 * actual device logic.
 *
 * This is a very simple, non-working example of the basic API used in the main
 * routines:
 * @include docs/snippets/basic-AS.cpp
 *
 * All send functions are used by sensor or actor classes like THSensor or Dimmer.
 */
class AS {

public:  //----------------------------------------------------------------------------------------------------------------
	AS() {}																					// constructor

	/*
	 * @brief Initialize the AS module
	 *
	 * init() has to be called from the main setup() routine.
	 */
	void init(void);

	/**
	 * @brief Poll routine for regular operation
	 *
	 * poll() needs to be called regularily from the main loop(). It takes care of
	 * all major tasks like sending and receiving messages, device configuration
	 * and message delegation.
	 */
	void poll(void);


	/* - receive functions ------------------------------------------------------------------------------------------------
	* @brief Received messages are stored and prepared in the rcv_msg struct. AS:poll is calling while rcv_msg.active
	* is set to 1. All receive functions are handled within the AS class - some forwarded to the channel module class.
	* The intent is to overload them there by the respective user channel module and work with the information accordingly.
	*/
	void rcv_poll(void);																	// poll function
	void get_intend(void);																	// checks the received string if addresses are known
	void process_message(void);

	void snd_poll(void);																	// poll function, process if something is to send


	/* - hardware related functions without any relation to a specific channel */
	void INSTRUCTION_RESET(s_m1104xx *buf);
	void INSTRUCTION_ENTER_BOOTLOADER(s_m1183xx *buf);
	void INSTRUCTION_ADAPTION_DRIVE_SET(s_m1187xx *buf);
	void INSTRUCTION_ENTER_BOOTLOADER2(s_m11caxx *buf);

};


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


/*
* @fn void everyTimeStart()
* @brief Callback for actions after bootup
*
* This function is called when AS has started and before the main loop runs.
*/
extern void everyTimeStart(void);
/*
* @fn void firstTimeStart()
* @brief Callback for actions after EEprom deletion
*
* This function needs to be defined in the user code space. It can be used to
* set the data of complete Lists with EE::setList() or single registers using
* EE::setListArray()
*/
extern void firstTimeStart(void);


extern AS hm;



//- some helpers ----------------------------------------------------------------------------------------------------------
uint32_t byteTimeCvt(uint8_t tTime);
uint32_t intTimeCvt(uint16_t iTime);

uint8_t isEmpty(void *ptr, uint8_t len);												// check if a byte array is empty
#define isEqual(p1,p2,len) memcmp(p1, p2, len)?0:1										// check if a byte array is equal

//- -----------------------------------------------------------------------------------------------------------------------





#endif

