#pragma once

#include "../model/model.h"
#include "../parse/type_ast.h"
#include "./check_common.h"

struct TypeParametersScope {
	DynArray<TypeParameter> outer; // This may come from a `spec`, or else be empty.
	DynArray<TypeParameter> inner; // This comes from the current signature.

	TypeParametersScope(DynArray<TypeParameter> _inner) : outer({}), inner(_inner) {}
	TypeParametersScope(DynArray<TypeParameter> _outer, DynArray<TypeParameter> _inner) : outer(_outer), inner(_inner) {}
};

Type type_from_ast(const TypeAst& ast, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
DynArray<Type> type_arguments_from_asts(const DynArray<TypeAst>& type_arguments, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
