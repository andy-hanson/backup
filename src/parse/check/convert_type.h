#pragma once

#include "../../model/model.h"
#include "../Lexer.h"
#include "./check_common.h"

struct TypeAst {
	Effect effect;
	StringSlice type_name;
	DynArray<TypeAst> type_arguments;
};

struct TypeParametersScope {
	DynArray<TypeParameter> outer; // This may come from a `spec`, or else be empty.
	DynArray<TypeParameter> inner; // This comes from the current signature.

	TypeParametersScope(DynArray<TypeParameter> _inner) : outer({}), inner(_inner) {}
	TypeParametersScope(DynArray<TypeParameter> _outer, DynArray<TypeParameter> _inner) : outer(_outer), inner(_inner) {}
};

Type parse_type(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
DynArray<Type> parse_type_arguments(Lexer& lexer, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);

TypeAst parse_type_ast(Lexer& lexer, Arena& arena);
DynArray<TypeAst> parse_type_argument_asts(Lexer& lexer, Arena& arena);

Type type_from_ast(const TypeAst& ast, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
DynArray<Type> type_arguments_from_asts(const DynArray<TypeAst>& type_arguments, Arena& arena, const StructsTable& structs_table, const TypeParametersScope& type_parameters_scope);
