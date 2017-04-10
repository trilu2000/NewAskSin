
#ifndef _HAL_ATMEGA_328_H
#define _HAL_ATMEGA_328_H


#define power_serial_enable()   power_usart0_enable();
#define UCSR                    UCSR0B
#define RXEN                    RXEN0

//- timer definitions -----------------------------------------------------------------------------------------------------
#define hasTimer0
#define hasTimer1
#define hasTimer2
//- -------------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()         uint8_t xPrr = PRR; PRR = 0xFF;
#define recoverPwrRegs()        PRR = xPrr;
#define offBrownOut()           MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
const uint8_t admux_internal = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);					// internal ADMUX register setup
const uint8_t admux_external = _BV(REFS1) | _BV(REFS0);											// | measurement pin
const uint16_t ref_v_external = 1100;															// internal reference voltage in 10mv
//- -------------------------------------------------------------------------------------------------------------------------


//- pin definition ----------------------------------------------------------------------------------------------------------
#define pci_int_size 3

#define pinD0 (0)		// &DDRD, &PORTD, &PIND, 16, PCINT16, &PCICR, &PCMSK2, PCIE2, 2
#define pinD1 (1)
#define pinD2 (2)
#define pinD3 (3)
#define pinD4 (4)
#define pinD5 (5)
#define pinD6 (6)
#define pinD7 (7)

#define pinB0 (8)		// &DDRB, &PORTB, &PINB,  0,  PCINT0, &PCICR, &PCMSK0, PCIE0, 0
#define pinB1 (9)
#define pinB2 (10)
#define pinB3 (11)
#define pinB4 (12)
#define pinB5 (13)

#define pinC0 (14)
#define pinC1 (15)
#define pinC2 (16)
#define pinC3 (17)
#define pinC4 (18) 
#define pinC5 (19)
#define pinC6 (20)
#define pinC7 (21)


//- -------------------------------------------------------------------------------------------------------------------------



#endif