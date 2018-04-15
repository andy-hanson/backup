#include "check_function_body.h"

#include "./check_expr.h"
#include "./parse_expr.h"

Expression check_function_body(
	Lexer& lexer,
	Arena& arena,
	Arena& scratch_arena,
	const FunsTable& funs_table,
	const StructsTable& structs_table,
	const FunDeclaration& fun,
	const BuiltinTypes& builtin_types) {
	ExprContext ctx { arena, scratch_arena, funs_table, structs_table, &fun, {}, builtin_types };
	return check_and_expect(parse_body_ast(lexer, scratch_arena, arena), ctx, fun.signature.return_type);
}
