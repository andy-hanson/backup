#pragma once

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "../../util/Vec.h"
#include "./ast.h"

Vec<DeclarationAst> parse_file(const StringSlice& file_content, Arena& arena);
