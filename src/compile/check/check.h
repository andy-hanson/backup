#pragma once

#include "../../util/List.h"
#include "../diag/diag.h"
#include "../model/model.h"
#include "../parse/ast.h"

void check(Ref<Module> m, const FileAst& ast, Arena& arena, List<Diagnostic>::Builder& diags);
