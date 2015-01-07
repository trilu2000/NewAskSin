//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin status led functions -------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _LD_H
#define _LD_H

#include "HAL.h"


struct s_blinkPattern {							// struct for defining the blink pattern
	uint8_t len     :3;							// length of pattern string
	uint8_t dur     :3;							// how often the pattern has to be repeated, 0 for endless
	uint8_t led0    :1;							// red
	uint8_t led1    :1;							// green, if you like orange, set led0 and led1 to one
	uint8_t pat[6];								// the pattern it self, pattern starts always with the on time, followed by off time.
};												// time is given in 10ms steps

enum ledStat {nothing, pairing, pair_suc, pair_err, send, ack, noack, bat_low, defect, welcome, key_long};

// we need two type of blink patterns, one with only one led and a second one with a bi color led
	const struct s_blinkPattern sPairing[2] = {	// 1; define pairing string
		{2, 0, 1, 0, {50, 50,} },
		{2, 0, 1, 1, {50, 50,} },
	};
	const struct s_blinkPattern sPair_suc[2] = {	// 2; define pairing success
		{2, 1, 1, 0, {200, 0,} },
		{2, 1, 0, 1, {200, 0,} },
	};
	const struct s_blinkPattern sPair_err[2] = {	// 3; define pairing error
		{2, 3, 1, 0, {5, 10,} },
		{2, 1, 1, 0, {200, 0,} },
	};
	const struct s_blinkPattern sSend[2] = {		// 4; define send indicator
		{2, 1, 1, 0, {5, 1,} },
		{2, 1, 1, 1, {5, 1,} },
	};
	const struct s_blinkPattern sAck[2] = {			// 5; define ack indicator
		{0, 0, 0, 0, {0, 0,} },
		{2, 1, 0, 1, {5, 1,} },
	};
	const struct s_blinkPattern sNoack[2] = {		// 6; define no ack indicator
		{0, 0, 0, 0, {0, 0,} },
		{2, 1, 1, 0, {10, 1,} },
	};
	const struct s_blinkPattern sBattLow[2] = {		// 7; define battery low indicator
		{6, 3, 1, 0, {50, 10, 10, 10, 10, 100} },
		{6, 3, 1, 0, {50, 10, 10, 10, 10 ,100} },
	};
	const struct s_blinkPattern sDefect[2] = {		// 8; define defect indicator
		{6, 3, 1, 0, {10, 10, 10, 10, 10, 100} },
		{6, 3, 1, 0, {10, 10, 10, 10, 10, 100} },
	};
	const struct s_blinkPattern sWelcome[2] = {		// 9; define welcome indicator
		{6, 1, 1, 0, {10, 10, 50, 10, 50, 100} },
		{6, 1, 0, 1, {10, 10, 50, 10, 50, 100} },
	};
	const struct s_blinkPattern sKeyLong[2] = {		// 10; key long indicator
		{2, 0, 1, 0, {20, 20, } },
		{2, 0, 1, 0, {20, 20, } },
	};



class LD {
	friend class AS;
  
  public:		//---------------------------------------------------------------------------------------------------------
	uint8_t active     :1;						// something active

  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	
	class AS *pHM;								// pointer to main class for function calls

	const struct s_blinkPattern *blinkPtr;		// pointer to blink struct
	
	uint8_t bLeds      :2;						// 1 or 2 leds
	uint8_t lCnt       :3;						// counter for positioning inside of blink string
	uint8_t dCnt       :3;						// duration counter	
	
  public:		//---------------------------------------------------------------------------------------------------------
	void init(uint8_t leds, AS *ptrMain);
	void set(ledStat stat);
	void blinkRed(void);
	
  protected:	//---------------------------------------------------------------------------------------------------------
  private:		//---------------------------------------------------------------------------------------------------------
	LD();
	void poll(void);

};


#endif
