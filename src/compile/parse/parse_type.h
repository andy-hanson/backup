#pragma once

#include "./expr_ast.h"
#include "./Lexer.h"

Slice<TypeAst> parse_type_arguments(Lexer& lexer, Arena& arena);

TypeAst parse_type(Lexer& lexer, Arena& arena);
