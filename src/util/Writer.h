#pragma once

#include "./store/BlockedList.h"
#include "./store/StringSlice.h"
#include "./Option.h"

class Writer {
public:
	using Output = BlockedList<1024, char>;

private:
	Output out;
	Arena& arena;
	uint _indent;

public:
	Writer(Arena& _arena) : out{}, arena{_arena}, _indent{0} {}
	Writer(const Writer& other) = delete;
	void operator=(const Writer& other) = delete;

	Output finish() { return out; }

	inline Writer& operator<<(char c) {
		out.push(c, arena);
		return *this;
	}
	inline Writer& operator<<(const char* s) {
		while (*s != '\0') {
			out.push(*s, arena);
			++s;
		}
		return *this;
	}
	inline Writer& operator<<(const StringSlice& s) {
		for (char c : s)
			out.push(c, arena);
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
