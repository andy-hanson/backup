#pragma once

#include "../model/model.h"
#include "../parse/expr_ast.h"
#include "./check_common.h"

Expression check(const ExprAst& ast, ExprContext& ctx, Expected& expected);
inline Expression check_and_expect(const ExprAst& ast, ExprContext& ctx, Type expected_type) {
	Expected expected { expected_type };
	return check(ast, ctx, expected);
}
struct ExpressionAndType { Expression expression; Type type; };
inline ExpressionAndType check_and_infer(const ExprAst& ast, ExprContext& ctx) {
	Expected infer;
	Expression res = check(ast, ctx, infer);
	return { res, infer.inferred_type() };
}
