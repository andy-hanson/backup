#pragma once

#include "./BuiltinTypes.h"
#include "./model.h"
#include "./expr.h"

Type type_of_expr(Expression e, const BuiltinTypes& builtins);
