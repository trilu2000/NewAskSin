//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin dimmer class ----------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _cmDimmer_H
#define _cmDimmer_H
#define CLASS_NAME cmDimmer

#include "AS.h"
#include "HAL.h"

// default settings for list3 or list4

// 01 00  02 00  03 00  04 32  05 64  06 00  07 FF  08 00  09 FF  0A 01  0B 12  0C 22  0D 23  0E 20  0F 00 
// 10 14  11 C8  12 0A  13 05  14 05  15 00  16 C8  17 0A  18 0A  19 04  1A 04  26 00  27 14  28 52  29 63 
// 81 00  82 00  83 00  84 32  85 64  86 00  87 FF  88 00  89 FF  8A 24  8B 12  8C 22  8D 23  8E 20  8F 00 
// 90 14  91 C8  92 0A  93 05  94 05  95 00  96 C8  97 0A  98 0A  99 04  9A 04  A6 20  A7 14  A8 52  A9 63
const uint8_t peerEven[] =    {		// cnl 2, 4, 6
	// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, - 15 bytes
	// 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, - 15 bytes
	// 0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, - 15 bytes
	// 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E, - 15 bytes

	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x01,0x12,0x22,0x23,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63, 
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x24,0x12,0x22,0x23,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};


// 01 00  02 00  03 00  04 32  05 64  06 00  07 FF  08 00  09 FF  0A 01  0B 44  0C 54  0D 64  0E 20  0F 00 
// 10 14  11 C8  12 0A  13 05  14 05  15 00  16 C8  17 0A  18 0A  19 04  1A 04  26 00  27 14  28 52  29 63 
// 81 00  82 00  83 00  84 32  85 64  86 00  87 0A  88 00  89 FF  8A A5  8B 44  8C 54  8D 64  8E 20  8F 00 
// 90 14  91 C8  92 0A  93 05  94 05  95 00  96 C8  97 0A  98 0A  99 04  9A 04  A6 20  A7 14  A8 52  A9 63
const uint8_t peerOdd[] =   {		// cnl 1, 3, 5
	// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, - 15 bytes
	// 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, - 15 bytes
	// 0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, - 15 bytes
	// 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E, - 15 bytes

	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x01,0x44,0x54,0x64,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63,
	0x00,0x00,0x00,0x32,0x64,0x00,0x0A,0x00,0xFF,0xA5,0x44,0x54,0x64,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};

// 01 00  02 00  03 00  04 32  05 64  06 00  07 FF  08 00  09 FF  0A 01  0B 14  0C 52  0D 63  0E 20  0F 00
// 10 14  11 C8  12 0A  13 05  14 05  15 00  16 C8  17 0A  18 0A  19 04  1A 04  26 00  27 14  28 52  29 63
// 81 00  82 00  83 00  84 32  85 64  86 00  87 FF  88 00  89 FF  8A 26  8B 14  8C 52  8D 63  8E 20  8F 00
// 90 14  91 C8  92 0A  93 05  94 05  95 00  96 C8  97 0A  98 0A  99 04  9A 04  A6 20  A7 14  A8 52  A9 63 
const uint8_t peerSingle[] = {
	// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F, - 15 bytes
	// 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E, - 15 bytes
	// 0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F, - 15 bytes
	// 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E, - 15 bytes

	0x00,0x00,0x00,0x32,0x64,0x00,0xff,0x00,0xff,0x01,0x14,0x52,0x63,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x00,0x14,0x52,0x63,
	0x00,0x00,0x00,0x32,0x64,0x00,0xFF,0x00,0xFF,0x26,0x14,0x52,0x63,0x20,0x00,
	0x14,0xC8,0x0A,0x05,0x05,0x00,0xC8,0x0A,0x0A,0x04,0x04,0x20,0x14,0x52,0x63,
};


class cmDimmer {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------

