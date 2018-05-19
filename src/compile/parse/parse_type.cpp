#include "./parse_type.h"

#include "../../util/store/ArenaArrayBuilders.h"

namespace {
	StoredTypeAst parse_stored_type(Lexer& lexer, Arena& arena) {
		bool is_type_parameter = lexer.try_take('?');
		StringSlice name = lexer.take_type_name();
		return is_type_parameter ? StoredTypeAst { name } : StoredTypeAst { name, parse_type_arguments(lexer, arena) };
	}
}

Slice<TypeAst> parse_type_arguments(Lexer& lexer, Arena& arena) {
	if (!lexer.try_take('<'))
		return {};
	MaxSizeVector<4, TypeAst> args;
	do { args.push(parse_type(lexer, arena)); } while (lexer.try_take_comma_space());
	lexer.take('>');
	return to_arena(args, arena);
}

TypeAst parse_type(Lexer& lexer, Arena& arena) {
	StoredTypeAst s = parse_stored_type(lexer, arena);
	MaxSizeVector<4, LifetimeConstraintAst> lifetimes;
	// Parse lifetimes
	if (lexer.try_take(' ')) {
		lexer.take('*');
		LifetimeConstraintAst::Kind kind = lexer.try_take('?') ? LifetimeConstraintAst::Kind::LifetimeVariableName : LifetimeConstraintAst::Kind::ParameterName;
		StringSlice name = lexer.take_value_name();
		lifetimes.push(LifetimeConstraintAst { kind, name });
	}
	return { s, to_arena(lifetimes, arena) };
}
