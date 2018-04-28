#include "parse_diag.h"

void ParseDiag::operator=(const ParseDiag& other) {
	_kind = other._kind;
	switch (_kind) {
		case Kind::TrailingSpace:
		case Kind::MustEndInBlankLine:
		case Kind::AssertMayNotAppearInsideArg:
		case Kind::PassMayNotAppearInsideArg:
		case Kind::WhenMayNotAppearInsideArg:
			break;
		case Kind::ExpectedCharacter:
		case Kind::UnexpectedCharacter:
			_data.expected_character = other._data.expected_character;
			break;
	}
}

void ParseDiag::write(Writer& out) const {
	switch (_kind) {
		case ParseDiag::Kind::TrailingSpace:
			out << "trailing space";
			break;
		case ParseDiag::Kind::MustEndInBlankLine:
			out << "file must end in a blank line.";
			break;
		case ParseDiag::Kind::ExpectedCharacter:
			out << "expected '" << _data.expected_character << "'";
			break;
		case ParseDiag::Kind::UnexpectedCharacter:
			out << "Did not expect '" << _data.expected_character << "'";
			break;
		case ParseDiag::Kind::AssertMayNotAppearInsideArg:
			out << "'assert' may not appear inside an argument";
			break;
		case ParseDiag::Kind::PassMayNotAppearInsideArg:
			out << "'pass' may not appear inside an argument";
			break;
		case ParseDiag::Kind::WhenMayNotAppearInsideArg:
			out << "'when' may not appear inside an argument";
			break;
	}
}
