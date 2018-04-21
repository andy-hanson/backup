#pragma once

#include <vector>
#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"
#include "./ast.h"

std::vector<DeclarationAst> parse_file(const StringSlice& file_content, Arena& arena);
