#ifndef _HAL_ATMEGA_32U4_H
#define _HAL_ATMEGA_32U4_H

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>


//- debug specific --------------------------------------------------------------------------------------------------------
#define power_debug_enable()    power_usb_enable(); power_usart0_enable(); power_timer0_enable(); power_usb_enable();
//- -------------------------------------------------------------------------------------------------------------------------


//- timer definitions -----------------------------------------------------------------------------------------------------
#define hasTimer0																			// timer0 should be enough, while timer1 and 3 are for PWM mode
//#define hasTimer1
//#define hasTimer3
extern volatile uint8_t timer;																// here we store the active timer

static void init_millis_timer0(int16_t correct_ms) {
	timer = 0;
	power_timer0_enable();

	TCCR0A = _BV(WGM01);																	// CTC mode
	TCCR0B = (_BV(CS01) | _BV(CS00));														// prescaler 64; 8.000.000 / 64 = 125.000 / 1000 = 125 
	TIMSK0 = _BV(OCIE0A);
	OCR0A = ((F_CPU / 64) / 1000) + correct_ms;
}
//- -------------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()         uint8_t xPrr0 = PRR0; uint8_t xPrr1 = PRR1; PRR0 = PRR1= 0xFF;
#define recoverPwrRegs()        PRR0 = xPrr0; PRR1 = xPrr1;
#define offBrownOut()           //MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
const uint8_t admux_internal = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);				// internal ADMUX register setup
const uint8_t admux_external = _BV(REFS1) | _BV(REFS0);										// | measurement pin
const uint16_t ref_v_external = 1100;														// internal reference voltage in 10mv
//- -------------------------------------------------------------------------------------------------------------------------


//- pin definition ----------------------------------------------------------------------------------------------------------
#define pc_interrupt_vectors 1																// amount of pin change interrupt vectors

#define pinD0 (3)		// pinD0, SCL, PWM, INT0
#define pinD1 (2)		// pinD1, SDA, INT1
#define pinD2 (0)		// pinD2, UART-RX, INT2
#define pinD3 (1)		// pinD3, UART-TX, INT3
#define pinD4 (4)		// pinD4, 
#define pinD6 (12)		// pinD6, PWM, ADC9
#define pinD7 (6)		// pinD7, PWM, ADC10

#define pinC6 (5)		// pinC5, PWM
#define pinC7 (11)		// pinC7, PWM

#define pinB0 (17)		// pinB0, RXLED, SS, PCINT0
#define pinB1 (15)		// pinB1, SCK, PCINT1
#define pinB2 (16)		// pinB2, MOSI, PCINT2
#define pinB3 (14)		// pinB3, MISO, PCINT3
#define pinB4 (8)		// pinB4, Chipselect
#define pinB5 (9)		// pinB5, GDO0
#define pinB6 (10)		// pinB6, PWM, PCINT6, ADC13
#define pinB7 (11)		// pinB7, PWM, PCINT7, UART-RTS

#define pinE6 (7)		// pinE6, GDO2

#define pinF0 (23)		// pinF0, ADC0
#define pinF4 (21)		// pinF4, ADC4
#define pinF5 (20)		// pinF5, ADC5
#define pinF6 (19)		// pinF6, ADC6
#define pinF7 (18)		// pinF7, ADC7



#endif