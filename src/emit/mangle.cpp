#include "mangle.h"

namespace {
	Option<StringSlice> mangle_char(char c) {
		switch (c) {
			case '+':
				return { "_add" };
			case '-':
				// '-' often used as a hyphen
				return { "__" };
			case '*':
				return { "_times" };
			case '/':
				return { "_div" };
			case '<':
				return { "_lt" };
			case '>':
				return { "_gt" };
			case '=':
				return { "_eq" };
			default:
				assert(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
				return {};
		}
	}
}

Writer& operator<<(Writer& out, mangle man) {
	for (char c : man.name.str.slice()) {
		auto m = mangle_char(c);
		if (m)
			out << m.get();
		else
			out << c;
	}
	return out;
}

//TODO: share code
Arena::StringBuilder& operator<<(Arena::StringBuilder& out, mangle man) {
	for (char c : man.name.str.slice()) {
		auto m = mangle_char(c);
		if (m)
			out << m.get();
		else
			out << c;
	}
	return out;
}
