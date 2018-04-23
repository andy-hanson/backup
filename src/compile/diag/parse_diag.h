#pragma once

#include <string>
#include <sys/types.h>

#include "../../util/StringSlice.h"
#include "../../util/int.h"
#include "../../util/Writer.h"

class ParseDiag {
public:
	enum class Kind {
		TrailingSpace,
		MustEndInBlankLine,
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
	ParseDiag(const ParseDiag& other) {
		*this = other;
	}
	void operator=(const ParseDiag& other) {
		_kind = other._kind;
		switch (_kind) {
			case Kind::TrailingSpace:
			case Kind::MustEndInBlankLine:
			case Kind::WhenMayNotAppearInsideArg:
				break;
			case Kind::ExpectedCharacter:
			case Kind::UnexpectedCharacter:
				_data.expected_character = other._data.expected_character;
				break;
		}
	}

	ParseDiag(Kind kind) : _kind(kind) {
		assert(kind == Kind::TrailingSpace || kind == Kind::MustEndInBlankLine);
	}
	ParseDiag(Kind kind, char c) : _kind(kind) {
		assert(kind == Kind::ExpectedCharacter || kind == Kind::UnexpectedCharacter);
		_data.expected_character = c;
	}

	friend Writer& operator<<(Writer& out, const ParseDiag& p) {
		switch (p._kind) {
			case Kind::TrailingSpace:
				return out << "trailing space";
			case Kind::MustEndInBlankLine:
				return out << "file must end in a blank line.";
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
