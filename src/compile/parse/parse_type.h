#pragma once

#include "./expr_ast.h"
#include "./Lexer.h"

Arr<TypeAst> parse_type_argument_asts(Lexer& lexer, Arena& arena);

TypeAst parse_type_ast(Lexer& lexer, Arena& arena);
