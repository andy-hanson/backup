#include "./parse_type.h"

DynArray<TypeAst> parse_type_argument_asts(Lexer& lexer, Arena& arena) {
	if (!lexer.try_take('<'))
		return {};
	auto args = arena.small_array_builder<TypeAst>();
	do { args.add(parse_type_ast(lexer, arena)); } while (lexer.try_take_comma_space());
	lexer.take('>');
	return args.finish();
}

TypeAst parse_type_ast(Lexer& lexer, Arena& arena) {
	Effect effect = lexer.try_take_effect();
	StringSlice name = lexer.take_type_name();
	DynArray<TypeAst> its_args = parse_type_argument_asts(lexer, arena);
	return { effect, name, its_args };
}
