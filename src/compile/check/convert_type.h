#pragma once

#include "../model/model.h"
#include "../parse/type_ast.h"
#include "./CheckCtx.h"

struct TypeParametersScope {
	Slice<TypeParameter> outer; // This may come from a `spec`, or else be empty.
	Slice<TypeParameter> inner; // This comes from the current signature.

	TypeParametersScope(Slice<TypeParameter> _inner) : outer({}), inner(_inner) {}
	TypeParametersScope(Slice<TypeParameter> _outer, Slice<TypeParameter> _inner) : outer(_outer), inner(_inner) {}
};

Type type_from_ast(const TypeAst& ast, CheckCtx& al, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope);
Slice<Type> type_arguments_from_asts(const Slice<TypeAst>& type_arguments, CheckCtx& ctx, const StructsTable& structs_table, Option<const Slice<Parameter>&> parameters, const TypeParametersScope& type_parameters_scope);
