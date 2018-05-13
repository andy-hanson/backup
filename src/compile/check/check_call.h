#pragma once

#include "../model/model.h"
#include "../parse/ast.h"
#include "../../util/store/collection_util.h"

#include "check_expr_call_common.h"

ExpressionAndLifetime check_call(const StringSlice& fun_name, const Slice<ExprAst>& argument_asts, const Slice<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected);
