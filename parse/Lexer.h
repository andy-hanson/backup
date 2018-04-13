#pragma once

#include "../model/model.h" // Effect
#include "../util/StringSlice.h"

struct ExpressionToken {
	enum class Kind {
		Name, TypeName, Literal, Lparen, As, When
	};
	Kind kind;
	union {
		StringSlice name;
		ArenaString literal;
	};
};

enum class TopLevelKeyword { KwCPlusPlus, KwStruct, KwFun, KwEof };

class Lexer {
	const char* ptr;
	const char* end; // ptr is null-terminated, so used only. to get a maximum size on string allocations
	uint _indent;

	inline char next() {
		char c = *ptr;
		++ptr;
		return c;
	}

	void expect(const char* expected);
	uint take_tabs();

	uint skip_indented_lines();

public:
	Lexer(StringSlice slice) : ptr(slice.begin()), end(slice.end()), _indent(0) {}
	inline void reset(StringSlice slice) {
		assert(_indent == 0);
		ptr = slice.begin();
		assert(slice.end() == end);
	}

	void take(char expected);
	inline bool try_take(char expected) {
		if (*ptr == expected) {
			++ptr;
			return true;
		} else
			return false;
	}

	inline uint indent() { return _indent; } // used for debugging

	Effect try_take_effect();
	TopLevelKeyword take_top_level_keyword();

	bool try_take_else();
	inline bool try_take_comma_space() {
		if (try_take(',')) {
			take(' ');
			return true;
		}
		return false;
	}

	inline void reduce_indent_by_2() {
		assert(_indent >= 2);
		_indent -= 2;
	}

	void take_dedent();
	void take_newline_same_indent();
	void take_indent();
	void skip_to_end_of_line();
	uint skip_indent_and_indented_lines();
	void skip_to_end_of_line_and_optional_indented_lines();
	void skip_to_end_of_line_and_indented_lines();
	ArenaString take_indented_string(Arena& arena);

	StringSlice take_value_name();
	StringSlice take_type_name();
	StringSlice take_cpp_type_name();

	// arena used to place literals.
	ExpressionToken take_expression_token(Arena& arena);
};
