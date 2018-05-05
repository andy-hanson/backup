#include "Writer.h"

Writer& Writer::operator<<(uint u) {
	//TODO: duplicate code in ArenaString.cpp
	if (u < 10) {
		*this << char('0' + char(u));
	} else if (u < 100) {
		*this << char('0' + char(u / 10));
		*this << char('0' + char(u % 10));
	} else
		throw "todo";
	return *this;
}
Writer& Writer::operator<<(ushort u) {
	return *this << uint(u);
}

Writer& operator<<(Writer& out, indented i)  {
	for (char c : i.s) {
		out << c;
		if (c == '\n')
			out << '\t';
	}
	return out;
}

Writer& Writer::operator<<(nl_t) {
	*this << '\n';
	for (uint i = 0; i != _indent; ++i)
		*this << '\t';
	return *this;
}
