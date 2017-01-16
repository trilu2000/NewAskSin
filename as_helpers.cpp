
/**
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin helper functions -----------------------------------------------------------------------------------------------
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "newasksin.h"



//- asksin related helpers ------------------------------------------------------------------------------------------------
uint32_t byteTimeCvt(uint8_t tTime) {
	const uint16_t c[8] = { 1,10,50,100,600,3000,6000,36000 };
	return (uint32_t)(tTime & 0x1F) * c[tTime >> 5] * 100;
}

uint32_t intTimeCvt(uint16_t iTime) {
	if (iTime == 0) return 0;

	// take care of the byte order
	#define LIT_ENDIAN ((1 >> 1 == 0) ? 1 : 0)
	#if LIT_ENDIAN
	iTime = (iTime >> 8) | (iTime << 8);
	#endif

	// process the conversation
	uint8_t tByte;
	if ((iTime & 0x1F) != 0) {
		tByte = 2;
		for (uint8_t i = 1; i < (iTime & 0x1F); i++) tByte *= 2;
	}
	else tByte = 1;
	return (uint32_t)tByte*(iTime >> 5) * 100;
}

/*
* @brief Decode the incoming messages
*        Note: this is no encryption!
*
* @param buf   pointer to buffer
*/
void hm_decode(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i = 2; i < buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xDC) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}

/*
* @brief Encode the outgoing messages
*        Note: this is no encryption!
*
* @param buf   pointer to buffer
*/
void hm_encode(uint8_t *buf) {
	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i = 2; i < buf[0]; i++) {
		prev = (prev + 0xDC) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}


//- byte array related helpers --------------------------------------------------------------------------------------------
uint8_t  isEmpty(void *ptr, uint8_t len) {
	while (len > 0) {
		len--;
		if (*((uint8_t*)ptr + len)) return 0;
	}
	return 1;
}

uint8_t isEqual(void *p1, void *p2, uint8_t len) {
	return memcmp(p1, p2, len) ? 0 : 1;
}


