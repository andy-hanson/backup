#pragma once

#include "../model/model.h"
#include "parse_expr.h" // ExprAst
#include "parser_internal.h"

Expression convert(
	const ExprAst& ast,
	Arena& arena,
	Arena& scratch_arena,
	const FunsTable& funs_table,
	const StructsTable& structs_table,
	const DynArray<Parameter>& parameters,
	const Option<Type>& bool_type,
	const Option<Type>& string_type,
	const Type& return_type);