	struct s_lstCnl {
		// 0x30,0x32,0x34,0x35,0x56,0x57,0x58,0x59,
		uint8_t  transmitTryMax      :8;     // 0x30, s:0, e:8
		uint8_t  ovrTempLvl          :8;     // 0x32, s:0, e:8
		uint8_t  redTempLvl          :8;     // 0x34, s:0, e:8
		uint8_t  redLvl              :8;     // 0x35, s:0, e:8
		uint8_t  powerUpAction       :1;     // 0x56, s:0, e:1
		uint8_t                      :7;     // 0x56, s:1, e:8
		uint8_t  statusInfoMinDly    :5;     // 0x57, s:0, e:5
		uint8_t  statusInfoRandom    :3;     // 0x57, s:5, e:8
		uint8_t  characteristic      :1;     // 0x58, s:0, e:1
		uint8_t                      :7;     // 0x58, s:1, e:8
		uint8_t  logicCombination    :5;     // 0x59, s:0, e:5
		uint8_t                      :3;     // 0x59, s:5, e:8
	} lstCnl;

	struct s_lstPeer {
		// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
		uint8_t  shCtRampOn          :4;     // 0x01, s:0, e:4
		uint8_t  shCtRampOff         :4;     // 0x01, s:4, e:8
		uint8_t  shCtDlyOn           :4;     // 0x02, s:0, e:4
		uint8_t  shCtDlyOff          :4;     // 0x02, s:4, e:8
		uint8_t  shCtOn              :4;     // 0x03, s:0, e:4
		uint8_t  shCtOff             :4;     // 0x03, s:4, e:8
		uint8_t  shCtValLo           :8;     // 0x04, s:0, e:0
		uint8_t  shCtValHi           :8;     // 0x05, s:0, e:0
		uint8_t  shOnDly             :8;     // 0x06, s:0, e:0
		uint8_t  shOnTime            :8;     // 0x07, s:0, e:0
		uint8_t  shOffDly            :8;     // 0x08, s:0, e:0
		uint8_t  shOffTime           :8;     // 0x09, s:0, e:0
		uint8_t  shActionTypeDim     :4;     // 0x0a, s:0, e:4
		uint8_t                      :2;     // 0x0a, s:4, e:6
		uint8_t  shOffTimeMode       :1;     // 0x0a, s:6, e:7
		uint8_t  shOnTimeMode        :1;     // 0x0a, s:7, e:8
		uint8_t  shDimJtOn           :4;     // 0x0b, s:0, e:4
		uint8_t  shDimJtOff          :4;     // 0x0b, s:4, e:8
		uint8_t  shDimJtDlyOn        :4;     // 0x0c, s:0, e:4
		uint8_t  shDimJtDlyOff       :4;     // 0x0c, s:4, e:8
		uint8_t  shDimJtRampOn       :4;     // 0x0d, s:0, e:4
		uint8_t  shDimJtRampOff      :4;     // 0x0d, s:4, e:8
		uint8_t                      :5;     // 0x0e, s:0, e:5
		uint8_t  shOffDlyBlink       :1;     // 0x0e, s:5, e:6
		uint8_t  shOnLvlPrio         :1;     // 0x0e, s:6, e:7
		uint8_t  shOnDlyMode         :1;     // 0x0e, s:7, e:8
		uint8_t  shOffLevel          :8;     // 0x0f, s:0, e:0
		uint8_t  shOnMinLevel        :8;     // 0x10, s:0, e:0
		uint8_t  shOnLevel           :8;     // 0x11, s:0, e:0
		uint8_t  shRampSstep         :8;     // 0x12, s:0, e:0
		uint8_t  shRampOnTime        :8;     // 0x13, s:0, e:0
		uint8_t  shRampOffTime       :8;     // 0x14, s:0, e:0
		uint8_t  shDimMinLvl         :8;     // 0x15, s:0, e:0
		uint8_t  shDimMaxLvl         :8;     // 0x16, s:0, e:0
		uint8_t  shDimStep           :8;     // 0x17, s:0, e:0
		uint8_t  shOffDlyStep        :8;     // 0x18, s:0, l:8
		uint8_t  shOffDlyNewTime     :8;     // 0x19, s:0, e:0
		uint8_t  shOffDlyOldTime     :8;     // 0x1a, s:0, e:0
		uint8_t  shDimElsActionType  :4;     // 0x26, s:0, e:4
		uint8_t                      :2;     // 0x26, s:4, e:6
		uint8_t  shDimElsOffTimeMd   :1;     // 0x26, s:6, e:7
		uint8_t  shDimElsOnTimeMd    :1;     // 0x26, s:7, e:8
		uint8_t  shDimElsJtOn        :4;     // 0x27, s:0, e:4
		uint8_t  shDimElsJtOff       :4;     // 0x27, s:4, e:8
		uint8_t  shDimElsJtDlyOn     :4;     // 0x28, s:0, e:4
		uint8_t  shDimElsJtDlyOff    :4;     // 0x28, s:4, e:8
		uint8_t  shDimElsJtRampOn    :4;     // 0x29, s:0, e:4
		uint8_t  shDimElsJtRampOff   :4;     // 0x29, s:4, e:8
		uint8_t  lgCtRampOn          :4;     // 0x81, s:0, e:4
		uint8_t  lgCtRampOff         :4;     // 0x81, s:4, e:8
		uint8_t  lgCtDlyOn           :4;     // 0x82, s:0, e:4
		uint8_t  lgCtDlyOff          :4;     // 0x82, s:4, e:8
		uint8_t  lgCtOn              :4;     // 0x83, s:0, e:4
		uint8_t  lgCtOff             :4;     // 0x83, s:4, e:8
		uint8_t  lgCtValLo           :8;     // 0x84, s:0, e:0
		uint8_t  lgCtValHi           :8;     // 0x85, s:0, e:0
		uint8_t  lgOnDly             :8;     // 0x86, s:0, e:0
		uint8_t  lgOnTime            :8;     // 0x87, s:0, e:0
		uint8_t  lgOffDly            :8;     // 0x88, s:0, e:0
		uint8_t  lgOffTime           :8;     // 0x89, s:0, e:0
		uint8_t  lgActionTypeDim     :4;     // 0x8a, s:0, e:4
		uint8_t                      :1;     // 0x8a, s:4, e:5
		uint8_t  lgMultiExec         :1;     // 0x8a, s:5, e:6
		uint8_t  lgOffTimeMode       :1;     // 0x8a, s:6, e:7
		uint8_t  lgOnTimeMode        :1;     // 0x8a, s:7, e:8
		uint8_t  lgDimJtOn           :4;     // 0x8b, s:0, e:4
		uint8_t  lgDimJtOff          :4;     // 0x8b, s:4, e:8
		uint8_t  lgDimJtDlyOn        :4;     // 0x8c, s:0, e:4
		uint8_t  lgDimJtDlyOff       :4;     // 0x8c, s:4, e:8
		uint8_t  lgDimJtRampOn       :4;     // 0x8d, s:0, e:4
		uint8_t  lgDimJtRampOff      :4;     // 0x8d, s:4, e:8
		uint8_t                      :5;     // 0x8e, s:0, e:5
		uint8_t  lgOffDlyBlink       :1;     // 0x8e, s:5, e:6
		uint8_t  lgOnLvlPrio         :1;     // 0x8e, s:6, e:7
		uint8_t  lgOnDlyMode         :1;     // 0x8e, s:7, e:8
		uint8_t  lgOffLevel          :8;     // 0x8f, s:0, e:0
		uint8_t  lgOnMinLevel        :8;     // 0x90, s:0, e:0
		uint8_t  lgOnLevel           :8;     // 0x91, s:0, e:0
		uint8_t  lgRampSstep         :8;     // 0x92, s:0, e:0
		uint8_t  lgRampOnTime        :8;     // 0x93, s:0, e:0
		uint8_t  lgRampOffTime       :8;     // 0x94, s:0, e:0
		uint8_t  lgDimMinLvl         :8;     // 0x95, s:0, e:0
		uint8_t  lgDimMaxLvl         :8;     // 0x96, s:0, e:0
		uint8_t  lgDimStep           :8;     // 0x97, s:0, e:0
		uint8_t  lgOffDlyStep        :8;     // 0x98, s:0, l:8
		uint8_t  lgOffDlyNewTime     :8;     // 0x99, s:0, e:0
		uint8_t  lgOffDlyOldTime     :8;     // 0x9a, s:0, e:0
		uint8_t  lgDimElsActionType  :4;     // 0xa6, s:0, e:4
		uint8_t                      :2;     // 0xa6, s:5, e:6
		uint8_t  lgDimElsOffTimeMd   :1;     // 0xa6, s:6, e:7
		uint8_t  lgDimElsOnTimeMd    :1;     // 0xa6, s:7, e:8
		uint8_t  lgDimElsJtOn        :4;     // 0xa7, s:0, e:4
		uint8_t  lgDimElsJtOff       :4;     // 0xa7, s:4, e:8
		uint8_t  lgDimElsJtDlyOn     :4;     // 0xa8, s:0, e:4
		uint8_t  lgDimElsJtDlyOff    :4;     // 0xa8, s:4, e:8
		uint8_t  lgDimElsJtRampOn    :4;     // 0xa9, s:0, e:4
		uint8_t  lgDimElsJtRampOff   :4;     // 0xa9, s:4, e:8
	} lstPeer;

