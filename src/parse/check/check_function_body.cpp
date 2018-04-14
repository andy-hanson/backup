#include "check_function_body.h"

#include "./check_expr.h"
#include "./parse_expr.h"

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
	const Type& return_type) {
	ExprContext ctx { arena, scratch_arena, funs_table, structs_table, parameters, {}, bool_type, string_type, void_type };
	return check_and_expect(parse_body_ast(lexer, scratch_arena, arena), ctx, return_type);
}
