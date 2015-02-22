#include <util/delay.h>

//- load user modules -----------------------------------------------------------------------------------------------------
#include <Wire.h>																			// library to communicate with i2c sensor
#define I2C_ADDR     0x39
#define REG_CONTROL  0x00
#define REG_CONFIG   0x01
#define REG_DATALOW  0x04
#define REG_DATAHIGH 0x05
#define REG_ID       0x0A
static uint8_t M = 0;

uint8_t thVal = 0;																			// variable which holds the measured value


void initTH1();
void measureTH1();
static void     initTSL(void);
static uint16_t readTSL(void);