	struct s_l3 {
		// 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,
		// 0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa6,0xa7,0xa8,0xa9,
		uint8_t  ctRampOn            :4;     // 0x01,0x81, s:0, e:4
		uint8_t  ctRampOff           :4;     // 0x01,0x81, s:4, e:8
		uint8_t  ctDlyOn             :4;     // 0x02,0x82, s:0, e:4
		uint8_t  ctDlyOff            :4;     // 0x02,0x82, s:4, e:8
		uint8_t  ctOn                :4;     // 0x03,0x83, s:0, e:4
		uint8_t  ctOff               :4;     // 0x03,0x83, s:4, e:8
		uint8_t  ctValLo             :8;     // 0x04,0x84, s:0, e:0
		uint8_t  ctValHi             :8;     // 0x05,0x85, s:0, e:0
		uint8_t  onDly               :8;     // 0x06,0x86, s:0, e:0
		uint8_t  onTime              :8;     // 0x07,0x87, s:0, e:0
		uint8_t  offDly              :8;     // 0x08,0x88, s:0, e:0
		uint8_t  offTime             :8;     // 0x09,0x89, s:0, e:0
		uint8_t  actionType          :4;     // 0x0a,0x8a, s:0, e:4
		uint8_t                      :1;     // 0x0a,0x8a, s:4. e:5
		uint8_t  lgMultiExec         :1;     // 0x0a,0x8a, s:5, e:6
		uint8_t  offTimeMode         :1;     // 0x0a,0x8a, s:6, e:7
		uint8_t  onTimeMode          :1;     // 0x0a,0x8a, s:7, e:8
		uint8_t  jtOn                :4;     // 0x0b,0x8b, s:0, e:4
		uint8_t  jtOff               :4;     // 0x0b,0x8b, s:4, e:8
		uint8_t  jtDlyOn             :4;     // 0x0c,0x8c, s:0, e:4
		uint8_t  jtDlyOff            :4;     // 0x0c,0x8c, s:4, e:8
		uint8_t  jtRampOn            :4;     // 0x0d,0x8d, s:0, e:4
		uint8_t  jtRampOff           :4;     // 0x0d,0x8d, s:4, e:8
		uint8_t                      :5;     // 0x0e,0x8e, s:0, e:5
		uint8_t  offDlyBlink         :1;     // 0x0e,0x8e, s:5, e:6
		uint8_t  onLvlPrio           :1;     // 0x0e,0x8e, s:6, e:7
		uint8_t  onDlyMode           :1;     // 0x0e,0x8e, s:7, e:8
		uint8_t  offLevel            :8;     // 0x0f,0x8f, s:0, e:0
		uint8_t  onMinLevel          :8;     // 0x10,0x90, s:0, e:0
		uint8_t  onLevel             :8;     // 0x11,0x91, s:0, e:0
		uint8_t  rampSstep           :8;     // 0x12,0x92, s:0, e:0
		uint8_t  rampOnTime          :8;     // 0x13,0x93, s:0, e:0
		uint8_t  rampOffTime         :8;     // 0x14,0x94, s:0, e:0
		uint8_t  dimMinLvl           :8;     // 0x15,0x95, s:0, e:0
		uint8_t  dimMaxLvl           :8;     // 0x16,0x96, s:0, e:0
		uint8_t  dimStep             :8;     // 0x17,0x97, s:0, e:0
		uint8_t  offDlyStep          :8;     // 0x18,0x98, s:0, l:8
		uint8_t  offDlyNewTime       :8;     // 0x19,0x99, s:0, e:0
		uint8_t  offDlyOldTime       :8;     // 0x1a,0x9a, s:0, e:0
		uint8_t  elsActionType       :4;     // 0x26,0xa6, s:0, e:4
		uint8_t                      :2;     // 0x26,0xa6, s:4, e:6
		uint8_t  elsOffTimeMd        :1;     // 0x26,0xa6, s:6, e:7
		uint8_t  elsOnTimeMd         :1;     // 0x26,0xa6, s:7, e:8
		uint8_t  elsJtOn             :4;     // 0x27,0xa7, s:0, e:4
		uint8_t  elsJtOff            :4;     // 0x27,0xa7, s:4, e:8
		uint8_t  elsJtDlyOn          :4;     // 0x28,0xa8, s:0, e:4
		uint8_t  elsJtDlyOff         :4;     // 0x28,0xa8, s:4, e:8
		uint8_t  elsJtRampOn         :4;     // 0x29,0xa9, s:0, e:4
		uint8_t  elsJtRampOff        :4;     // 0x29,0xa9, s:4, e:8
	} *l3;
	
