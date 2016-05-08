//- -----------------------------------------------------------------------------------------------------------------------
// part of AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- pin change interrupt handler functions --------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _PCINT_h
#define _PCINT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


//- helper to make serial communication simpler -------------------------------------------------------------
#ifndef dbg
	#define dbg Serial
	template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }
#endif


//- some general defines ------------------------------------------------------------------------------------
#define PCINT_SLOTS 10																			// define the max amount of slots for interrupts to register
#define PCINT_DEBOUNCE_TIME 10																	// milliseconds to debounce
#ifdef HAS_OWN_MILLIS_TIMER
#define PCINT_MILLIS getMillis()
#else
#define PCINT_MILLIS millis()
#endif

#define PIN_HIGH 1
#define PIN_LOW  0
#define DEBOUNCE 1
#define DIRECT   0


//- hw specific defines -------------------------------------------------------------------------------------
#if defined __AVR_ATmega168__ || defined __AVR_ATmega168A__ || defined __AVR_ATmega168P__ || defined __AVR_ATmega168PA__ || defined __AVR_ATmega328__ || defined __AVR_ATmega328P__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels

#define _PCINT00 &PCMSK0,&PCICR,PCIE0,PCINT0,DDRB,&PORTB,&PINB,PORTB0
#define _PCINT01 &PCMSK0,&PCICR,PCIE0,PCINT1,DDRB,&PORTB,&PINB,PORTB1
#define _PCINT02 &PCMSK0,&PCICR,PCIE0,PCINT2,DDRB,&PORTB,&PINB,PORTB2
#define _PCINT03 &PCMSK0,&PCICR,PCIE0,PCINT3,DDRB,&PORTB,&PINB,PORTB3
#define _PCINT04 &PCMSK0,&PCICR,PCIE0,PCINT4,DDRB,&PORTB,&PINB,PORTB4
#define _PCINT05 &PCMSK0,&PCICR,PCIE0,PCINT5,DDRB,&PORTB,&PINB,PORTB5
#define _PCINT06 &PCMSK0,&PCICR,PCIE0,PCINT6,DDRB,&PORTB,&PINB,PORTB6
#define _PCINT07 &PCMSK0,&PCICR,PCIE0,PCINT6,DDRB,&PORTB,&PINB,PORTB7

#define _PCINT08 &PCMSK1,&PCICR,PCIE1,PCINT8,DDRC,&PORTC,&PINC,PORTC0
#define _PCINT09 &PCMSK1,&PCICR,PCIE1,PCINT9,DDRC,&PORTC,&PINC,PORTC1
#define _PCINT10 &PCMSK1,&PCICR,PCIE1,PCINT10,DDRC,&PORTC,&PINC,PORTC2
#define _PCINT11 &PCMSK1,&PCICR,PCIE1,PCINT11,DDRC,&PORTC,&PINC,PORTC3
#define _PCINT12 &PCMSK1,&PCICR,PCIE1,PCINT12,DDRC,&PORTC,&PINC,PORTC4
#define _PCINT13 &PCMSK1,&PCICR,PCIE1,PCINT13,DDRC,&PORTC,&PINC,PORTC5
#define _PCINT14 &PCMSK1,&PCICR,PCIE1,PCINT14,DDRC,&PORTC,&PINC,PORTC6
//#define _PCINT15 &PCMSK1,&PCICR,PCIE1,PCINT15,DDRC,&PORTC,&PINC,PORTC7

#define _PCINT16 &PCMSK2,&PCICR,PCIE2,PCINT16,DDRD,&PORTD,&PIND,PORTD0
#define _PCINT17 &PCMSK2,&PCICR,PCIE2,PCINT17,DDRD,&PORTD,&PIND,PORTD1
#define _PCINT18 &PCMSK2,&PCICR,PCIE2,PCINT18,DDRD,&PORTD,&PIND,PORTD2
#define _PCINT19 &PCMSK2,&PCICR,PCIE2,PCINT19,DDRD,&PORTD,&PIND,PORTD3
#define _PCINT20 &PCMSK2,&PCICR,PCIE2,PCINT20,DDRD,&PORTD,&PIND,PORTD4
#define _PCINT21 &PCMSK2,&PCICR,PCIE2,PCINT21,DDRD,&PORTD,&PIND,PORTD5
#define _PCINT22 &PCMSK2,&PCICR,PCIE2,PCINT22,DDRD,&PORTD,&PIND,PORTD6
#define _PCINT23 &PCMSK2,&PCICR,PCIE2,PCINT23,DDRD,&PORTD,&PIND,PORTD7


#elif defined __AVR_ATmega640__ || defined __AVR_ATmega2560__ || defined __AVR_ATmega1280__ || defined __AVR_ATmega1281__ || defined __AVR_ATmega2561__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels


#elif defined __AVR_ATmega32U4__ || defined __AVR_ATmega16U4__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels
#define PCINT06 &PCMSK0,&PCICR,PCIE0,PCINT6,DDRB,&PORTB,&PINB,PORTB6


#elif defined __AVR_ATmega1284P__ || defined __AVR_ATmega1284__ || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels


