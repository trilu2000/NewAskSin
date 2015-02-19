#define power_serial_enable()   power_usb_enable(); pinOutput(DDRB, PINB0);		// pin output, otherwise USB will not work
#define UCSR                    UCSR1B
#define RXEN                    RXEN1

//- cc1100 hardware definitions ---------------------------------------------------------------------------------------------
#define SPI_PORT		PORTB															// SPI port definition
#define SPI_DDR			DDRB
#define SPI_MISO		PORTB3
#define SPI_MOSI		PORTB2
#define SPI_SCLK        PORTB1

#define CC_CS_PORT		PORTB															// SPI chip select definition
#define CC_CS_DDR		DDRB
#define CC_CS_PIN		PORTB4

#define CC_GDO0_DDR     DDRB															// GDO0 pin, signals received data
#define CC_GDO0_PORT    PINB
#define CC_GDO0_PIN     PORTB5

#define CC_GDO0_PCICR   PCICR															// GDO0 interrupt register
#define CC_GDO0_PCIE    PCIE0
#define CC_GDO0_PCMSK   PCMSK0															// GDO0 interrupt mask
#define CC_GDO0_INT     PCINT5															// pin interrupt
//- -----------------------------------------------------------------------------------------------------------------------


//- power management definitions --------------------------------------------------------------------------------------------
#define backupPwrRegs()	 uint8_t xPrr0 = PRR0; uint8_t xPrr1 = PRR1; PRR0 = PRR1= 0xFF;
#define recoverPwrRegs() PRR0 = xPrr0; PRR1 = xPrr1;
#define offBrownOut()
//- -----------------------------------------------------------------------------------------------------------------------


//- adc definitions --------------------------------------------------------------------------------------------
#define AVR_BANDGAP_VOLTAGE   1100UL											// band gap reference for Atmega328p