	void (*fInit)(uint8_t);																	// pointer to init function in main sketch
	void (*fSwitch)(uint8_t,uint8_t,uint8_t);												// pointer to switch function (PWM) in main sketch, first value is PWM level, second the characteristics

	uint8_t  *pTemp;																		// pointer to temperature byte in main sketch
	
	uint8_t   sendStat :2;																	// is there a status to be send, 1 indicates an ACK, 2 a status message 
	uint16_t  msgDelay;
	waitTimer msgTmr;																		// message timer for sending status

	waitTimer delayTmr;																		// delay timer for on,off and delay time
	uint16_t  rampTme, duraTme;																// time store for trigger 11

	uint8_t   setStat;																		// status to set on the PWM channel
	uint32_t  adjDlyPWM;																	// timer to follow in adjPWM function
	uint16_t  characteristicStat;															// depends on list1 characteristic setting
	waitTimer adjTmr;																		// timer for adjustment of PWM

	//uint8_t   oldStat;																	// remember modStat in delay off blink function
	uint8_t   activeOffDlyBlink :1;															// activate off delay blinking
	uint8_t   statusOffDlyBlink :1;															// remember led off cycle
	uint8_t   directionDim :1;																// used in toogleDim function

	uint8_t   cnt;																			// message counter for type 40 message
	uint8_t   curStat:4, nxtStat:4;															// current state and next state

