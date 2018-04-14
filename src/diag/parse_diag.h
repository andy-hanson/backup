#pragma once

#include <string>
#include <sys/types.h>

#include "../util/StringSlice.h"
#include "../util/util.h"
#include "../emit/Writer.h"

struct SourceRange {
	uint start;
	uint end;

	static SourceRange from_pointers(const StringSlice& str, const char* begin, const char* end) {
		return { to_uint(to_unsigned(begin - str.begin())), to_uint(to_unsigned(end - str.begin())) };
	}
	static SourceRange from_pointer(const StringSlice& str, const char* begin) {
		return from_pointers(str, begin, begin + 1);
	}
};

class ParseDiag {
public:
	enum class Kind {
		TrailingSpace,
		MustEndInBlankLine,
	};
private:
	Kind _kind;
public:
	ParseDiag(Kind kind) : _kind(kind) {
		assert(kind == Kind::TrailingSpace || kind == Kind::MustEndInBlankLine);
	}

	friend Writer& operator<<(Writer& out, const ParseDiag& p) {
		switch (p._kind) {
			case Kind::TrailingSpace:
				return out << "trailing space";
			case Kind::MustEndInBlankLine:
				return out << "file must end in a blank line.";
		}
	}
};

struct ParseDiagnostic {
	SourceRange range;
	ParseDiag diag;
};
