#pragma once

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "../../util/Vec.h"
#include "../diag/parse_diag.h"
#include "./ast.h"

// May throw a ParseDiagnostic.
Vec<DeclarationAst> parse_file(const StringSlice& file_content, Arena& arena);
