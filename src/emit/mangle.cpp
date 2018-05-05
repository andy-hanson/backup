#include "mangle.h"

namespace {
	Option<StringSlice> mangle_char(char c) {
		switch (c) {
			case '+':
				return Option<StringSlice> { "_add" };
			case '-':
				// '-' often used as a hyphen
				return Option<StringSlice> { "__" };
			case '*':
				return Option<StringSlice> { "_times" };
			case '/':
				return Option<StringSlice> { "_div" };
			case '<':
				return Option<StringSlice> { "_lt" };
			case '>':
				return Option<StringSlice> { "_gt" };
			case '=':
				return Option<StringSlice> { "_eq" };
			default:
				assert(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
				return {};
		}
	}

	const StringSlice TRUE = "true";
	const StringSlice FALSE = "false";

	template <typename WriterLike>
	void write_mangled(WriterLike& out, const StringSlice& name) {
		if (name == TRUE || name == FALSE)
			out << '_' << name;
		else {
			for (char c : name) {
				auto m = mangle_char(c);
				if (m.has())
					out << m.get();
				else
					out << c;
			}
		}
	}
}

Writer& operator<<(Writer& out, mangle man) {
	write_mangled(out, man.name);
	return out;
}
StringBuilder& operator<<(StringBuilder& out, mangle man) {
	write_mangled(out, man.name);
	return out;
}
