#ifndef INPUTPARSER_H
	#define INPUTPARSER_H

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	//- -----------------------------------------------------------------------------------------------------------------------
	//- serial parser and display functions -----------------------------------------------------------------------------------
	//- Parser sketch from: http://jeelabs.org/2010/10/24/parsing-input-commands/
	//- -----------------------------------------------------------------------------------------------------------------------
	class InputParser {
		public:
		typedef struct {
			char code;															// one-letter command code
			byte bytes;															// number of bytes required as input
			void (*fun)();														// code to call for this command
		} Commands;

		InputParser (byte size, Commands*, Stream& =Serial);						// set up with a buffer of specified size

		byte count() { return fill; }												// number of data bytes
		byte *buffer;																// holds the read data

		void poll();																// call this frequently to check for incoming data

		InputParser& operator >> (char& v)      { return get(&v, 1); }
		InputParser& operator >> (byte& v)      { return get(&v, 1); }
		InputParser& operator >> (int& v)       { return get(&v, 2); }
		InputParser& operator >> (word& v)      { return get(&v, 2); }
		InputParser& operator >> (long& v)      { return get(&v, 4); }
		InputParser& operator >> (uint32_t& v)  { return get(&v, 4); }
		InputParser& operator >> (const char*& v);

		private:
		InputParser& get(void*, byte);
		void reset();

		byte limit, fill, top, next;
		byte instring, hexmode, hasvalue;
		uint32_t value;
		Commands* cmds;
		Stream& io;
	};
	extern const InputParser::Commands cmdTab[];
#endif
