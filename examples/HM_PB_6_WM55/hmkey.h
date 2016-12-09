#ifndef _HMKEY_h
	#define _HMKEY_h

	/*
	 * This is the sample AES key for your device.
	 * You should change it maybe to the hm-default key
	 */
	#define HM_DEVICE_AES_KEY        0xA4, 0xE3, 0x75, 0xC6, 0xB0, 0x9F, 0xD1, 0x85, 0xF2, 0x7C, 0x4E, 0x96, 0xFC, 0x27, 0x3A, 0xE4
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
