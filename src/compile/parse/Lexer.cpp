#include "./Lexer.h"

namespace {
	bool is_operator_char(char c) {
		switch (c) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '<':
			case '>':
			case '=':
				return true;
			default:
				return false;
		}
	}

	bool is_lower_case_letter(char c) {
		return 'a' <= c && c <= 'z';
	}
	bool is_upper_case_letter(char c) {
		return 'A' <= c && c <= 'Z';
	}
	bool is_digit(char c) {
		return '0' <= c && c <= '9';
	}

	bool is_value_name_continue(char c) {
		return is_lower_case_letter(c) || c == '-';
	}
	bool is_type_name_continue(char c) {
		return is_upper_case_letter(c) || is_lower_case_letter(c);
	}

	ArenaString take_string_literal(const char* &ptr, StringBuilder b) {
		++ptr;
		while (*ptr != '"' && *ptr != '\0') {
			//TODO:ESCAPING
			b << *ptr;
			++ptr;
		}
		if (*ptr != '\0') ++ptr;
		return b.finish();
	}

	// Literals are allowed to be written as +123.456.789 instead of "+123.456.789" since numeric literals are common.
	ArenaString take_numeric_literal(const char* &ptr, Arena& arena) {
		const char* begin = ptr;
		while (is_digit(*ptr) || *ptr == '.') ++ptr;
		return copy_string(arena, { begin, ptr });
	}

	// Assumes the first char is validated already.
	StringSlice take_name_helper(const char* begin, const char* &ptr, bool next_char_pred(char)) {
		assert(ptr > begin);
		while (next_char_pred(*ptr)) ++ptr;
		return { begin, ptr };
	}
}

ParseDiagnostic Lexer::unexpected() {
	return diag_at_char({ ParseDiag::Kind::UnexpectedCharacter, *ptr });
}

void Lexer::validate_file(const StringSlice& source) {
	assert(!source.is_empty() && *(source.end() - 1) == '\0'); // Should be guaranteed by file reader

	// Look for trailing whitespace.
	for (const char* ptr = source.begin() + 1; *ptr != '\0'; ++ptr)
		if (*ptr == '\n' && (*(ptr - 1) == ' ' || *(ptr - 1) == '\t'))
			throw ParseDiagnostic { SourceRange::inner_slice(source, { ptr - 1, ptr }), ParseDiag::Kind::TrailingSpace };

	if (source.size() == 1 || *(source.end() - 2) != '\n')
		throw ParseDiagnostic { SourceRange::inner_slice(source, { source.end() - 1, source.end() }), ParseDiag::Kind::MustEndInBlankLine };
}

ParseDiagnostic Lexer::diag_at_char(ParseDiag diag) {
	return { SourceRange::inner_slice(source, { ptr, ptr + 1 }), diag };
}

StringBuilder Lexer::string_builder(Arena& arena) {
	return StringBuilder { arena, to_unsigned(source.end() - ptr) };
}

void Lexer::expect(const char* expected) {
	for (; *expected != 0; ++expected) {
		take(*expected);
	}
}

uint Lexer::take_tabs() {
	const char* const begin = ptr;
	while (*ptr == '\t')
		++ptr;
	return uint(ptr - begin);
}

void Lexer::take(char expected) {
	if (!try_take(expected))
		throw diag_at_char({ ParseDiag::Kind::ExpectedCharacter, expected });
}

bool Lexer::try_take_copy_keyword() {
	if (ptr[0] == 'c' && ptr[1] == 'o' && ptr[2] == 'p' && ptr[3] == 'y') {
		ptr += 4;
		return true;
	}
	return false;
}
bool Lexer::try_take_from_keyword() {
	if (ptr[0] == 'f' && ptr[1] == 'r' && ptr[2] == 'o' && ptr[3] == 'm') {
		ptr += 4;
		return true;
	}
	return false;
}
bool Lexer::try_take_else_keyword() {
	if (ptr[0] == 'e' && ptr[1] == 'l' && ptr[2] == 's' && ptr[3] == 'e') {
		ptr += 4;
		return true;
	}
	return false;
}
bool Lexer::try_take_import_space() {
	if (ptr[0] == 'i' && ptr[1] == 'm' && ptr[2] == 'p' && ptr[3] == 'o' && ptr[4] == 'r' && ptr[5] == 't' && ptr[6] == ' ') {
		ptr += 7;
		return true;
	}
	return false;
}
bool Lexer::try_take_private_nl() {
	if (ptr[0] == 'p' && ptr[1] == 'r' && ptr[2] == 'i' && ptr[3] == 'v' && ptr[4] == 'a' && ptr[5] == 't' && ptr[6] == 'e' && ptr[7] == '\n') {
		ptr += 8;
		return true;
	}
	return false;
}

Option<Effect> Lexer::try_take_effect() {
	const char c = *ptr;
	switch (c) {
		case 'g':
		case 's':
		case 'i':
		case 'o':
			++ptr;
			if (c == 'g' || c == 's') expect("et"); else if (c == 'i') take('o'); else expect("wn");
			take(' ');
			return Option { c == 'g' ? Effect::EGet : c == 's' ? Effect::ESet : c == 'i' ? Effect::EIo : Effect::EOwn };
		default:
			return {};
	}
}

