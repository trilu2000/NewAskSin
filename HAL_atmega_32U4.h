#define power_serial_enable()   power_usb_enable(); pinOutput(DDRB, PINB0);		// pin output, otherwise USB will not work
#define UCSR                    UCSR1B
#define RXEN                    RXEN1


//- timer definitions -----------------------------------------------------------------------------------------------------
#define hasTimer0
#define hasTimer1
//- -------------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()         uint8_t xPrr0 = PRR0; uint8_t xPrr1 = PRR1; PRR0 = PRR1= 0xFF;
#define recoverPwrRegs()        PRR0 = xPrr0; PRR1 = xPrr1;
#define offBrownOut()           //MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
const uint8_t admux_internal = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);					// internal ADMUX register setup
const uint8_t admux_external = _BV(REFS1) | _BV(REFS0);											// | measurement pin
const uint16_t ref_v_external = 1100;															// internal reference voltage in 10mv
//- -------------------------------------------------------------------------------------------------------------------------


//- pin definition ----------------------------------------------------------------------------------------------------------
#define PCINT_PCIE_SIZE        1																// amount of pin change interrupt vectors

const s_pin_def pin_B0 = { PORTB0, &DDRB, &PORTB, &PINB,  0,  PCINT0, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B1 = { PORTB1, &DDRB, &PORTB, &PINB,  1,  PCINT1, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B2 = { PORTB2, &DDRB, &PORTB, &PINB,  2,  PCINT2, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B3 = { PORTB3, &DDRB, &PORTB, &PINB,  3,  PCINT3, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B4 = { PORTB4, &DDRB, &PORTB, &PINB,  4,  PCINT4, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B5 = { PORTB5, &DDRB, &PORTB, &PINB,  5,  PCINT5, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B6 = { PORTB6, &DDRB, &PORTB, &PINB,  6,  PCINT6, &PCICR, &PCMSK0, PCIE0, 0 };
const s_pin_def pin_B7 = { PORTB7, &DDRB, &PORTB, &PINB,  7,  PCINT7, &PCICR, &PCMSK0, PCIE0, 0 };

const s_pin_def pin_D0 = { PORTD0, &DDRD, &PORTD, &PIND, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D1 = { PORTD1, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D2 = { PORTD2, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D3 = { PORTD3, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D4 = { PORTD4, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D5 = { PORTD5, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D6 = { PORTD6, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_D7 = { PORTD7, &DDRD, &PORTC, &PINC, NULL, NULL, NULL, NULL, NULL, NULL };

const s_pin_def pin_F0 = { PORTF0, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_F1 = { PORTF1, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
//const s_pin_def pin_F2 = { PORTF2, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
//const s_pin_def pin_F3 = { PORTF3, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_F4 = { PORTF4, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_F5 = { PORTF5, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_F6 = { PORTF6, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
const s_pin_def pin_F7 = { PORTF7, &DDRF, &PORTF, &PINF, NULL, NULL, NULL, NULL, NULL, NULL };
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
/*#define AVR_BANDGAP_VOLTAGE     1100UL											// band gap reference for Atmega328p


//- pin definition ----------------------------------------------------------------------------------------------------------
#define PCINT_PCIE_SIZE        1																// amount of pin change interrupt vectors

#define PIN_B0   PORTB0, &DDRB, &PORTB, &PINB,  0,  PCINT0, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B1   PORTB1, &DDRB, &PORTB, &PINB,  1,  PCINT1, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B2   PORTB2, &DDRB, &PORTB, &PINB,  2,  PCINT2, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B3   PORTB3, &DDRB, &PORTB, &PINB,  3,  PCINT3, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B4   PORTB4, &DDRB, &PORTB, &PINB,  4,  PCINT4, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B5   PORTB5, &DDRB, &PORTB, &PINB,  5,  PCINT5, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B6   PORTB6, &DDRB, &PORTB, &PINB,  6,  PCINT6, &PCICR, &PCMSK0, PCIE0,0 
#define PIN_B7   PORTB7, &DDRB, &PORTB, &PINB,  7,  PCINT7, &PCICR, &PCMSK0, PCIE0,0 

#define PIN_C0   PORTC0, &DDRC, &PORTC, &PINC,,,,,,
#define PIN_C1   PORTC1, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C2   PORTC2, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C3   PORTC3, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C4   PORTC4, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C5   PORTC5, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C6   PORTC6, &DDRC, &PORTC, &PINC,,,,,, 
#define PIN_C7   PORTC7, &DDRC, &PORTC, &PINC,,,,,, 

#define PIN_D0   PORTD0, &DDRD, &PORTD, &PIND,,,,,,
#define PIN_D1   PORTD1, &DDRD, &PORTD, &PIND,,,,,, 
#define PIN_D2   PORTD2, &DDRD, &PORTD, &PIND,,,,,,
#define PIN_D3   PORTD3, &DDRD, &PORTD, &PIND,,,,,, 
#define PIN_D4   PORTD4, &DDRD, &PORTD, &PIND,,,,,, 
#define PIN_D5   PORTD5, &DDRD, &PORTD, &PIND,,,,,,
#define PIN_D6   PORTD6, &DDRD, &PORTD, &PIND,,,,,, 
#define PIN_D7   PORTD7, &DDRD, &PORTD, &PIND,,,,,, 

#define PIN_E0   PORTE0, &DDRE, &PORTE, &PINE,,,,,, 
#define PIN_E1   PORTE1, &DDRE, &PORTE, &PINE,,,,,,
#define PIN_E2   PORTE2, &DDRE, &PORTE, &PINE,,,,,,
#define PIN_E3   PORTE3, &DDRE, &PORTE, &PINE,,,,,, 
#define PIN_E4   PORTE4, &DDRE, &PORTE, &PINE,,,,,, 
#define PIN_E5   PORTE5, &DDRE, &PORTE, &PINE,,,,,, 
#define PIN_E6   PORTE6, &DDRE, &PORTE, &PINE,,,,,, 
#define PIN_E7   PORTE7, &DDRE, &PORTE, &PINE,,,,,, 

#define PIN_F0   PORTF0, &DDRF, &PORTF, &PINF,,,,,,
#define PIN_F1   PORTF1, &DDRF, &PORTF, &PINF,,,,,, 
#define PIN_F2   PORTF2, &DDRF, &PORTF, &PINF,,,,,,
#define PIN_F3   PORTF3, &DDRF, &PORTF, &PINF,,,,,, 
#define PIN_F4   PORTF4, &DDRF, &PORTF, &PINF,,,,,, 
#define PIN_F5   PORTF5, &DDRF, &PORTF, &PINF,,,,,, 
#define PIN_F6   PORTF6, &DDRF, &PORTF, &PINF,,,,,, 
#define PIN_F7   PORTF7, &DDRF, &PORTF, &PINF,,,,,, */


