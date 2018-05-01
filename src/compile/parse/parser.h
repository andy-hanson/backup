#pragma once

#include "../../host/Path.h"
#include "../../util/Alloc.h"
#include "./ast.h"

// May throw a ParseDiagnostic.
void parse_file(FileAst& ast, PathCache& path_cache, Arena& arena);
