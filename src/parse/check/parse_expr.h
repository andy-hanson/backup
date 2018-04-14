#pragma once

#include "../Lexer.h"

#include "./expr_ast.h"

ExprAst parse_body_ast(Lexer& lexer, Arena& expr_ast_arena, Arena& literals_arena);
