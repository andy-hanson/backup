#pragma once

#include "./Lexer.h"
#include "./expr_ast.h"

// Note: This may throw a ParseDiagnostic.
// In that case, skip until the next blank line.
ExprAst parse_body_ast(Lexer& lexer, Arena& arena);
