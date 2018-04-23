#pragma once

#include "../model/model.h"
#include "../parse/type_ast.h"
#include "./check_common.h"

struct TypeParametersScope {
	Arr<TypeParameter> outer; // This may come from a `spec`, or else be empty.
	Arr<TypeParameter> inner; // This comes from the current signature.

	TypeParametersScope(Arr<TypeParameter> _inner) : outer({}), inner(_inner) {}
	TypeParametersScope(Arr<TypeParameter> _outer, Arr<TypeParameter> _inner) : outer(_outer), inner(_inner) {}
};

Type type_from_ast(const TypeAst& ast, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
Arr<Type> type_arguments_from_asts(const Arr<TypeAst>& type_arguments, Al& al, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
