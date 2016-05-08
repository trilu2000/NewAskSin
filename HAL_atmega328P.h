#define power_serial_enable()   power_usart0_enable();
#define UCSR                    UCSR0B
#define RXEN                    RXEN0

//- cc1100 hardware definitions ---------------------------------------------------------------------------------------------
#define SPI_PORT                PORTB											// SPI port definition
#define SPI_DDR                 DDRB
#define SPI_MISO                PORTB4
#define SPI_MOSI                PORTB3
#define SPI_SCLK                PORTB5
//- -------------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()         uint8_t xPrr = PRR; PRR = 0xFF;
#define recoverPwrRegs()        PRR = xPrr;
#define offBrownOut()           MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
#define AVR_BANDGAP_VOLTAGE     1100UL											// band gap reference for Atmega328p


//- pin definition ----------------------------------------------------------------------------------------------------------
//                              Pin Bit,   Port Register,       PCI Byte / Bit,        ICR Register,  Interrupt Register
#define _PIN_B0   0,B, 0,,0  // PORTB0,   (DDRB, PORTB, PINB),  (PCINT #0,  PCINT0),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B1   1,B, 1,,0  // PORTB1,   (DDRB, PORTB, PINB),  (PCINT #1,  PCINT1),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B2   2,B, 2,,0  // PORTB2,   (DDRB, PORTB, PINB),  (PCINT #2,  PCINT2),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B3   3,B, 3,,0  // PORTB3,   (DDRB, PORTB, PINB),  (PCINT #3,  PCINT3),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B4   4,B, 4,,0  // PORTB4,   (DDRB, PORTB, PINB),  (PCINT #4,  PCINT4),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B5   5,B, 5,,0  // PORTB5,   (DDRB, PORTB, PINB),  (PCINT #5,  PCINT5),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B6   6,B, 6,,0  // PORTB6,   (DDRB, PORTB, PINB),  (PCINT #6,  PCINT6),   PCICR,         (PCMSK0, PCIE0) 
#define _PIN_B7   7,B, 7,,0  // PORTB7,   (DDRB, PORTB, PINB),  (PCINT #7,  PCINT7),   PCICR,         (PCMSK0, PCIE0) 

#define _PIN_C0   0,C, 8,,1  // PORTC0,   (DDRC, PORTC, PINC),  (PCINT #8,  PCINT8),   PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C1   1,C, 9,,1  // PORTC1,   (DDRC, PORTC, PINC),  (PCINT #9,  PCINT9),   PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C2   2,C,10,,1  // PORTC2,   (DDRC, PORTC, PINC),  (PCINT #10, PCINT10),  PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C3   3,C,11,,1  // PORTC3,   (DDRC, PORTC, PINC),  (PCINT #11, PCINT11),  PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C4   4,C,12,,1  // PORTC4,   (DDRC, PORTC, PINC),  (PCINT #12, PCINT12),  PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C5   5,C,13,,1  // PORTC5,   (DDRC, PORTC, PINC),  (PCINT #13, PCINT13),  PCICR,         (PCMSK1, PCIE1) 
#define _PIN_C6   6,C,14,,1  // PORTC6,   (DDRC, PORTC, PINC),  (PCINT #14, PCINT14),  PCICR,         (PCMSK1, PCIE1) 
//#define _PIN_C7   7,B,15,,0 // PORTC7,  (DDRC, PORTC, PINC),  (PCINT #15, PCINT15),  PCICR,         (PCMSK1, PCIE1) 

#define _PIN_D0   0,D,16,,2  // PORTD0,   (DDRD, PORTD, PIND),  (PCINT #16, PCINT16),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D1   1,D,17,,2  // PORTD1,   (DDRD, PORTD, PIND),  (PCINT #17, PCINT17),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D2   2,D,18,,2  // PORTD2,   (DDRD, PORTD, PIND),  (PCINT #18, PCINT18),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D3   3,D,19,,2  // PORTD3,   (DDRD, PORTD, PIND),  (PCINT #19, PCINT19),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D4   4,D,20,,2  // PORTD4,   (DDRD, PORTD, PIND),  (PCINT #20, PCINT20),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D5   5,D,21,,2  // PORTD5,   (DDRD, PORTD, PIND),  (PCINT #21, PCINT21),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D6   6,D,22,,2  // PORTD6,   (DDRD, PORTD, PIND),  (PCINT #22, PCINT22),  PCICR,         (PCMSK2, PCIE2) 
#define _PIN_D7   7,D,23,,2  // PORTD7,   (DDRD, PORTD, PIND),  (PCINT #23, PCINT23),  PCICR,         (PCMSK2, PCIE2) 

