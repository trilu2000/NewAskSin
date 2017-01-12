#include "cm.h"

void debugActionType(uint8_t actionType) {
	dbg << F(">ACTION_TYPE: ");
	if        (actionType == AS_CM_ACTIONTYPE_INACTIVE) {
		dbg << F("INACTIVE");

	} else if (actionType == AS_CM_ACTIONTYPE_JUMP_TO_TARGET) {
		dbg << F("JUMP_TO_TARGET");

	} else if (actionType == AS_CM_ACTIONTYPE_TOGGLE_TO_COUNTER) {
		dbg << F("TOGGLE_TO_COUNTER");

	} else if (actionType == AS_CM_ACTIONTYPE_TOGGLE_INVERSE_TO_COUNTER) {
		dbg << F("TOGGLE_INVERSE_TO_COUNTER");

	} else if (actionType == AS_CM_ACTIONTYPE_UPDIM) {
		dbg << F("ACTIONTYPE_UPDIM");

	} else if (actionType == AS_CM_ACTIONTYPE_DOWNDIM) {
		dbg << F("DOWNDIM");

	} else if (actionType == AS_CM_ACTIONTYPE_TOGGLEDIM) {
		dbg << F("TOGGLEDIM");

	} else if (actionType == AS_CM_ACTIONTYPE_TOGGLEDIM_TO_COUNTER) {
		dbg << F("TOGGLEDIM_TO_COUNTER");

	} else if (actionType == AS_CM_ACTIONTYPE_TOGGLEDIM_TO_COUNTER_INVERSE) {
		dbg << F("TOGGLEDIM_TO_COUNTER_INVERSE");
	}
}

void debugState(uint8_t state) {
	if        (state == AS_CM_JT_NONE) {
		dbg << F("NONE");

	} else if (state == AS_CM_JT_ONDELAY) {
		dbg << F("ONDELAY");

	} else if (state == AS_CM_JT_REFON) {
		dbg << F("REFON");

	} else if (state == AS_CM_JT_RAMPON) {
		dbg << F("RAMPON");

	} else if (state == AS_CM_JT_ON) {
		dbg << F("ON");

	} else if (state == AS_CM_JT_OFFDELAY) {
		dbg << F("OFFDELAY");

	} else if (state == AS_CM_JT_REFOFF) {
		dbg << F("REFOFF");

	} else if (state == AS_CM_JT_RAMPOFF) {
		dbg << F("RAMPOFF");

	} else if (state == AS_CM_JT_OFF) {
		dbg << F("OFF");
	}
}

//- helpers defined functions -------------------------------------------------------------------------------------------
inline void debugShowStruct(void) {
//	dbg << "\nctRampOn " << l3->ctRampOn << ", ctRampOff " << l3->ctRampOff << ", ctDlyOn " << l3->ctDlyOn << \
//	        ", ctDlyOff " << l3->ctDlyOff << ", ctOn " << l3->ctOn << ", ctOff " << l3->ctOff << \
//			", ctValLo " << l3->COND_VALUE_LO << ", ctValHi " << l3->COND_VALUE_HI << '\n' << '\n';

//	dbg << "onDly " << l3->ONDELAY_TIME << ", onTime " << l3->ON_TIME << ", offDly " << l3->OFFDELAY_TIME << ", OFF_TIME " << l3->OFF_TIME << \
//	       ", >ACTION_TYPE " << l3->ACTION_TYPE << ", lgMultiExec " << l3->lgMultiExec << \
//		   ", OFF_TIMEMode " << l3->OFF_TIMEMode << ", onTimeMode " << l3->ON_TIMEMode << '\n' << '\n';

//	dbg << "jtOn " << l3->JT_ON << ", jtOff " << l3->JT_OFF << ", jtDlyOn " << l3->JT_ONDELAY << ", jtDlyOff " << l3->JT_OFFDELAY << \
//	       ", jtRampOn " << l3->JT_RAMPON << ", jtRampOff " << l3->JT_RAMPOFF << '\n' << '\n';

//	dbg << "offDlyBlink " << l3->offDlyBlink << ", onLvlPrio " << l3->onLvlPrio << ", onDlyMode " << l3->ONDELAY_TIMEMode << ", offLevel " << l3->OFF_LEVEL << \
//	       ", onMinLevel " << l3->onMinLevel << ", onLevel " << l3->ON_LEVEL << '\n' << '\n';

//	dbg << "rampSstep " << l3->rampSstep << ", REFERENCE_RUNNING_TIME_TOP_BOTTOM " << lstCnl->REFERENCE_RUNNING_TIME_TOP_BOTTOM << ", rampOFF_TIME " << l3->rampOFF_TIME << '\n' << '\n';

//	dbg << "dimMinLvl " << l3->dimMinLvl << ", dimMaxLvl " << l3->dimMaxLvl << ", dimStep " << l3->dimStep << \
//	       ", offDlyNewTime " << l3->offDlyNewTime << ", offDlyOldTime " << l3->offDlyOldTime << '\n' << '\n';

//	dbg << '\n';
}
