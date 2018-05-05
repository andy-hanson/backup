#include "./parse_type.h"

#include "../../util/ArenaArrayBuilders.h"

Slice<TypeAst> parse_type_argument_asts(Lexer& lexer, Arena& arena) {
	if (!lexer.try_take('<'))
		return {};
	SmallArrayBuilder<TypeAst> args;
	do { args.add(parse_type_ast(lexer, arena)); } while (lexer.try_take_comma_space());
	lexer.take('>');
	return args.finish(arena);
}

TypeAst parse_type_ast(Lexer& lexer, Arena& arena) {
	bool is_type_parameter = lexer.try_take('?');
	StringSlice name = lexer.take_type_name();
	return is_type_parameter ? TypeAst { name } : TypeAst { name, parse_type_argument_asts(lexer, arena) };
}
