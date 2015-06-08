/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: In-Circuit radino CC1101, Platform=ICT_Boards, Package=radino
*/

#define __AVR_ATmega32u4__
#define __AVR_ATmega32U4__
#define ARDUINO 158
#define ARDUINO_MAIN
#define F_CPU 8000000L
#define printf iprintf
#define __ICT_BOARDS__
extern "C" void __cxa_pure_virtual() {;}

//
//
static void     configTSL(void);
void serialEvent(void);

#include "C:\Users\MCHHBITT\Documents\02_Arduino\hardware\radino\ICT_Boards\variants\ictmicro\pins_arduino.h" 
#include "C:\Program Files\arduino-1.5.8\hardware\arduino\avr\cores\arduino\arduino.h"
#include <CUSTOM_LUX.ino>
#include <hardware.cpp>
#include <hardware.h>
#include <register.h>