  public://----------------------------------------------------------------------------------------------------------------
  //- user defined functions ----------------------------------------------------------------------------------------------

	void     config(void Init(uint8_t), void xSwitch(uint8_t,uint8_t,uint8_t), uint8_t *temperature);// configures the module, jump addresses, etc

	void     trigger11(uint8_t setValue, uint8_t *rampTime, uint8_t *duraTime);				// messages coming from master
	void     trigger40(uint8_t msgLng, uint8_t msgCnt);										// messages coming from switch
	void     trigger41(uint8_t msgBLL, uint8_t msgCnt, uint8_t msgVal);						// messages coming from sensor

  private://---------------------------------------------------------------------------------------------------------------
	void     toggleDim(void);																// dim up or down with one key
	void     upDim(void);																	// up dim procedure
	void     downDim(void);																	// down dim procedure

	void     adjPWM(void);																	// adjusts PWM value in a regular manner
	void     blinkOffDly(void);																// polling function to blink led while in off delay
	void     sendStatus(void);																// send status function
	void     dimPoll(void);																	// dimmer polling function
	
  //- helpers defined functions -------------------------------------------------------------------------------------------
	void     showStruct(void);

	
  public://----------------------------------------------------------------------------------------------------------------
  //- mandatory functions for every new module to communicate within AS protocol stack ------------------------------------
	uint8_t  modStat;																		// module status byte, needed for list3 modules to answer status requests
	uint8_t  modDUL;																		// module down up low battery byte
	uint8_t  regCnl;																		// holds the channel for the module

	//AS       *hm;																			// pointer to HM class instance

	void     setToggle(void);																// toggle the module initiated by config button
	void     configCngEvent(void);															// list1 on registered channel had changed
	void     pairSetEvent(uint8_t *data, uint8_t len);										// pair message to specific channel, handover information for value, ramp time and so on
	void     pairStatusReq(void);															// event on status request
	void     peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);						// peer message was received on the registered channel, handover the message bytes and length

	void     poll(void);																	// poll function, driven by HM loop

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void     regInHM(uint8_t cnl, uint8_t lst, AS *instPtr);								// register this module in HM on the specific channel
	void     hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len);// call back address for HM for informing on events
	void     peerAddEvent(uint8_t *data, uint8_t len);										// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
	void     firstStart(void);																// first start detection, to write list1
};


#endif
