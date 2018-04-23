#include "./parse_type.h"

Arr<TypeAst> parse_type_argument_asts(Lexer& lexer, Arena& arena) {
	if (!lexer.try_take('<'))
		return {};
	auto args = arena.small_array_builder<TypeAst>();
	do { args.add(parse_type_ast(lexer, arena)); } while (lexer.try_take_comma_space());
	lexer.take('>');
	return args.finish();
}

TypeAst parse_type_ast(Lexer& lexer, Arena& arena) {
	bool is_type_parameter = lexer.try_take('?');
	StringSlice name = lexer.take_type_name();
	return { is_type_parameter, name, is_type_parameter ? Arr<TypeAst>{} : parse_type_argument_asts(lexer, arena) };
}
