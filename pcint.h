
#ifndef _PCINT_h
#define _PCINT_h


#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define dbg Serial
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

//- some general defines ------------------------------------------------------------------------------------
#define PCINT_SLOTS 10																			// define the max amount of slots for interrupts to register
#define PCINT_DEBOUNCE_TIME 10																	// milliseconds to debounce


//- hw specific defines -------------------------------------------------------------------------------------
#if defined __AVR_ATmega168__ || defined __AVR_ATmega168A__ || defined __AVR_ATmega168P__ || defined __AVR_ATmega168PA__ || defined __AVR_ATmega328__ || defined __AVR_ATmega328P__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#elif defined __AVR_ATmega640__ || defined __AVR_ATmega2560__ || defined __AVR_ATmega1280__ || defined __AVR_ATmega1281__ || defined __AVR_ATmega2561__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#elif defined __AVR_ATmega32U4__ || defined __AVR_ATmega16U4__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels
#define PCINT06 &PCMSK0,&PCICR,PCIE0,PCINT6,DDRB,&PORTB,PORTB6


#elif defined __AVR_ATmega1284P__ || defined __AVR_ATmega1284__ || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#elif defined __AVR_ATtiny24__  || defined __AVR_ATtiny24A__ || defined __AVR_ATtiny44__ || defined __AVR_ATtiny44A__ || defined __AVR_ATtiny84__ || defined  __AVR_ATtiny84A_
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#elif defined __AVR_ATtiny25__ || defined  __AVR_ATtiny45__ || defined  __AVR_ATtiny85__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#endif

//- some macros ---------------------------------------------------------------------------------------------
#define pinInput(DDR,PIN)     (DDR   &= ~_BV(PIN))
#define setPinHigh(PORT,PIN)  (*PORT |=  _BV(PIN))
#define setPinLow(PORT,PIN)   (*PORT &= ~_BV(PIN))
#define regPCIE(ICR,PORT)     (*ICR  |= _BV(PORT))
#define regPCINT(MASK,PORT)   (*MASK |= _BV(PORT))


//- include and typedef for delegate ------------------------------------------------------------------------ 
#include "Fastdelegate.h"																		// include the library

using namespace fastdelegate;
typedef FastDelegate2<uint8_t, uint8_t> s_pcint_dlgt;											// void function_name(uint8_t pcint_pin, uint8_t pcint_mode)


//- declare and size the structs 
struct s_pcint_table {
	uint8_t pin : 7;																			// to store and find the interrupt pin
	uint8_t debounce : 1;																		// should we debounce?
	uint8_t ie;																					// interrupt vector
	s_pcint_dlgt dlgt;																			// delegate for the call back function
};																								// size the table in the cpp file
extern s_pcint_table pcint_table[];

struct  s_pcint_vector_byte {
	uint8_t cur;
	uint8_t prev;
	uint8_t mask;
	uint32_t time;
};
extern volatile s_pcint_vector_byte pcint_vector_byte[];										// size of the table depending on avr type in the cpp file


//- function to register interrupt
void    pcint_init(void);
uint8_t pcinit_register(volatile uint8_t *pcint_MASK, volatile uint8_t *pcint_ICR, uint8_t pcint_PCIE, uint8_t pcint_PCIPIN, uint8_t pcint_DDR, volatile uint8_t *pcint_PORT, uint8_t pcint_PIN, uint8_t bool_Config, s_pcint_dlgt pcint_CallBack);

//- function to deregister interrupt


//- poll function for debouncing




#endif

