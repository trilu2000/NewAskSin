//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Hardware abstraction layer --------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#include "HAL.h"


//- eeprom functions ------------------------------------------------------------------------------------------------------
void getEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_read_block((void*)ptr,(const void*)addr,len);
}
void setEEPromBlock(uint16_t addr,uint8_t len,void *ptr) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_write_block((const void*)ptr,(void*)addr,len);
}
void clearEEPromBlock(uint16_t addr, uint16_t len) {
	uint8_t tB=0;
	for (uint16_t l = 0; l < len; l++) {												// step through the bytes of eeprom
		setEEPromBlock(addr+l,1,(void*)&tB);
	}
}