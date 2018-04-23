#pragma once

#include "./Option.h"
#include "./StringSlice.h"

class Writer {
	std::string out;
	uint _indent = 0;

public:
	Writer() = default;
	Writer(const Writer& other) = delete;
	void operator=(const Writer& other) = delete;

	std::string finish() {
		return std::move(out);
	}

	inline Writer& operator<<(char s) {
		out += s;
		return *this;
	}
	inline Writer& operator<<(const char* s) {
		out += s;
		return *this;
	}
	inline Writer& operator<<(const StringSlice& s) {
		for (char c : s)
			out += c;
		return *this;
	}
	inline Writer& operator<<(size_t u) {
		out += std::to_string(u);
		return *this;
	}
	inline Writer& operator<<(uint u) {
		out += std::to_string(u);
		return *this;
	}
	inline Writer& operator<<(ushort u) {
		out += std::to_string(u);
		return *this;
	}
	static const struct indent_t {} indent;
	static const struct dedent_t {} dedent;
	static const struct nl_t {} nl;
	inline Writer& operator<<(indent_t) { ++_indent; return *this; }
	inline Writer& operator<<(dedent_t) { --_indent; return *this; }
	Writer& operator<<(nl_t);
};

struct indented { const StringSlice& s; };
Writer& operator<<(Writer& out, indented i);
