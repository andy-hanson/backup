#include "convert_type.h"

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

Type type_from_ast(const TypeAst& ast, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	Option<const TypeParameter&> tp = find_in_either(type_parameters_scope.outer, type_parameters_scope.inner, [&](const TypeParameter& t) { return t.name == ast.type_name; });
	if (tp) {
		if (ast.effect != Effect::Pure) throw "todo";
		if (!ast.type_arguments.empty()) throw "todo";
		return Type { &tp.get() };
	}

	Option<const ref<const StructDeclaration>&> op_type = structs_table.get(ast.type_name);
	if (!op_type)
		throw "todo";

	ref<const StructDeclaration> strukt = op_type.get();
	if (ast.type_arguments.size() != strukt->type_parameters.size()) throw "todo";
	return Type { PlainType { ast.effect, { strukt, type_arguments_from_asts(ast.type_arguments, arena, structs_table, type_parameters_scope) } } };
}

DynArray<Type> type_arguments_from_asts(const DynArray<TypeAst>& type_arguments, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	return arena.map_array<Type>()(type_arguments, [&](const TypeAst& t) { return type_from_ast(t, arena, structs_table, type_parameters_scope); });
}

Type parse_type(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	Arena scratch_arena;
	return type_from_ast(parse_type_ast(lexer, scratch_arena), arena, structs_table, type_parameters_scope);
}

DynArray<Type> parse_type_arguments(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	Arena scratch_arena;
	return type_arguments_from_asts(parse_type_argument_asts(lexer, scratch_arena), arena, structs_table, type_parameters_scope);
}
