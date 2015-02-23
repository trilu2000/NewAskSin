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