void Lexer::skip_blank_lines() {
	while (*ptr == '\n')
		++ptr;
}

NewlineOrDedent Lexer::take_newline_or_dedent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent == _indent - 1) {
		_indent = new_indent;
		return NewlineOrDedent::Dedent;
	}
	else if (new_indent == _indent)
		return NewlineOrDedent::Newline;
	else
		throw unexpected();
}

void Lexer::take_dedent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent - 1)
		throw unexpected();
	_indent = new_indent;
}

void Lexer::take_newline_same_indent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent)
		throw unexpected();
	_indent = new_indent;
}

bool Lexer::try_take_indent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent == _indent)
		return false;
	if (new_indent != _indent + 1)
		throw unexpected();
	_indent = new_indent;
	return true;
}

void Lexer::take_indent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent + 1)
		throw unexpected();
	_indent = new_indent;
}

ArenaString Lexer::take_indented_string(Arena& arena) {
	assert(_indent == 0);
	take('\n');
	take('\t');

	assert(*ptr != '\n');

	StringBuilder b = string_builder(arena);
	// Keep eating until we see a line that begins in something other than '\n'.
	while (true) {
		char c = next();
		assert(c != '\0'); // File will end in a blank line, so this should never happen.
		if (c != '\n') {
			b << c;
			continue;
		}

		b << '\n';
		for (; *ptr == '\n'; ++ptr) b << '\n';
		if (*ptr != '\t') break;
		++ptr;
	}

	// Remove trailing newlines
	while (b.back() == '\n')
		b.pop();
	return b.finish();
}

namespace {
	StringSlice take_rest_of_line(const char* &ptr) {
		const char* start = ptr;
		while (*ptr != '\n') ++ptr;
		StringSlice res { start, ptr };
		++ptr;
		return res;
	}
}

Option<ArenaString> Lexer::try_take_comment(Arena& arena) {
	if (*ptr != '|') return {};
	StringBuilder b = string_builder(arena);
	do {
		++ptr;
		take(' ');
		if (!b.is_empty()) b << '\n';
		for (uint i = _indent; i != 0; --i) take('\t');
		b << take_rest_of_line(ptr);
	} while (*ptr == '|');
	return Option { b.finish() };
}

StringSlice Lexer::take_cpp_include() {
	const char* begin = ptr;
	while (*ptr != '\n') ++ptr;
	return StringSlice { begin, ptr };
}

StringSlice Lexer::take_type_name() {
	const char* begin = ptr;
	if (!is_upper_case_letter(*ptr)) throw unexpected();
	++ptr;
	return take_name_helper(begin, ptr, is_type_name_continue);
}

StringSlice Lexer::take_spec_name() {
	if (*ptr != '$') throw unexpected();
	++ptr;
	return take_type_name();
}

StringSlice Lexer::take_value_name() {
	const char* begin = ptr;
	if (is_operator_char(*ptr)) {
		++ptr;
		return take_name_helper(begin, ptr, is_operator_char);
	} else if (is_lower_case_letter(*ptr)) {
		++ptr;
		return take_name_helper(begin, ptr, is_value_name_continue);
	} else {
		throw unexpected();
	}
}

Lexer::ValueOrTypeName Lexer::take_value_or_type_name() {
	return is_upper_case_letter(*ptr) ? ValueOrTypeName { false, take_type_name() } : ValueOrTypeName { true, take_value_name() };
}

StringSlice Lexer::take_cpp_type_name() {
	const char* begin = ptr;
	if (!is_lower_case_letter(*ptr)) throw unexpected();
	++ptr;
	return take_name_helper(begin, ptr, [](char c) { return c != '\n' && c != '\0'; });
}

namespace {
	const StringSlice WHEN { "when" };
	const StringSlice ASSERT { "assert" };
	const StringSlice PASS { "pass" };
}

// Take a token in an expression.
ExpressionToken Lexer::take_expression_token(Arena& arena) {
	const char* begin = ptr;
	char c = *ptr;
	if (c == '(') {
		++ptr;
		return { ExpressionToken::Kind::Lparen, {} };
	} else if (c == '"') {
		return { ExpressionToken::Kind::Literal, { take_string_literal(ptr, string_builder(arena)) } };
	} else if (is_operator_char(c)) {
		++ptr;
		return { ExpressionToken::Kind::Name, { take_name_helper(begin, ptr, is_operator_char) } };
	} else if (is_lower_case_letter(c)) {
		++ptr;
		StringSlice name = take_name_helper(begin, ptr, is_value_name_continue);
		ExpressionToken::Kind kind = name == WHEN ? ExpressionToken::Kind::When
			: name == PASS ? ExpressionToken::Kind::Pass
			: name == ASSERT ? ExpressionToken::Kind::Assert
			: ExpressionToken::Kind::Name;
		return { kind, { name } };
	} else if (is_upper_case_letter(c)) {
		++ptr;
		return { ExpressionToken::Kind::TypeName, { take_name_helper(begin, ptr, is_type_name_continue) } };
	} else if (is_digit(c) || c == '+' || c == '-') {
		return { ExpressionToken::Kind::Literal, { take_numeric_literal(ptr, arena) } };
	} else {
		throw unexpected();
	}
}
