#pragma once

#include "../../util/store/ListBuilder.h"
#include "../diag/diag.h"
#include "../model/model.h"
#include "../parse/ast.h"

void check(Ref<Module> m, const FileAst& ast, Arena& arena, ListBuilder<Diagnostic>& diags);
