#pragma once

#include "../model/model.h"
#include "../parse/ast.h"

ref<Module> check(const StringSlice& file_path, const Identifier& module_name, const std::vector<DeclarationAst>& declarations, Arena& arena);
