#include "./parse_expr.h"

#include "../../util/ArenaArrayBuilders.h"
#include "./parse_type.h"

namespace {
	// In Statement allow anything; in EqualsRhs anything but `=`; in Case anything single-line.
	enum class ExprCtx { Statement, EqualsRhs, Case };

	ExprAst parse_expr_ast(Lexer& lexer, Arena& arena, ExprCtx where);
	ExprAst parse_expr_arg_ast(Lexer& lexer, Arena& arena, ExpressionToken et);
	ExprAst parse_expr_arg_ast(Lexer& lexer, Arena& arena) {
		return parse_expr_arg_ast(lexer, arena, lexer.take_expression_token(arena));
	}

	Arr<ExprAst> parse_prefix_args(Lexer& lexer, Arena& arena) {
		if (!lexer.try_take(' '))
			return {};
		SmallArrayBuilder<ExprAst> args;
		do {
			args.add(parse_expr_arg_ast(lexer, arena));
		} while (lexer.try_take_comma_space());
		return args.finish(arena);
	}

	WhenAst parse_when(Lexer& lexer, Arena& arena, const char* start) {
		lexer.take_indent();
		SmallArrayBuilder<CaseAst> cases;
		while (true) {
			if (lexer.try_take_else_keyword()) {
				lexer.take_indent();
				ref<ExprAst> elze = arena.put(parse_expr_ast(lexer, arena, ExprCtx::Statement));
				lexer.reduce_indent_by_2();
				return { lexer.range(start), cases.finish(arena), elze };
			}

			ExprAst cond = parse_expr_ast(lexer, arena, ExprCtx::Case);
			lexer.take_indent();
			ExprAst then = parse_expr_ast(lexer, arena, ExprCtx::Statement);
			lexer.take_dedent();
			cases.add({ cond, then });
		}
	}

	LetAst parse_let(Lexer& lexer, Arena& arena, const StringSlice& name) {
		// `a = b`
		lexer.take(' ');
		ref<ExprAst> init = arena.put(parse_expr_ast(lexer, arena, ExprCtx::EqualsRhs));
		lexer.take_newline_same_indent();
		ref<ExprAst> then = arena.put(parse_expr_ast(lexer, arena, ExprCtx::Statement));
		return { name, init, then };
	}

	CallAst parse_call(Lexer& lexer, Arena& arena, ExprAst arg0) {
		// `a f b, c, d`
		StringSlice fn_name = lexer.take_value_name();
		Arr<TypeAst> type_arguments = parse_type_argument_asts(lexer, arena);
		SmallArrayBuilder<ExprAst> args;
		args.add(arg0);
		if (lexer.try_take(' '))
			do {
				args.add(parse_expr_arg_ast(lexer, arena));
			} while (lexer.try_take_comma_space());
		return { fn_name, type_arguments, args.finish(arena) };
	}

	ExprAst parse_expr_ast(Lexer& lexer, Arena& arena, ExprCtx where) {
		const char* start = lexer.at();
		ExpressionToken et = lexer.take_expression_token(arena);
		if (where != ExprCtx::Case) {
			switch (et.kind) {
				case ExpressionToken::Kind::Assert: {
					lexer.take(' ');
					ref<ExprAst> asserted = arena.put(parse_expr_ast(lexer, arena, ExprCtx::EqualsRhs));
					return ExprAst { AssertAst { lexer.range(start), asserted } };
				}
				case ExpressionToken::Kind::When:
					return ExprAst { parse_when(lexer, arena, start) };
				case ExpressionToken::Kind::Pass:
					return ExprAst { lexer.range(start), ExprAst::Kind::Pass };
				case ExpressionToken::Kind::Name:
				case ExpressionToken::Kind::TypeName:
				case ExpressionToken::Kind::Literal:
				case ExpressionToken::Kind::Lparen:
				case ExpressionToken::Kind::As:
					break;
			}
		}

		// Start by parsing a simple expr
		ExprAst arg0 = parse_expr_arg_ast(lexer, arena, et);
		if (!lexer.try_take(' '))
			return arg0;
		else if (where == ExprCtx::Statement && arg0.kind() == ExprAst::Kind::Identifier && lexer.try_take('='))
			return ExprAst { parse_let(lexer, arena, arg0.identifier()) };
		else
			return ExprAst { parse_call(lexer, arena, arg0) };
	}

