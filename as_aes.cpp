/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin AES class ------------------------------------------------------------------------------------------------------
*   special thank you to https://git.zerfleddert.de/hmcfgusb/AES/ for bidcos(R) AES explanation
* - -----------------------------------------------------------------------------------------------------------------------
*/

#include "as_aes.h"


/* AES main class definition */
AES::AES() {
}
void AES::prep_AES_REQ(uint8_t *hmkey, uint8_t *rcv_buf, uint8_t *snd_buf) {
}
void AES::prep_AES_REPLY(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *challenge, uint8_t *snd_buf) {
}
void AES::check_AES_REPLY(uint8_t *hmkey, uint8_t *rcv_buf) {
}
uint8_t AES::check_SEND_AES_TO_ACTOR(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *rcv_buf) {
}



/* NO_AES functions to overwrite AES main class */
NO_AES::NO_AES() : AES() {
}



/* HAS_AES functions to overwrite AES main class */
HAS_AES::HAS_AES() : AES() {
}
void HAS_AES::prep_AES_REQ(uint8_t *hmkey, uint8_t *rcv_buf, uint8_t *snd_buf) {
	/* save the initial message for later use */
	memcpy(prev_buf, rcv_buf, rcv_buf[0] + 1);				// we store the initial message
	active = MSG_AES::AES_REQ;								// set the flag that something is in the buffer

	/* here we make a temporarily key with the challenge and the HMKEY, as we need this for later signature verification */
	get_random(snd_buf + 11);								// six random bytes to the payload
	make_temp_hmkey(hmkey, snd_buf + 11);					// generate a temp key
}

void HAS_AES::prep_AES_REPLY(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *challenge, uint8_t *snd_buf) {
	//dbg << "key: " << _HEX(hmkey, 16) << ", idx: " << _HEXB(hmkey_index[0]) << ", challenge: " << _HEX(challenge, 7) << ", snd_buf: " << _HEX(snd_buf, snd_buf[0] + 1) << '\n';
	/* we need a key to encrypt */
	make_temp_hmkey(hmkey, challenge);						// build the temporarily key from challenge

	/* the iv and a message to encrypt */
	get_random(prev_buf);									// fill the first 6 byte with random
	memcpy(prev_buf + 6, snd_buf + 1, 10);					// copy the message to sign, without the length byte

	/* encrypt and copy back the payload */
	aes128_enc(prev_buf, &ctx);								// encrypt the message first time
	for (uint8_t i = 0; i < 16; i++)
		prev_buf[i] ^= iv[i];								// xor encrypted payload with iv
	aes128_enc(prev_buf, &ctx);								// encrypt payload again
}

void HAS_AES::check_AES_REPLY(uint8_t *hmkey, uint8_t *rcv_buf) {
	/* decrypt it and check if the content compares to the last received message */
	prep_iv(prev_buf); 										// some cleanup and preparation of iv variable
	aes128_dec(rcv_buf + 10, &ctx);							// decrypt payload with temporarily key first time

	for (uint8_t i = 0; i < 16; i++)
		rcv_buf[i + 10] ^= iv[i];							// xor encrypted payload with iv

	memcpy(ACK_payload, rcv_buf + 10, 4);					// and copy the payload
	aes128_dec(rcv_buf + 10, &ctx);							// decrypt payload with temporarily key again
	//dbg << F("HMKEY: ") << _HEX(hmkey, 16) << F(", initial: ") << _HEX(prev_buf + 1, 10) << F(", reply: ") << _HEX(rcv_buf + 10, 16) << '\n';

	/* compare decrypted message with original message, memcmp returns 0 if compare true, we send an ACK_AES and
	*  process the original message, or terminate the communication */
	if (!memcmp(rcv_buf + 16, prev_buf + 1, 10)) {			// compare bytes 7-17 (first 9 byte are flags and addresses) of decrypted data with bytes 2-12 of msgOriginal
		memcpy(rcv_buf, prev_buf, prev_buf[0] + 1);			// restore the saved message to be processed
		active = MSG_AES::AES_REPLY_OK;
	} else {
		rcv_buf[0] = 0;										// nothing to do any more
		active = MSG_AES::NONE;
	}
}

uint8_t HAS_AES::check_SEND_AES_TO_ACTOR(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *rcv_buf) {
	/* a new hmkey will be delivered in two messages, identifikation is based on the hmkey index in byte 11, the first 8 byte of
	* the key are flagged by an even index, the next 8 byte by an odd index. the key index we have to remember has to be divided by 2 */
	aes128_init(hmkey, &ctx);								// load HMKEY
	aes128_dec(rcv_buf + 10, &ctx);							// decrypt payload width HMKEY first time

	if (rcv_buf[10] != 0x01) return 0;						// byte 10 needs to be 0x01 
	uint8_t slice = rcv_buf[11] & 1;						// we are here while it was a valid decrypt, now analyze if it is the first, or second part of the new hmkey 
	memcpy(new_hmkey + (slice * 8), &rcv_buf[12], 8);		// copy content of the message in our temp hm key
	new_hmkey_index[0] = rcv_buf[11] & 0xFE;				// calculate the index variable

	if (slice) return 1;									// and if complete, signalize it
	else return 0;
}

void HAS_AES::make_temp_hmkey(uint8_t *hmkey, uint8_t *challenge) {
	memcpy(temp_hmkey, hmkey, 16);
	for (uint8_t i = 0; i < 6; i++) temp_hmkey[i] ^= challenge[i];
	aes128_init(temp_hmkey, &ctx);
}

void HAS_AES::prep_iv(uint8_t *buf) {
	memset(iv, 0x00, 16);
	memcpy(iv, buf + 11, buf[0] - 10);						// copy payload of initial message into IV
}

