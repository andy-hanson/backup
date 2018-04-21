#include "convert_type.h"

Type type_from_ast(const TypeAst& ast, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	Option<ref<const TypeParameter>> tp = find_in_either(type_parameters_scope.outer, type_parameters_scope.inner, [&](const TypeParameter& t) { return t.name == ast.type_name; });
	if (tp) {
		if (ast.effect != Effect::Pure) throw "todo";
		if (!ast.type_arguments.empty()) throw "todo";
 		return Type { tp.get() };
	}

	Option<const ref<const StructDeclaration>&> op_type = structs_table.get(ast.type_name);
	if (!op_type)
		throw "todo";

	ref<const StructDeclaration> strukt = op_type.get();
	if (ast.type_arguments.size() != strukt->type_parameters.size()) throw "todo";
	return Type { PlainType { ast.effect, { strukt, type_arguments_from_asts(ast.type_arguments, al, structs_table, type_parameters_scope) } } };
}

DynArray<Type> type_arguments_from_asts(const DynArray<TypeAst>& type_arguments, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	return al.arena.map<Type>()(type_arguments, [&](const TypeAst& t) { return type_from_ast(t, al, structs_table, type_parameters_scope); });
}
