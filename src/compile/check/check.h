#pragma once

#include "../diag/diag.h"
#include "../model/model.h"
#include "../parse/ast.h"

void check(ref<Module> m, const FileAst& ast, Arena& arena, Vec<Diagnostic>& diagnostics);
