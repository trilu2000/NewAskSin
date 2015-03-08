#define power_serial_enable()   power_usb_enable(); pinOutput(DDRB, PINB0);		// pin output, otherwise USB will not work
#define UCSR                    UCSR1B
#define RXEN                    RXEN1

//- cc1100 hardware definitions ---------------------------------------------------------------------------------------------
#define SPI_PORT                PORTB											// SPI port definition
#define SPI_DDR                 DDRB
#define SPI_MISO                PORTB3
#define SPI_MOSI                PORTB2
#define SPI_SCLK                PORTB1
//- -------------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()         uint8_t xPrr0 = PRR0; uint8_t xPrr1 = PRR1; PRR0 = PRR1= 0xFF;
#define recoverPwrRegs()        PRR0 = xPrr0; PRR1 = xPrr1;
#define offBrownOut()           //MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);
//- -------------------------------------------------------------------------------------------------------------------------


//- adc definitions ---------------------------------------------------------------------------------------------------------
#define AVR_BANDGAP_VOLTAGE     1100UL											// band gap reference for Atmega328p
