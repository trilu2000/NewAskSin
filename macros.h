//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- collection of macros  -------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _MACROS_H
#define _MACROS_H



//- some macros and definitions -------------------------------------------------------------------------------------------
#define _PGM_BYTE(x) pgm_read_byte(&x)										// short hand for PROGMEM read
#define _PGM_WORD(x) pgm_read_word(&x)
//- -----------------------------------------------------------------------------------------------------------------------



//- bit manipulation macros -----------------------------------------------------------------------------------------------
#define _SET_PIN_OUTPUT(DDR,PIN)      ((*DDR)    |=  _BV(PIN))				// pin functions
#define _SET_PIN_INPUT(DDR,PIN)       ((*DDR)    &= ~_BV(PIN))

#define _SET_PIN_HIGH(PORT,PIN)       ((*PORT)   |=  _BV(PIN))
#define _SET_PIN_LOW(PORT,PIN)        ((*PORT)   &= ~_BV(PIN))
#define _SET_PIN_TOOGLE(PORT,PIN)     ((*PORT)   ^=  _BV(PIN))

#define _GET_PIN_STATUS(PINP,PIN)     ((*PINP)   &   _BV(PIN))

#define _REG_PCI_ICR(ICR,ICPORT)      ((*ICR)    |=  _BV(ICPORT))			// pci functions
#define _REG_PCI_PIN(ICMASK,ICPIN)    ((*ICMASK) |=  _BV(ICPIN))
//- -----------------------------------------------------------------------------------------------------------------------



//- macros to shorten the pin definition ----------------------------------------------------------------------------------
// pin definition is done in the following format
// #define PIN_B0   0,B, 0,,0  // PORTB0,   (DDRB, PORTB, PINB),  (PCINT #0,  PCINT0),   PCICR,         (PCMSK0, PCIE0) 

#define SET_PIN_OUTPUT(...)        _SET_PIN_OUTPUT(_DDR(__VA_ARGS__), _PIN(__VA_ARGS__))   
#define SET_PIN_INPUT(...)         _SET_PIN_INPUT(_DDR(__VA_ARGS__), _PIN(__VA_ARGS__))

#define SET_PIN_HIGH(...)          _SET_PIN_HIGH(_PORT(__VA_ARGS__), _PIN(__VA_ARGS__))
#define SET_PIN_LOW(...)           _SET_PIN_LOW(_PORT(__VA_ARGS__), _PIN(__VA_ARGS__))
#define SET_PIN_TOOGLE(...)        _SET_PIN_TOOGLE(_PORT(__VA_ARGS__), _PIN(__VA_ARGS__))

#define GET_PIN_STATUS(...)        _GET_PIN_STATUS(_PINP(__VA_ARGS__), _PIN(__VA_ARGS__))

#define REG_PCI_ICR(...)           _REG_PCI_ICR(_PCICR(__VA_ARGS__), _PCIE(__VA_ARGS__))
#define REG_PCI_PIN(...)           _REG_PCI_PIN(_PCMSK(__VA_ARGS__), _PCINT(__VA_ARGS__))



/**
* @brief Converts a PIN_BIT shortcut to the asked information
* @param  PAD	   PIN, DDREG, PORTREG, PINREG, PCINR, PCINT, PCICREG, PCIMSK, PCIEREG
* @param  PIN_DEF  Pin definition like PIN_B0
*/
#define EX(PAD, ...) PAD(__VA_ARGS__)

/* break down the EX macro to the single PAD's and replace the answer with the info asked for */
#define _DDR(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)             bDDREG			// DDRA

#define _PORT(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)            bPORTREG			// PORTA
#define _PINP(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)            bPINREG			// PINA
#define _PIN(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)             bPINBIT			// PORTA2 

#define _PCICR(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)           bPCICREG			// PCICR
#define _PCIE(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)            bPCIEREG			// PCIE0
#define _PCMSK(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)           bPCIMASK			// PCMSK0
#define _PCINT(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)           bPCIBYTE			// PCINT7
#define _PCINT_NR(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)        bPCINR			// 7

#define _PCIVEC(bPINBIT, bDDREG, bPORTREG, bPINREG, bPCINR, bPCIBYTE, bPCICREG, bPCIMASK, bPCIEREG, bVEC)          bVEC				// 0

//- -----------------------------------------------------------------------------------------------------------------------



//- some macros for debugging ---------------------------------------------------------------------------------------------
#define dbg Serial

#define _HI_HEX_BITS(x)  char((x>>4)>9?(x>>4)+55:(x>>4)+48)
#define _LO_HEX_BITS(x)  char((x&0xF)>9?(x&0xF)+55:(x&0xF)+48)

//- -----------------------------------------------------------------------------------------------------------------------






#endif