#elif defined __AVR_ATtiny24__  || defined __AVR_ATtiny24A__ || defined __AVR_ATtiny44__ || defined __AVR_ATtiny44A__ || defined __AVR_ATtiny84__ || defined  __AVR_ATtiny84A_
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels


#elif defined __AVR_ATtiny25__ || defined  __AVR_ATtiny45__ || defined  __AVR_ATtiny85__
#define PCINT_PCIE_SIZE 3																		// amount of Pin Change Interrupt Enable channels


#endif


//- some macros for port communication ----------------------------------------------------------------------
#define pinInput(DDR,PIN)     (DDR   &= ~_BV(PIN))
#define setPinHigh(PORT,PIN)  (*PORT |=  _BV(PIN))
#define setPinLow(PORT,PIN)   (*PORT &= ~_BV(PIN))
#define regPCIE(ICR,PORT)     (*ICR  |= _BV(PORT))
#define regPCINT(MASK,PORT)   (*MASK |= _BV(PORT))


//- include and typedef for delegate ------------------------------------------------------------------------ 
#include "Fastdelegate.h"																		// include the fastdelegate library

using namespace fastdelegate;
typedef FastDelegate2<uint8_t, uint8_t> s_pcint_dlgt;											// void function_name(uint8_t pcint_pin, uint8_t pcint_mode)


/**
* @brief Structure to handle the registered interrupt requests
*
* @param pin        Stores the interrupt pin
* @param debounce   Debounce flag, callback will be handled differently
* @param active     Flag is set while interrupt was detected and we need to debounce
* @param ie         Interrupt vector, needed for calculation of the pin change interrupt number
* @param time       Here we store the time stamp for debouncing functionallity
* @param dlgt       Delegate, holds the callback information
*/
struct s_pcint_table {
	uint8_t pin;	
	uint8_t debounce;	
	uint8_t active;	
	uint8_t ie;		
	uint32_t time;	
	s_pcint_dlgt dlgt;
};	
extern s_pcint_table pcint_table[];


/**
* @brief Structure to handle information raised by the interrupt function
*
* @param *PINR  Pointer to PIN register, to read the PIN status
* @param prev   To remember on the previus status of the port, to identify which PIN was raising the interrupt 
* @param mask   Mask byte to clean out bits which are not registered for interrupt detection
*/
struct  s_pcint_vector_byte {
	volatile uint8_t *PINR;																		// pointer to the port where pin status can be read
	uint8_t prev;
	uint8_t mask;
};
extern volatile s_pcint_vector_byte pcint_vector_byte[];										// size of the table depending on avr type in the cpp file


/**
* @brief Empty the structs while program start
*
* @param nothing
*/
void    pcint_init(void);


/**
* @brief Registers an pin change interrupt in an array and set the respective PIN as input
* 
* Within this function we register all necassary interrupts in one table, set the PINS as input
* and hold the definition of some registers for further processing.
* Majority of parameters can be skipped easily while using the macros defined in the HW section above.
*
* The defined macros cover all hw specific variables till the config flag
* _PCINT00  ( &PCMSK0,&PCICR,PCIE0,PCINT0,DDRB,&PORTB,&PINB,PORTB0 )
*
* @param *pcint_MASK      Pin Change Mask Register, related to the port, PCMSK0 for PCINT[7:0], etc...
* @param *pcint_ICR       Pin Change Interrupt Control Register
* @param pcint_PCIE       Pin Change Interrupt Enable Bit, related to the port
* @param pcint_PCIPIN     Pin Change Interrupt PIN, defined as number value
* @param pcint_DDR	      Data Direction Register, to set pins as input or output
* @param *pcint_PORT      Port Data Register, to set pins low or high
* @param *pcint_PINR      Port Input Pins Register, here we can read the status of the pins
* @param  pcint_PIN	      Port Pin, defined as number value
* @param  bool_Config     Defines if the input pin should be default high (1) or low (0)
* @param  bool_Debounce   Debounce flag, (0) no debounce, (1) debounce with the time specified in PCINT_DEBOUNCE_TIME
* @param  pcint_CallBack  Callback address for function or class member function, needs an & upfront the function name
*/
uint8_t pcinit_register(volatile uint8_t *pcint_MASK, volatile uint8_t *pcint_ICR, uint8_t pcint_PCIE, uint8_t pcint_PCIPIN, uint8_t pcint_DDR, volatile uint8_t *pcint_PORT, volatile uint8_t *pcint_PINR, uint8_t pcint_PIN, uint8_t bool_Config, uint8_t bool_Debounce, s_pcint_dlgt pcint_CallBack);


/**
* @brief Poll function for debouncing, should be called regulary
*
* @param nothing
*/
void    pcint_poll(void);


/**
* @brief Handler for all interrupt messages
*
* Within this function we identify the port pin which was raing the interrupt, 
* further on we identify if it was the raising or falling edge. Callback will be called
* out of this function while debounce is not neccassary. If debounce is asked, we prepare the 
* respective information only.
*
* @param vector  Hands over the information on which port the interrupt was raised
*/
void    pcint_process(uint8_t vector);




#endif

