#pragma once

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "../../util/Vec.h"
#include "../diag/parse_diag.h"
#include "./ast.h"

// May throw a ParseDiagnostic.
void parse_file(FileAst& ast, PathCache& path_cache, Arena& arena);
