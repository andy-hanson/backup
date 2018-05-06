#pragma once

#include "../../util/store/Arena.h"
#include "../../util/Path.h"
#include "./ast.h"

// May throw a ParseDiagnostic.
void parse_file(FileAst& ast, PathCache& path_cache, Arena& arena);
