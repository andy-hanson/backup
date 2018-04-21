#pragma once

#include "../model/model.h"
#include "../parse/ast.h"
#include "../../util/collection_util.h"

#include "./check_common.h"

Expression check_call(const StringSlice& fun_name, const DynArray<ExprAst>& argument_asts, const DynArray<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected);
