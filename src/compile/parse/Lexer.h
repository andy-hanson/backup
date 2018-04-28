#pragma once

#include "../diag/parse_diag.h"
#include "../model/effect.h"
#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "../../util/Vec.h"

struct ExpressionToken {
	enum class Kind {
		Name, TypeName, Literal, Lparen, As, When, Assert, Pass
	};
	Kind kind;
	union {
		StringSlice name;
		ArenaString literal;
	};
};

enum class NewlineOrDedent { Newline, Dedent };

class Lexer {
	const StringSlice source;
	const char* ptr; // current pointer into source
	uint _indent;

	inline char next() {
		char c = *ptr;
		++ptr;
		return c;
	}

	ParseDiagnostic unexpected() {
		return diag_at_char({ ParseDiag::Kind::UnexpectedCharacter, *ptr });
	}

	void expect(const char* expected);
	uint take_tabs();

	Arena::StringBuilder string_builder(Arena& arena);

public:
	// May throw a ParseDiagnostic.
	static void validate_file(const StringSlice& source);

	inline const char* at() { return ptr; }
	inline SourceRange range(const char* start) {
		return source.range_from_inner_slice({ start, ptr });
	}

	Lexer(StringSlice _source) : source(_source), ptr(_source.begin()), _indent(0) {}

	ParseDiagnostic diag_at_char(ParseDiag diag) {
		return { source.range_from_inner_slice({ ptr, ptr + 1 }), diag };
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

	Option<Effect> try_take_effect();

	bool try_take_copy_keyword();
	bool try_take_from_keyword();
	bool try_take_else_keyword();
	bool try_take_import_space();
	bool try_take_private_nl();

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
	ArenaString take_indented_string(Arena& arena);

	ArenaString try_take_comment(Arena& arena);

	StringSlice take_cpp_include();
	StringSlice take_type_name();
	StringSlice take_spec_name();
	StringSlice take_value_name();
	struct ValueOrTypeName { bool is_value; StringSlice name; };
	ValueOrTypeName take_value_or_type_name();
	StringSlice take_cpp_type_name();

	// arena used to place literals.
	ExpressionToken take_expression_token(Arena& arena);
};
