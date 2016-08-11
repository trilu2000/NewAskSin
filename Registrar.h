//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin registrar functions --------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------


#ifndef _RG_H
#define _RG_H

#include "HAL.h"

//- some macro to make it easier to call hmEventCol of user modules
#define _USER_MODULE_POLL(CNL)              if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x00, 0x00, 0x00, NULL, 0)
#define _USER_MODULE_TOGGLE(CNL)            if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x00, 0x01, 0x00, NULL, 0)
#define _USER_MODULE_CONFIG_CHANGE(CNL)     if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x01, 0x00, 0x06, NULL, 0)
#define _USER_MODULE_PAIR_STATUS(CNL)       if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x01, 0x00, 0x0E, NULL, 0)
#define _USER_MODULE_PEER_ADD(CNL,DATA,LEN) if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x01, 0x00, 0x01, DATA, LEN)
#define _USER_MODULE_PAIR_SET(CNL,DATA,LEN) if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x11, 0x02, 0x00, DATA, LEN)
//#define _USER_MODULE_PEER_MSG(CNL,DATA,LEN) if (modTbl[CNL-1].cnl) modTbl[CNL-1].mDlgt(0x11, 0x02, 0x00, DATA, LEN)
//modTbl[mod_cnl].mDlgt(0x01, 0, 0x06, NULL, 0);
//else if (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);


/**
* @brief Delegation for channel module registrar.
*        There is no way to store class memeber pointers easily in c++,
*        therefore we use a delegation class.
* You will find more infos here: http://blog.coldflake.com/posts/C++-delegates-on-steroids/
*
*/
template<typename return_type, typename... params>
class Delegate {

	typedef return_type(*Type)(void* callee, params...);

public:	 //---------------------------------------------------------------------------------------------------------
	Delegate(void* callee = NULL, Type function = NULL) : fpCallee(callee), fpCallbackFunction(function) {}
	template <class T, return_type(T::*TMethod)(params...)>
	static Delegate from_function(T* callee) {
		Delegate d(callee, &methodCaller<T, TMethod>);
		return d;
	}
	return_type operator()(params... xs) const {
		return (*fpCallbackFunction)(fpCallee, xs...);
	}

private: //---------------------------------------------------------------------------------------------------------
	void* fpCallee;
	Type fpCallbackFunction;
	template <class T, return_type(T::*TMethod)(params...)>
	static return_type methodCaller(void* callee, params... xs) {
		T* p = static_cast<T*>(callee);
		return (p->*TMethod)(xs...);
	}
};
//- typedef for delegate to module function
typedef Delegate<void, uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t> myDelegate;



class RG {

public:	//---------------------------------------------------------------------------------------------------------
	struct s_modTable {
		uint8_t cnl;																		// channel where the module is registered to
		uint8_t lst;																		// module has a list3 or list 4
		uint8_t msgCnt;																		// channel message counter
		uint8_t *lstCnl;																	// pointer to list0/1
		uint8_t *lstPeer;																	// pointer to list3/4
		myDelegate mDlgt;																	// delegate to the module function
	};

	RG() {}																					// class constructor
	//enum event { POLL, SET_TOGGLE, CONFIG_CHANGE, PAIR_SET, PAIR_STATUS, PEER_ADD, PEER_MESSAGE };
	void regUserModuleInAS(uint8_t cnl, uint8_t lst, myDelegate delegate, uint8_t *mainList, uint8_t *peerList);
	void poll(void);
	
};
extern RG::s_modTable modTbl[];																// initial register.h





#endif
