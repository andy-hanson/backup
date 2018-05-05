#pragma once

#include "../../util/Path.h"
#include "../../util/Arena.h"
#include "./ast.h"

// May throw a ParseDiagnostic.
void parse_file(FileAst& ast, PathCache& path_cache, Arena& arena);
