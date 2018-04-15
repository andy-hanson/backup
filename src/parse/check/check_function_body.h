#pragma once

#include "../../model/model.h"
#include "./check_common.h"

#include "../Lexer.h"

#include "./BuiltinTypes.h"

Expression check_function_body(
	Lexer& lexer,
	Arena& arena,
	Arena& scratch_arena,
	const FunsTable& funs_table,
	const StructsTable& structs_table,
	const FunDeclaration& fun,
	const BuiltinTypes& builtin_types);
