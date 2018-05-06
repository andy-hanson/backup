#include "./convert_type.h"

#include "./scope.h"

namespace {
	Type type_from_parameter(const StringSlice& name, CheckCtx& ctx, const TypeParametersScope& type_parameters_scope) {
		Option<Ref<const TypeParameter>> tp = find_in_either(type_parameters_scope.outer, type_parameters_scope.inner, [&](const TypeParameter& t) { return t.name == name; });
		if (!tp.has()) {
			ctx.diag(name, Diag::Kind::TypeParameterNameNotFound);
			return Type::bogus();
		}
		return Type { tp.get() };
	}

	Type type_from_struct(const StringSlice& name, const Slice<TypeAst>& type_arguments, CheckCtx& ctx, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
		Option<Ref<const StructDeclaration>> op_strukt = find_struct(name, ctx, structs_table);
		if (!op_strukt.has()) {
			ctx.diag(name, Diag::Kind::StructNameNotFound);
			return Type::bogus();
		}

		Ref<const StructDeclaration> strukt = op_strukt.get();
		if (type_arguments.size() != strukt->type_parameters.size()) todo();
		return Type { InstStruct { strukt, type_arguments_from_asts(type_arguments, ctx, structs_table, type_parameters_scope) } };
	}
}

Type type_from_ast(const TypeAst& ast, CheckCtx& ctx, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	switch (ast.kind()) {
		case TypeAst::Kind::Parameter:
			return type_from_parameter(ast.name(), ctx, type_parameters_scope);
		case TypeAst::Kind::InstStruct:
			return type_from_struct(ast.name(), ast.type_arguments(), ctx, structs_table, type_parameters_scope);
	}
}

Slice<Type> type_arguments_from_asts(const Slice<TypeAst>& type_arguments, CheckCtx& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope) {
	return map<Type>()(al.arena, type_arguments, [&](const TypeAst& t) { return type_from_ast(t, al, structs_table, type_parameters_scope); });
}
