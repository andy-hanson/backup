#pragma once

#include "./Grow.h"
#include "./Option.h"
#include "./StringSlice.h"

class Writer {
	Grow<char>& out;
	uint _indent = 0;

public:
	Writer(Grow<char>& _out) : out(_out) {}
	Writer(const Writer& other) = delete;
	void operator=(const Writer& other) = delete;

	inline Writer& operator<<(char c) {
		out.push_copy(c);
		return *this;
	}
	inline Writer& operator<<(const char* s) {
		while (*s != '\0') {
			out.push_copy(*s);
			++s;
		}
		return *this;
	}
	inline Writer& operator<<(const StringSlice& s) {
		for (char c : s)
			out.push_copy(c);
		return *this;
	}
	Writer& operator<<(uint u);
	Writer& operator<<(ushort u);
	static const struct indent_t {} indent;
	static const struct dedent_t {} dedent;
	static const struct nl_t {} nl;
	inline Writer& operator<<(indent_t) { ++_indent; return *this; }
	inline Writer& operator<<(dedent_t) { --_indent; return *this; }
	Writer& operator<<(nl_t);
};

struct indented { const StringSlice& s; };
Writer& operator<<(Writer& out, indented i);
