#include "Lexer.h"
#include "../diag/parse_diag.h"

void Lexer::validate_file(const StringSlice& source) {
	if (source.empty()) return;

	for (const char* ptr = source.begin(); *ptr != '\0'; ++ptr)
		if (*ptr == '\n' && (*(ptr - 1) == ' ' || *(ptr - 1) == '\t'))
			throw ParseDiagnostic { SourceRange::from_pointer(source, ptr), ParseDiag::Kind::TrailingSpace };

	if (*(source.end() - 1) != '\n')
		throw ParseDiagnostic { SourceRange::from_pointer(source, source.end() - 1), ParseDiag::Kind::MustEndInBlankLine };

	return;
}

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

	void skip_blank_lines(const char* &ptr) {
		while (*ptr == '\n')
			++ptr;
	}

	ArenaString take_string_literal(const char* &ptr, const char* end, Arena& arena) {
		Arena::StringBuilder b = arena.string_builder(to_unsigned(end - ptr));
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
		return arena.str({ begin, ptr });
	}

	// Assumes the first char is validated already.
	StringSlice take_name_helper(const char* &ptr, bool next_char_pred(char)) {
		const char* begin = ptr;
		++ptr;
		while (next_char_pred(*ptr)) ++ptr;
		return { begin, ptr };
	}
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
		throw "todo";
}

bool Lexer::try_take_else() {
	if (ptr[0] == 'e' && ptr[1] == 'l' && ptr[2] == 's' && ptr[3] == 'e') {
		ptr += 4;
		return true;
	}
	return false;
}

Effect Lexer::try_take_effect() {
	const char c = *ptr;
	switch (c) {
		case 'g':
		case 's':
		case 'i':
			++ptr;
			if (c == 'i') take('o'); else expect("et");
			take(' ');
			return c == 'g' ? Effect::Get : c == 's' ? Effect::Set : Effect::Io;
		default:
			return Effect::Pure;
	}
}

TopLevelKeyword Lexer::try_take_top_level_keyword() {
	skip_blank_lines(ptr);
	switch (*ptr) {
		case 'c':
			++ptr;
			expect("pp");
			take(' ');
			switch (*ptr) {
				case 'i':
					++ptr;
					expect("nclude");
					return TopLevelKeyword::KwCppInclude;
				case 's':
					++ptr;
					expect("truct");
					return TopLevelKeyword::KwCppStruct;
				default:
					return TopLevelKeyword::KwCpp;
			}
		case 's':
			++ptr;
			expect("truct");
			return TopLevelKeyword::KwStruct;
		case '\0':
			return TopLevelKeyword::KwEof;
		default:
			return TopLevelKeyword::None;
	}
}

NewlineOrDedent Lexer::take_newline_or_dedent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent == _indent - 1) {
		_indent = new_indent;
		return NewlineOrDedent::Dedent;
	}
	if (new_indent == _indent) {
		return NewlineOrDedent::Newline;
	}
	throw "todo";
}

void Lexer::take_dedent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent - 1)
		throw "todo";
	_indent = new_indent;
}

void Lexer::take_newline_same_indent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent)
		throw "todo";
	_indent = new_indent;
}

void Lexer::take_indent() {
	take('\n');
	uint new_indent = take_tabs();
	if (new_indent != _indent + 1)
		throw "todo";
	_indent = new_indent;
}

void Lexer::skip_to_end_of_line() {
	while (*ptr != '\n') ++ptr;
}

// Should match the behavior of take_indented_string (but without writing out a result)
uint Lexer::skip_indented_lines() {
	assert(_indent == 0);
	uint lines = 1;
	while (true) {
		skip_to_end_of_line();
		skip_blank_lines(ptr);
		if (!try_take('\t')) break;
		++lines;
	}
	return lines;
}

uint Lexer::skip_indent_and_indented_lines() {
	assert(_indent == 0);
	take('\n');
	take('\t');
	return skip_indented_lines();
}

void Lexer::skip_to_end_of_line_and_optional_indented_lines() {
	assert(_indent == 0);
	skip_to_end_of_line();
	take('\n');
	skip_blank_lines(ptr);
	if (try_take('\t')) {
		skip_indented_lines();
	}
}

void Lexer::skip_to_end_of_line_and_indented_lines() {
	skip_to_end_of_line();
	skip_indent_and_indented_lines();
}

//Problem: what about a blank line inside the indented string?
//Soln: this method needs to be the one responsible for skipping blank lines afterward.
ArenaString Lexer::take_indented_string(Arena& arena) {
	assert(_indent == 0);
	take('\n');
	take('\t');

	if (*ptr == '\n') throw "todo";

	Arena::StringBuilder b = arena.string_builder(to_unsigned(end - ptr));

	// Keep eating until we see a line that begins in something other than '\n'.
	while (true) {
		char c = next();
		if (c == '\0') throw "todo"; // file ought to end in a blank line, complain
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

StringSlice Lexer::take_value_name() {
	if (is_operator_char(*ptr)) {
		return take_name_helper(ptr, is_operator_char);
	} else if (is_lower_case_letter(*ptr)) {
		return take_name_helper(ptr, is_value_name_continue);
	} else {
		throw "todo";
	}
}

StringSlice Lexer::take_type_name() {
	if (!is_upper_case_letter(*ptr)) {
		throw "todo";
	}
	return take_name_helper(ptr, is_type_name_continue);
}

StringSlice Lexer::take_cpp_type_name() {
	//TODO: allow more
	if (!is_lower_case_letter(*ptr)) throw "todo";
	return take_name_helper(ptr, [](char c) { return c != '\n' && c != '\0'; });
}

namespace {
	const StringSlice AS { "as" };
	const StringSlice WHEN { "when" };
}

// Take a token in an expression.
ExpressionToken Lexer::take_expression_token(Arena& arena) {
	char c = *ptr;
	if (c == '(') {
		return { ExpressionToken::Kind::Lparen, {} };
	} else if (c == '"') {
		return { ExpressionToken::Kind::Literal, { take_string_literal(ptr, end, arena) } };
	} else if (is_operator_char(c)) {
		return { ExpressionToken::Kind::Name, { take_name_helper(ptr, is_operator_char) } };
	} else if (is_lower_case_letter(c)) {
		StringSlice name = take_name_helper(ptr, is_value_name_continue);
		return { name == AS ? ExpressionToken::Kind::As : name == WHEN ? ExpressionToken::Kind::When : ExpressionToken::Kind::Name, { name } };
	} else if (is_upper_case_letter(c)) {
		return { ExpressionToken::Kind::TypeName, { take_name_helper(ptr, is_type_name_continue) } };
	} else if (is_digit(c) || c == '+' || c == '-') {
		return { ExpressionToken::Kind::Literal, { take_numeric_literal(ptr, arena) } };
	} else {
		throw "todo";
	}
}
