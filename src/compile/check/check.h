#pragma once

#include "../model/model.h"
#include "../parse/ast.h"

void check(ref<Module> m, const StringSlice& source, const Vec<DeclarationAst>& declarations, Arena& arena);
