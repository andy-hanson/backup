#pragma once

#include "../model/model.h" // Effect
#include "../util/StringSlice.h"

class ExpressionToken {
public:
	enum class Kind { Name, TypeName, UintLiteral, Lparen, When };

private:
	union Data {
		StringSlice name; // Goes with ExpressionTokenKind::Name
		uint64_t uint_literal; // Goes with ExpressionTokenKind::UintLiteral
		Data() {} // uninitialized
	};

	Kind _kind;
	Data data;

public:
	ExpressionToken(Kind kind) : _kind(kind) {
		assert(_kind == Kind::Lparen || _kind == Kind::When);
	}
	ExpressionToken(Kind kind, StringSlice name) : _kind(kind) {
		assert(_kind == Kind::Name || _kind == Kind::TypeName);
		data.name = name;
	}
	ExpressionToken(uint64_t literal) : _kind(Kind::UintLiteral) {
		data.uint_literal = literal;
	}

	Kind kind() { return _kind; }

	StringSlice name() {
		assert(_kind == Kind::Name || _kind == Kind::TypeName);
		return data.name;
	}

	uint64_t uint_literal() {
		assert(_kind == Kind::UintLiteral);
		return data.uint_literal;
	}
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

	inline bool try_take(char expected) {
		if (*ptr == expected) {
			++ptr;
			return true;
		} else
			return false;
	}

	void take(char expected);

public:
	Lexer(const char* _ptr) : ptr(_ptr), _indent(0) {}
	inline void reset(const char* _ptr) {
		assert(_indent == 0);
		ptr = _ptr;
	}

	inline uint indent() { return _indent; } // used for debugging

	Effect try_take_effect();
	TopLevelKeyword take_top_level_keyword();

	inline void take_space() { take(' '); }
	inline void take_lparen() { take('('); }
	inline void take_rparen() { take(')'); }
	inline void take_comma() { take(','); }
	inline void take_equals() { take('='); }
	inline void take_less() { take('<'); }
	inline void take_greater() { take('>'); }
	inline bool try_take_rparen() { return try_take(')'); }
	inline bool try_take_equals() { return try_take('='); }
	inline bool try_take_space() { return try_take(' '); }
	inline bool try_take_dot() { return try_take('.'); }
	inline bool try_take_less() { return try_take('<'); }
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
	void skip_to_end_of_line_and_indented_lines();
	ArenaString take_indented_string(Arena& arena);

	StringSlice take_value_name();
	StringSlice take_type_name();
	StringSlice take_cpp_type_name();

	ExpressionToken take_expression_token();
};
