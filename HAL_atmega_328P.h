#ifndef _HAL_ATMEGA_328_H
#define _HAL_ATMEGA_328_H

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>


//- debug specific --------------------------------------------------------------------------------------------------------
#define power_debug_enable()    power_usart0_enable();
//- -------------------------------------------------------------------------------------------------------------------------


//- timer definitions -----------------------------------------------------------------------------------------------------
#define hasTimer0																			// timer0 should be enough, while timer1 and 3 are for PWM mode
#define hasTimer1
#define hasTimer2
extern volatile uint8_t timer;																// here we store the active timer

static void init_millis_timer0(int16_t correct_ms) {
	timer = 0;
	power_timer0_enable();

	TCCR0A = _BV(WGM01);																	// CTC mode
	TCCR0B = (_BV(CS01) | _BV(CS00));														// prescaler 64; 8.000.000 / 64 = 125.000 / 1000 = 125 
	TIMSK0 = _BV(OCIE0A);
	OCR0A = ((F_CPU / 64) / 1000) + correct_ms;
}
static void init_millis_timer1(int16_t correct_ms) {
	timer = 1;
	power_timer1_enable();

	TCCR1A = 0;
	TCCR1B = (_BV(WGM12) | _BV(CS10) | _BV(CS11));
	TIMSK1 = _BV(OCIE1A);
	OCR1A = ((F_CPU / 64) / 1000) + correct_ms;
}
static void init_millis_timer2(int16_t correct_ms) {
	timer = 2;
	power_timer2_enable();

	TCCR2A = _BV(WGM21);
	TCCR2B = (_BV(CS21) | _BV(CS20));
	TIMSK2 = _BV(OCIE2A);
	OCR2A = ((F_CPU / 32) / 1000) + correct_ms;
}
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
#define pc_interrupt_vectors 3																// amount of pin change interrupt vectors

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