#pragma once

#include "../../util/store/ListBuilder.h"
#include "../diag/diag.h"
#include "../model/BuiltinTypes.h"
#include "../model/model.h"
#include "../parse/ast.h"

// builtin_types will be filled in for the first module checked.
void check(Ref<Module> m, Option<BuiltinTypes>& builtlin_types, const FileAst& ast, Arena& arena, ListBuilder<Diagnostic>& diags);
