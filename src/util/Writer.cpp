#include "Writer.h"

Writer& operator<<(Writer& out, indented i)  {
	for (char c : i.s) {
		out << c;
		if (c == '\n')
			out << '\t';
	}
	return out;
}

Writer& Writer::operator<<(nl_t) {
	out += '\n';
	for (uint i = 0; i != _indent; ++i)
		out += '\t';
	return *this;
}
