#define power_serial_enable()   power_usart0_enable();
#define UCSR                    UCSR0B
#define RXEN                    RXEN0

//- cc1100 hardware definitions ---------------------------------------------------------------------------------------------
#define SPI_PORT		PORTB															// SPI port definition
#define SPI_DDR			DDRB
#define SPI_MISO		PORTB4
#define SPI_MOSI		PORTB3
#define SPI_SCLK        PORTB5

#define CC_CS_PORT		PORTB															// SPI chip select definition
#define CC_CS_DDR		DDRB
#define CC_CS_PIN		PORTB2

#define CC_GDO0_DDR     DDRD															// GDO0 pin, signals received data
#define CC_GDO0_PORT    PIND
#define CC_GDO0_PIN     PORTD2

#define CC_GDO0_PCICR   PCICR															// GDO0 interrupt register
#define CC_GDO0_PCIE    PCIE2
#define CC_GDO0_PCMSK   PCMSK2															// GDO0 interrupt mask
#define CC_GDO0_INT     PCINT18															// pin interrupt
//- -----------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()  uint8_t xPrr = PRR; PRR = 0xFF;
#define recoverPwrRegs() PRR = xPrr;
#define offBrownOut()    MCUCR = (1<<BODS)|(1<<BODSE); MCUCR = (1<<BODS);																		// must be done right before sleep
//- -----------------------------------------------------------------------------------------------------------------------


//- adc definitions --------------------------------------------------------------------------------------------
#define AVR_BANDGAP_VOLTAGE   1100UL											// band gap reference for Atmega328p
