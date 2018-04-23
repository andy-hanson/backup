#include "convert_type.h"

Type type_from_ast(const TypeAst& ast, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	if (ast.is_type_parameter) {
		assert(ast.type_arguments.empty());
		Option<ref<const TypeParameter>> tp = find_in_either(type_parameters_scope.outer, type_parameters_scope.inner, [&](const TypeParameter& t) { return t.name == ast.type_name; });
		if (!tp) {
			al.diag(ast.type_name, Diag::Kind::TypeParameterNameNotFound);
			return Type::bogus();
		}
		return Type { tp.get() };
	} else {
		Option<const ref<const StructDeclaration>&> op_struct = structs_table.get(ast.type_name);
		if (!op_struct) {
			al.diag(ast.type_name, Diag::Kind::StructNameNotFound);
			return Type::bogus();
		}
		ref<const StructDeclaration> strukt = op_struct.get();
		if (ast.type_arguments.size() != strukt->type_parameters.size()) throw "todo";
		return Type { InstStruct { strukt, type_arguments_from_asts(ast.type_arguments, al, structs_table, type_parameters_scope) } };
	}
}

Arr<Type> type_arguments_from_asts(const Arr<TypeAst>& type_arguments, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	return al.arena.map<Type>()(type_arguments, [&](const TypeAst& t) { return type_from_ast(t, al, structs_table, type_parameters_scope); });
}
