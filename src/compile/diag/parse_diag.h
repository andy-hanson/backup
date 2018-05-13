#pragma once

#include "../../util/store/StringSlice.h"
#include "../../util/int.h"
#include "../../util/Writer.h"
#include "./SourceRange.h"

class ParseDiag {
public:
	enum class Kind {
		TrailingSpace,
		MustEndInBlankLine,
		ExpectedCharacter,
		UnexpectedCharacter,

		AssertMayNotAppearInsideArg,
		PassMayNotAppearInsideArg,
		WhenMayNotAppearInsideArg,
	};
private:
	union Data {
		char expected_character;
	};
	Kind _kind;
	Data _data;
public:
	ParseDiag(const ParseDiag& other) { *this = other; }
	void operator=(const ParseDiag& other);

	ParseDiag(Kind kind) : _kind{kind} {
		assert(kind == Kind::TrailingSpace || kind == Kind::MustEndInBlankLine);
	}
	ParseDiag(Kind kind, char c) : _kind{kind} {
		assert(kind == Kind::ExpectedCharacter || kind == Kind::UnexpectedCharacter);
		_data.expected_character = c;
	}

	void write(Writer& out) const;
};

struct ParseDiagnostic {
	SourceRange range;
	ParseDiag diag;
};
