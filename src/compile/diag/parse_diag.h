#pragma once

#include <string>
#include <sys/types.h>

#include "../../util/StringSlice.h"
#include "../../util/int.h"
#include "../emit/Writer.h"

class ParseDiag {
public:
	enum class Kind {
		TrailingSpace,
		MustEndInBlankLine,
		TrailingTypeParametersAtEndOfFile,
		ExpectedCharacter,
		UnexpectedCharacter,

		WhenMayNotAppearInsideArg,
	};
private:
	union Data {
		char expected_character;
	};
	Kind _kind;
	Data _data;
public:
	ParseDiag(Kind kind) : _kind(kind) {
		assert(kind == Kind::TrailingSpace || kind == Kind::MustEndInBlankLine);
	}
	ParseDiag(Kind kind, char c) {
		assert(kind == Kind::ExpectedCharacter || kind == Kind::UnexpectedCharacter);
		_data.expected_character = c;
	}

	friend Writer& operator<<(Writer& out, const ParseDiag& p) {
		switch (p._kind) {
			case Kind::TrailingSpace:
				return out << "trailing space";
			case Kind::MustEndInBlankLine:
				return out << "file must end in a blank line.";
			case Kind::TrailingTypeParametersAtEndOfFile:
				return out << "trailing type parameters";
			case Kind::ExpectedCharacter:
				return out << "expected '" << p._data.expected_character << "'";
			case Kind::UnexpectedCharacter:
				return out << "Did not expect '" << p._data.expected_character << "'";

			case Kind::WhenMayNotAppearInsideArg:
				return out << "'when' may not appear inside an argument";
		}
	}
};

struct ParseDiagnostic {
	SourceRange range;
	ParseDiag diag;
};
