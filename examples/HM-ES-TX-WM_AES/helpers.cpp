#include "helpers.h"

//- -----------------------------------------------------------------------------------------------------------------------
//- serial parser and display functions -----------------------------------------------------------------------------------
//- Parser sketch from: http://jeelabs.org/2010/10/24/parsing-input-commands/
//- -----------------------------------------------------------------------------------------------------------------------
InputParser::InputParser (byte size, Commands *ctab, Stream& stream)
: limit (size), cmds (ctab), io (stream) {
	buffer = (byte*) malloc(size);
	reset();
}
void InputParser::reset() {
	fill = next = 0;
	instring = hexmode = hasvalue = 0;
	top = limit;
}
void InputParser::poll() {
	if (!io.available())
	return;
	char ch = io.read();
	if (ch < ' ' || fill >= top) {
		reset();
		return;
	}
	if (instring) {
		if (ch == '"') {
			buffer[fill++] = 0;
			do
			buffer[--top] = buffer[--fill];
			while (fill > value);
			ch = top;
			instring = 0;
		}
		buffer[fill++] = ch;
		return;
	}
	if (hexmode && (('0' <= ch && ch <= '9') ||
	('A' <= ch && ch <= 'F') ||
	('a' <= ch && ch <= 'f'))) {
		if (!hasvalue)
		value = 0;
		if (ch > '9')
		ch += 9;
		value <<= 4;
		value |= (byte) (ch & 0x0F);
		hasvalue = 1;
		return;
	}
	if ('0' <= ch && ch <= '9') {
		if (!hasvalue)
		value = 0;
		value = 10 * value + (ch - '0');
		hasvalue = 1;
		return;
	}
	hexmode = 0;
	switch (ch) {
		case '$':   hexmode = 1;
		return;
		case '"':   instring = 1;
		value = fill;
		return;
		case ':':   (word&) buffer[fill] = value;
		fill += 2;
		value >>= 16;
		// fall through
		case '.':   (word&) buffer[fill] = value;
		fill += 2;
		hasvalue = 0;
		return;
		case '-':   value = - value;
		hasvalue = 0;
		return;
		case ' ':   if (!hasvalue)
		return;
		// fall through
		case ',':   buffer[fill++] = value;
		hasvalue = 0;
		return;
	}
	if (hasvalue) {
		io.print(F("Unrecognized character: "));
		io.print(ch);
		io.println();
		reset();
		return;
	}

	for (Commands* p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
		break;
		if (ch == code) {
			byte bytes = pgm_read_byte(&p->bytes);
			if (fill < bytes) {
				io.print(F("Not enough data, need "));
				io.print((int) bytes);
				io.println(F(" bytes"));
				io.println(F(" got: "));
				io.print((int) fill);
				io.println(F(" bytes"));
			} else {
				memset(buffer + fill, 0, top - fill);
				((void (*)()) pgm_read_word(&p->fun))();
			}
			reset();
			return;
		}
	}

	io.print(F("Known commands:"));
	for (Commands* p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
		break;
		io.print(' ');
		io.print(code);
	}
	io.println();
}
InputParser& InputParser::get(void *ptr, byte len) {
	memcpy(ptr, buffer + next, len);
	next += len;
	return *this;
}
InputParser& InputParser::operator >> (const char*& v) {
	byte offset = buffer[next++];
	v = top <= offset && offset < limit ? (char*) buffer + offset : "";
	return *this;
}
