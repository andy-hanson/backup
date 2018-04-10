#include "Lexer.h"

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

	// Returns 10 on failure
	uint64_t digit_to_number(char c) {
		return '0' <= c && c <= '9' ? uint64_t(c - '0') : 10;
	}

	uint64_t read_uint_literal(const char* &ptr) {
		uint64_t value = digit_to_number(*ptr);
		assert(0 <= value && value <= 9);
		while (true) {
			uint64_t d = digit_to_number(*ptr);
			if (d == 10) break;
			value = value * 10 + d;
			++ptr;
		}
		return value;
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
			take_space();
			return c == 'g' ? Effect::Get : c == 's' ? Effect::Set : Effect::Io;
		default:
			return Effect::Pure;
	}
}

TopLevelKeyword Lexer::take_top_level_keyword() {
	skip_blank_lines(ptr);
	switch (next()) {
		case 'c':
			expect("++");
			return TopLevelKeyword::KwCPlusPlus;
		case 's':
			expect("truct");
			return TopLevelKeyword::KwStruct;
		case 'f':
			expect("un");
			return TopLevelKeyword::KwFun;
		case '\0':
			--ptr; // TODO: shouldn't actually be necessary to do it since we should discard the Lexer..
			return TopLevelKeyword::KwEof;
		default:
			throw "todo";
	}
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

uint Lexer::skip_indent_and_indented_lines() {
	assert(_indent == 0);
	take('\n');
	take('\t');
	uint lines = 1;
	while (true) {
		skip_to_end_of_line();
		++ptr; // skip the newline
		if (!try_take('\t')) break;
		++lines;
	}
	return lines;
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
			b.add(c);
		} else {
			b.add('\n');
			for (; *ptr == '\n'; ++ptr) b.add('\n');
			if (*ptr == '\t') {
				++ptr;
			} else {
				break;
			}
		}
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
	return take_name_helper(ptr, is_type_name_continue);
}

// Take a token in an expression.
ExpressionToken Lexer::take_expression_token() {
	char c = *ptr;
	switch (c) {
		case '(':
			++ptr;
			return ExpressionToken(ExpressionToken::Kind::Lparen);

		default: {
			if (is_operator_char(c)) {
				return { ExpressionToken::Kind::Name, take_name_helper(ptr, is_operator_char) };
			} else if (is_lower_case_letter(c)) {
				return { ExpressionToken::Kind::Name, take_name_helper(ptr, is_value_name_continue) };
			} else if (is_upper_case_letter(c)) {
				return { ExpressionToken::Kind::TypeName, take_name_helper(ptr, is_type_name_continue) };
			} else if (is_digit(c)) {
				return { read_uint_literal(ptr) };
			} else {
				throw "todo";
			}
		}
	}
}
