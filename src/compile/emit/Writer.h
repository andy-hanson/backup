#pragma once

#include "../../util/Option.h"
#include "../../util/StringSlice.h"

class Writer {
	std::string out;
	uint _indent = 0;

public:
	static const struct indent_t {} indent;
	static const struct dedent_t {} dedent;
	static const struct nl_t {} nl;

	Writer() = default;
	Writer(const Writer& other) = delete;
	void operator=(const Writer& other) = delete;

	std::string finish() {
		return std::move(out);
	}

	Writer& operator<<(char s) {
		out += s;
		return *this;
	}
	Writer& operator<<(const char* s) {
		out += s;
		return *this;
	}
	Writer& operator<<(const StringSlice& s) {
		for (char c : s)
			out += c;
		return *this;
	}
	Writer& operator<<(size_t u) {
		out += std::to_string(u);
		return *this;
	}
	Writer& operator<<(uint u) {
		out += std::to_string(u);
		return *this;
	}
	Writer& operator<<(ushort u) {
		out += std::to_string(u);
		return *this;
	}
	Writer& operator<<(nl_t) {
		out += '\n';
		for (uint i = 0; i != _indent; ++i)
			out += '\t';
		return *this;
	}
	Writer& operator<<(indent_t) { ++_indent; return *this; }
	Writer& operator<<(dedent_t) { --_indent; return *this; }
};

struct indented { const StringSlice& s; };
Writer& operator<<(Writer& out, indented i);
