#include "Writer.h"

Writer& Writer::operator<<(size_t u) {
	*this << char('0' + char(u % 10));
	throw "todo";
}
Writer& Writer::operator<<(uint u) {
	return *this << size_t(u);
}
Writer& Writer::operator<<(ushort u) {
	return *this << size_t(u);
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
