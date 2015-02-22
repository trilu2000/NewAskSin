//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de>, <dirk@hfma.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _VERSION_H
	#define _VERSION_H

	#define LIB_VERSION_MAJOR     0
	#define LIB_VERSION_MINOR     2
	#define LIB_VERSION_PATCH     2

	#define STR(x) #x
	#define LIB_VERSION_STRING_FUNC(v1, v2, v3)  "AskSin-Lib V " STR(v1) "." STR(v2) "." STR(v3) "\n"
	#define LIB_VERSION_STRING LIB_VERSION_STRING_FUNC (LIB_VERSION_MAJOR, LIB_VERSION_MINOR, LIB_VERSION_PATCH)

#endif
