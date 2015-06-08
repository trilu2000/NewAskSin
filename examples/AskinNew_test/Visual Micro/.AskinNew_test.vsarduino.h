/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Pro or Pro Mini w/ ATmega328 (3.3V, 8 MHz), Platform=avr, Package=arduino
*/

#define __AVR_ATmega328p__
#define __AVR_ATmega328P__
#define ARDUINO 158
#define ARDUINO_MAIN
#define F_CPU 8000000L
#define __AVR__
extern "C" void __cxa_pure_virtual() {;}

//
//
void initTH1();
void measureTH1();
static void     initTSL(void);
static uint16_t readTSL(void);
static void     configTSL(void);
void serialEvent(void);

#include "C:\Program Files\arduino-1.5.8\hardware\arduino\avr\variants\eightanaloginputs\pins_arduino.h" 
#include "C:\Program Files\arduino-1.5.8\hardware\arduino\avr\cores\arduino\arduino.h"
#include <AskinNew_test.ino>
#include <AskinNew_test.h>
#include <hardware.cpp>
#include <hardware.h>
#include <register.h>
