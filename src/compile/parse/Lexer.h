#pragma once

#include "../diag/parse_diag.h"
#include "../model/effect.h"
#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"

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

enum class TopLevelKeyword { None, KwStruct, KwSpec, KwCpp, KwCppInclude, KwCppStruct, KwEof };
enum class NewlineOrDedent { Newline, Dedent };

class Lexer {
	const char* ptr;
	const char* end; // ptr is null-terminated, so used only to get a maximum size on string allocations
	uint _indent;

	inline char next() {
		char c = *ptr;
		++ptr;
		return c;
	}

	void expect(const char* expected);
	uint take_tabs();

public:
	// Throws a ParseDiagnostic on failure.
	static void validate_file(const StringSlice& source);

	Lexer(StringSlice slice) : ptr(slice.begin()), end(slice.end()), _indent(0) {}

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
	TopLevelKeyword try_take_top_level_keyword();

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

	void skip_blank_lines();

	NewlineOrDedent take_newline_or_dedent();
	void take_dedent();

	void take_newline_same_indent();
	void take_indent();
	StringSlice take_indented_string(Arena& arena);

	StringSlice take_type_name();
	StringSlice take_spec_name();
	StringSlice take_value_name();
	StringSlice take_cpp_type_name();

	// arena used to place literals.
	ExpressionToken take_expression_token(Arena& arena);
};
