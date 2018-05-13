#pragma once

#include "../model/model.h"
#include "../parse/expr_ast.h"
#include "./check_expr_call_common.h"
#include "./check_lifetime.h"

ExpressionAndLifetime check_expr(const ExprAst& ast, ExprContext& ctx, Expected& expected);
inline ExpressionAndLifetime check_and_expect_stored_type(const ExprAst& ast, ExprContext& ctx, const StoredType& expected_type) {
	Expected expected { expected_type };
	return check_expr(ast, ctx, expected);
}
inline Expression check_and_expect_stored_type_and_lifetime(const ExprAst& ast, ExprContext& ctx, const Type& expected) {
	ExpressionAndLifetime inferred = check_and_expect_stored_type(ast, ctx, expected.stored_type());
	check_return_lifetime(expected.lifetime(), inferred.lifetime);
	return inferred.expression;
}
struct ExpressionAndType { Expression expression; Type type; };
inline ExpressionAndType check_and_infer(const ExprAst& ast, ExprContext& ctx) {
	Expected infer;
	ExpressionAndLifetime res = check_expr(ast, ctx, infer);
	return { res.expression, Type { infer.inferred_stored_type(), res.lifetime } };
}
