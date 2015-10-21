#ifndef _HMKEY_h
	#define _HMKEY_h

	/*
	 * This is the sample AES key for your device.
	 * You should change it maybe to the hm-default key
	 */
	#define HM_DEVICE_AES_KEY        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10

	/*
	 * This is the sample key index for your device.
	 * For the HM-Default-Key the index must be 0x00
	 *
	 * If you would use your private key of your central unit, the key index must be
	 * the same of your active used key in the file keys.
	 *
	 * Pleas note: The key from your central unit must multiply by 2
	 * Example: Current Index = 2 -> HM_DEVICE_AES_KEY_INDEX 0x04
	 */
	#define HM_DEVICE_AES_KEY_INDEX  0x00

#endif
