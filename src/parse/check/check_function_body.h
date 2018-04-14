#pragma once

#include "../../model/model.h"
#include "./check_common.h"

#include "../Lexer.h"

Expression check_function_body(
	Lexer& lexer,
	Arena& arena,
	Arena& scratch_arena,
	const FunsTable& funs_table,
	const StructsTable& structs_table,
	const DynArray<Parameter>& parameters,
	const Option<Type>& bool_type,
	const Option<Type>& string_type,
	const Option<Type>& void_type,
	const Type& return_type);
