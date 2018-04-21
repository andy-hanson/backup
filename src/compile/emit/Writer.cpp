#include "Writer.h"

Writer& operator<<(Writer& out, indented i)  {
	for (char c : i.s) {
		out << c;
		if (c == '\n')
			out << '\t';
	}
	return out;
}
