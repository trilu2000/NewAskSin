/*  This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2008  Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef AES_H_
#define AES_H_

#include <stdint.h>


typedef struct {
	uint8_t ks[16];
} aes_roundkey_t;
typedef struct {
	aes_roundkey_t key[10 + 1];
} aes128_ctx_t;
typedef struct {
	aes_roundkey_t key[12 + 1];
} aes192_ctx_t;
typedef struct {
	aes_roundkey_t key[14 + 1];
} aes256_ctx_t;
typedef struct {
	aes_roundkey_t key[1];																	// just to avoid the warning 
} aes_genctx_t;
typedef struct {
	uint8_t s[16];
} aes_cipher_state_t;


/**
* \brief initialize the keyschedule
*
* This function computes the keyschedule from a given key with a given length
* and stores it in the context variable
* \param key       pointer to the key material
* \param keysize_b length of the key in bits (valid are 128, 192 and 256)
* \param ctx       pointer to the context where the keyschedule should be stored
*/
void aes_init(const void* key, uint16_t keysize_b, aes_genctx_t* ctx);

/**
* \brief encrypt with 128 bit key.
*
* This function encrypts one block with the AES algorithm under control of
* a keyschedule produced from a 128 bit key.
* \param buffer pointer to the block to decrypt
* \param ctx    pointer to the key schedule
*/
void aes128_enc(void* buffer, aes128_ctx_t* ctx);

/**
* \brief decrypt with 128 bit key.
*
* This function decrypts one block with the AES algorithm under control of
* a keyschedule produced from a 128 bit key.
* \param buffer pointer to the block to decrypt
* \param ctx    pointer to the key schedule
*/
void aes128_dec(void* buffer, aes128_ctx_t* ctx);


// - non public ----------------------------------------------------------------------------

// - encrypt
void aes_shiftcol(void* data, uint8_t shift);
static void aes_enc_round(aes_cipher_state_t* state, const aes_roundkey_t* k);
static void aes_enc_lastround(aes_cipher_state_t* state, const aes_roundkey_t* k);
void aes_encrypt_core(aes_cipher_state_t* state, const aes_genctx_t* ks, uint8_t rounds);
void aes_encrypt_core(aes_cipher_state_t* state, const aes_genctx_t* ks, uint8_t rounds);

// - decrypt 
void aes_invshiftrow(void* data, uint8_t shift);
void aes_invshiftcol(void* data, uint8_t shift);
static void aes_dec_round(aes_cipher_state_t* state, const aes_roundkey_t* k);
static void aes_dec_firstround(aes_cipher_state_t* state, const aes_roundkey_t* k);
void aes_decrypt_core(aes_cipher_state_t* state, const aes_genctx_t* ks, uint8_t rounds);

// - init
void aes128_init(const void* key, aes128_ctx_t* ctx);
void aes192_init(const void* key, aes192_ctx_t* ctx);
void aes256_init(const void* key, aes256_ctx_t* ctx);
static void aes_rotword(void* a);

uint8_t gf256mul(uint8_t a, uint8_t b, uint8_t reducer);


#endif

