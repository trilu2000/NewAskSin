/*- -----------------------------------------------------------------------------------------------------------------------
*  AskSin driver implementation
*  2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
* - -----------------------------------------------------------------------------------------------------------------------
* - AskSin AES class ------------------------------------------------------------------------------------------------------
*   special thank you to https://git.zerfleddert.de/hmcfgusb/AES/ for bidcos(R) AES explanation
* - -----------------------------------------------------------------------------------------------------------------------
*/

#ifndef _AES_H
#define _AES_H

#include "00_debug-flag.h"
#include "HAL.h"
#include "aes.h"


namespace MSG_AES {
	enum E : uint8_t { NONE = 0, AES_REQ = 1, AES_REPLY_OK = 2, };
};

#define MaxDataLen 40

/*
* @brief Helper class for all AES relevant variables
* Virtual class to overwrite if AES should be available or not
* defined in register.h as pointer and filled by NO_AES or HAS_AES
*/
class AES {

public:
	MSG_AES::E active;											// MSG_AES:: NONE, AES_REQ
	uint8_t  has_ACK_payload;									// ACK payload flag
	uint8_t  ACK_payload[4];									// and the respective payload

	uint8_t  new_hmkey[16];										// new hmkey for key exchange
	uint8_t  new_hmkey_index[1];								// new hmkey index
	uint8_t  prev_buf[MaxDataLen];								// store to save the previous received string

	AES(void);
	virtual void prep_AES_REQ(uint8_t *hmkey, uint8_t *rcv_buf, uint8_t *snd_buf);
	virtual void prep_AES_REPLY(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *challenge, uint8_t *snd_buf);
	virtual void check_AES_REPLY(uint8_t *hmkey, uint8_t *rcv_buf);
	virtual uint8_t check_SEND_AES_TO_ACTOR(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *rcv_buf);

};



class NO_AES : public AES {

public:
	NO_AES(void);

};



class HAS_AES : public AES {

protected:
	uint8_t  temp_hmkey[16];									// temp hmkey 
	uint8_t  iv[16];
	aes128_ctx_t ctx; 											// the context where the round keys are stored

public:
	HAS_AES(void);
	void prep_AES_REQ(uint8_t *hmkey, uint8_t *rcv_buf, uint8_t *snd_buf);
	void prep_AES_REPLY(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *challenge, uint8_t *snd_buf);
	void check_AES_REPLY(uint8_t *hmkey, uint8_t *rcv_buf);
	uint8_t check_SEND_AES_TO_ACTOR(uint8_t *hmkey, uint8_t *hmkey_index, uint8_t *rcv_buf);

private:
	void make_temp_hmkey(uint8_t *hmkey, uint8_t *challenge);
	void prep_iv(uint8_t *buf);

};




#endif