	struct AstAndShouldParseDot { ExprAst ast; bool may_parse_dot; };
	AstAndShouldParseDot parse_expr_arg_ast_worker(Lexer& lexer, Arena& arena, ExpressionToken et) {
		switch (et.kind) {
			case ExpressionToken::Kind::Name: {
				Arr<TypeAst> type_arguments = parse_type_argument_asts(lexer, arena);
				return type_arguments.empty() ? AstAndShouldParseDot { ExprAst { et.name }, true } : AstAndShouldParseDot { ExprAst { CallAst { et.name, type_arguments, {} } }, true };
			}

			case ExpressionToken::Kind::TypeName: {
				Arr<TypeAst> type_args = parse_type_argument_asts(lexer, arena);
				return { ExprAst { StructCreateAst { et.name, type_args, parse_prefix_args(lexer, arena) } }, false };
			}

			case ExpressionToken::Kind::Lparen: {
				auto inner = parse_expr_ast(lexer, arena, ExprCtx::Case);
				lexer.take(')');
				return { inner, true };
			}

			case ExpressionToken::Kind::Literal: {
				Arr<TypeAst> type_args = parse_type_argument_asts(lexer, arena);
				// e.g. `BigInt i = 123456789(arena)`
				Arr<ExprAst> args;
				if (lexer.try_take('(')) {
					args = parse_prefix_args(lexer, arena);
					lexer.take(')');
				}
				return { ExprAst { LiteralAst { et.literal, type_args, args } }, true };
			}

			case ExpressionToken::Kind::As: {
				lexer.take(' ');
				TypeAst type = parse_type_ast(lexer, arena);
				ExprAst arg = parse_expr_arg_ast(lexer, arena, lexer.take_expression_token(arena));
				return { ExprAst { arena.put(TypeAnnotateAst { type, arg }) }, false };
			}

			case ExpressionToken::Kind::Assert:
				throw ParseDiagnostic { lexer.diag_at_char(ParseDiag::Kind::AssertMayNotAppearInsideArg ) };
			case ExpressionToken::Kind::Pass:
				throw ParseDiagnostic { lexer.diag_at_char(ParseDiag::Kind::PassMayNotAppearInsideArg ) };
			case ExpressionToken::Kind::When:
				throw ParseDiagnostic { lexer.diag_at_char(ParseDiag::Kind::WhenMayNotAppearInsideArg ) };
		}
	}

	ExprAst parse_dots(ExprAst initial, Lexer& lexer, Arena& arena) {
		return lexer.try_take('.')
			? parse_dots(ExprAst { CallAst { lexer.take_value_name(), {}, single_element_array(arena, initial) } }, lexer, arena)
			: initial;
	}

	ExprAst parse_expr_arg_ast(Lexer& lexer, Arena& arena, ExpressionToken et) {
		AstAndShouldParseDot a = parse_expr_arg_ast_worker(lexer, arena, et);
		return a.may_parse_dot ? parse_dots(a.ast, lexer, arena) : a.ast;
	}
}

ExprAst parse_body_ast(Lexer& lexer, Arena& arena) {
	lexer.take_indent();
	const char* start = lexer.at();
	ExprAst res = parse_expr_ast(lexer, arena, ExprCtx::Statement);
	while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline) {
		ExprAst next_line = parse_expr_ast(lexer, arena, ExprCtx::Statement);
		res = ExprAst { SeqAst { lexer.range(start), arena.put(res), arena.put(next_line) } };
	}
	lexer.assert_no_indent();
	return res;
}
