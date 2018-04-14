#pragma once

#include "../../model/model.h"

#include "./check_common.h"

inline Type convert_type(const TypeAst& ast, ExprContext& ctx) {
	Effect e __attribute__((unused)) = ast.effect;
	Option<const ref<const StructDeclaration>&> s __attribute__((unused)) = ctx.structs_table.get(ast.type_name);
	DynArray<TypeAst> ta __attribute__((unused)) = ast.type_arguments;
	throw "todo";
}

inline DynArray<Type> convert_type_arguments(const DynArray<TypeAst>& type_arguments, ExprContext& ctx) {
	return ctx.arena.map_array<TypeAst, Type>(type_arguments)([&ctx](const TypeAst& t) { return convert_type(t, ctx); });
}
