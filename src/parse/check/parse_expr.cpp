#include "parse_expr.h"

namespace {
	// In Statement allow anything; in EqualsRhs anything but `=`; in Case anything single-line.
	enum class ExprCtx { Statement, EqualsRhs, Case };

	struct Arenas {
		Arena& expr_ast;
		Arena& literals;
	};

	ExprAst parse_expr_ast(Lexer& lexer, Arenas arenas, ExprCtx where);
	ExprAst parse_expr_arg_ast(Lexer& lexer, Arenas arenas, ExpressionToken et);
	ExprAst parse_expr_arg_ast(Lexer& lexer, Arenas arenas) {
		return parse_expr_arg_ast(lexer, arenas, lexer.take_expression_token(arenas.literals));
	}

	DynArray<ExprAst> parse_prefix_args(Lexer& lexer, Arenas arenas) {
		if (!lexer.try_take(' '))
			return {};
		auto args = arenas.expr_ast.small_array_builder<ExprAst>();
		while (true) {
			args.add(parse_expr_arg_ast(lexer, arenas));
			if (!lexer.try_take_comma_space())
				return args.finish();
		}
	}

	ExprAst parse_when(Lexer& lexer, Arenas arenas) {
		lexer.take_indent();
		Arena::SmallArrayBuilder<CaseAst> cases = arenas.expr_ast.small_array_builder<CaseAst>();
		while (true) {
			if (lexer.try_take_else()) {
				lexer.take_indent();
				ref<ExprAst> elze = arenas.expr_ast.emplace_copy(parse_expr_ast(lexer, arenas, ExprCtx::Statement));
				lexer.reduce_indent_by_2();
				return WhenAst { cases.finish(), elze };
			}

			ExprAst cond = parse_expr_ast(lexer, arenas, ExprCtx::Case);
			lexer.take_indent();
			ExprAst then = parse_expr_ast(lexer, arenas, ExprCtx::Statement);
			lexer.take_dedent();
			cases.add({ cond, then });
		}
	}

	ExprAst parse_expr_ast(Lexer& lexer, Arenas arenas, ExprCtx where) {
		ExpressionToken et = lexer.take_expression_token(arenas.literals);
		if (where != ExprCtx::Case && et.kind == ExpressionToken::Kind::When) {
			return parse_when(lexer, arenas);
		}

		// Start by parsing a simple expr
		ExprAst arg0 = parse_expr_arg_ast(lexer, arenas, et);
		if (!lexer.try_take(' '))
			return arg0;

		if (where == ExprCtx::Statement && arg0.kind() == ExprAst::Kind::Identifier && lexer.try_take('=')) {
			// `a = b`
			lexer.take(' ');
			ref<ExprAst> init = arenas.expr_ast.emplace_copy(parse_expr_ast(lexer, arenas, ExprCtx::EqualsRhs));
			lexer.take_newline_same_indent();
			ref<ExprAst> then = arenas.expr_ast.emplace_copy(parse_expr_ast(lexer, arenas, ExprCtx::Statement));
			return LetAst { arg0.identifier(), init, then };
		}
		else {
			// `a f b, c, d`
			StringSlice fn_name = lexer.take_value_name();
			DynArray<TypeAst> type_arguments = parse_type_argument_asts(lexer, arenas.expr_ast);
			auto args = arenas.expr_ast.small_array_builder<ExprAst>();
			args.add(arg0);
			if (lexer.try_take(' ')) {
				do {
					args.add(parse_expr_arg_ast(lexer, arenas));
				} while (lexer.try_take_comma_space());
			}
			return CallAst { fn_name, type_arguments, args.finish() };
		}
	}

	struct AstAndShouldParseDot { ExprAst ast; bool may_parse_dot; };
	AstAndShouldParseDot parse_expr_arg_ast_worker(Lexer& lexer, Arenas arenas, ExpressionToken et) {
		switch (et.kind) {
			case ExpressionToken::Kind::Name: {
				DynArray<TypeAst> type_arguments = parse_type_argument_asts(lexer, arenas.expr_ast);
				if (!type_arguments.empty()) {
					lexer.take('(');
					lexer.take(')');
					return { CallAst { et.name, type_arguments, {}}, true };
				} else if (lexer.try_take('(')) {
					lexer.take(')');
					// `f()` is a call with no arguments
					return { CallAst { et.name, {}, {}}, true };
				} else {
					return {{ et.name }, true };
				}
			}

			case ExpressionToken::Kind::TypeName: {
				if (lexer.try_take(':')) {
					lexer.take(' ');
					parse_expr_arg_ast_worker(lexer, arenas, et);
					throw "todo";
				}
				DynArray<TypeAst> type_args = parse_type_argument_asts(lexer, arenas.expr_ast);
				return { StructCreateAst { et.name, type_args, parse_prefix_args(lexer, arenas) }, false };
			}

			case ExpressionToken::Kind::Lparen: {
				auto inner = parse_expr_ast(lexer, arenas, ExprCtx::Case);
				lexer.take(')');
				return { inner, true };
			}

			case ExpressionToken::Kind::Literal: {
				DynArray<TypeAst> type_args = parse_type_argument_asts(lexer, arenas.expr_ast);
				// BigInt i = 123456789(arena)
				DynArray<ExprAst> args;
				if (lexer.try_take('(')) {
					args = parse_prefix_args(lexer, arenas);
					lexer.take(')');
				}
				return { LiteralAst { et.literal, type_args, args }, true };
			}

			case ExpressionToken::Kind::As: {
				lexer.take(' ');
				TypeAst type = parse_type_ast(lexer, arenas.expr_ast);
				ExprAst arg = parse_expr_arg_ast(lexer, arenas, lexer.take_expression_token(arenas.literals));
				return { { arenas.expr_ast.emplace<TypeAnnotateAst>()(type, arg) }, false };
			}

			case ExpressionToken::Kind::When:
				throw "todo"; // not allowed in arg context
		}
	}

	ExprAst parse_dots(ExprAst initial, Lexer& lexer, Arenas arenas) {
		return lexer.try_take('.')
			? parse_dots(CallAst { lexer.take_value_name(), {}, arenas.expr_ast.make_array(initial) }, lexer, arenas)
			: initial;
	}

	ExprAst parse_expr_arg_ast(Lexer& lexer, Arenas arenas, ExpressionToken et) {
		AstAndShouldParseDot a = parse_expr_arg_ast_worker(lexer, arenas, et);
		return a.may_parse_dot ? parse_dots(a.ast, lexer, arenas) : a.ast;
	}
}

ExprAst parse_body_ast(Lexer& lexer, Arena& expr_ast, Arena& literals) {
	ExprAst res = parse_expr_ast(lexer, { expr_ast, literals }, ExprCtx::Statement);
	while (lexer.take_newline_or_dedent() == NewlineOrDedent::Newline) {
		ExprAst next_line = parse_expr_ast(lexer, { expr_ast, literals }, ExprCtx::Statement);
		res = SeqAst { expr_ast.emplace_copy(res), expr_ast.emplace_copy(next_line) };
	}
	assert(lexer.indent() == 0);
	return res;
}
