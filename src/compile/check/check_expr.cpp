#include "./check_expr.h"

#include "../../util/ArenaArrayBuilders.h"
#include "./check_call.h"
#include "./convert_type.h"

namespace {
	Option<Ref<const Parameter>> find_parameter(const ExprContext& ctx, StringSlice name) {
		return find(ctx.current_fun->signature.parameters, [&](const Parameter& p) { return p.name == name; });
	}
	Option<Ref<const Let>> find_local(const ExprContext& ctx, StringSlice name) {
		return un_ref(find(ctx.locals, [name](const Ref<const Let>& l) { return l->name == name; }));
	}

	InstStruct struct_create_type(
		const StructDeclaration& containing __attribute__((unused)),
		ExprContext& ctx __attribute__((unused)),
		const Slice<TypeAst> type_arguments __attribute__((unused)),
		Expected& expected __attribute__((unused))) {
		throw "todo";
	}

	Type struct_field_type(const InstStruct& inst_struct __attribute__((unused)), uint i __attribute__((unused))) {
		throw "todo";
	}

	Expression check_struct_create(const StructCreateAst& create, ExprContext& ctx, Expected& expected) {
		Option<const Ref<const StructDeclaration>&> struct_op = ctx.structs_table.get(create.struct_name);
		if (!struct_op.has()) {
			ctx.check_ctx.diag(create.struct_name, Diag::Kind::StructNameNotFound);
			return Expression::bogus();
		}

		const StructDeclaration& strukt = *struct_op.get();
		if (!strukt.body.is_fields()) {
			ctx.check_ctx.diag(create.struct_name, Diag::Kind::CantCreateNonStruct);
			return Expression::bogus();
		}

		InstStruct inst_struct = struct_create_type(strukt, ctx, create.type_arguments, expected);

		uint size = strukt.body.fields().size();
		if (create.arguments.size() != size) {
			ctx.check_ctx.diag(create.struct_name, Diag::Kind::WrongNumberNewStructArguments);
			return Expression::bogus();
		}

		Slice<Expression> arguments = fill_array<Expression>()(ctx.check_ctx.arena, size, [&](uint i) {
			return check_and_expect(create.arguments[i], ctx, struct_field_type(inst_struct, i));
		});

		expected.check_no_infer(Type { inst_struct });
		return Expression { StructCreate { inst_struct, arguments } };
	}

	Expression check_type_annotate(const TypeAnnotateAst& ast, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) {
			ctx.check_ctx.diag(ast.type.name(), Diag::Kind::UnnecessaryTypeAnnotate);
			return Expression::bogus(); // Note: could continue anyway, but must check that the actual type here matches the actual type.
		}
		Type type = type_from_ast(ast.type, ctx.check_ctx, ctx.structs_table, ctx.current_fun->signature.type_parameters);
		expected.set_inferred(type);
		return check_and_expect(ast.expression, ctx, type);
	}

	Expression check_let(const LetAst& ast, ExprContext& ctx, Expected& expected) {
		StringSlice name = ast.name;
		check_param_or_local_shadows_fun(ctx.check_ctx, name, ctx.funs_table, ctx.current_fun->signature.specs);
		if (find_parameter(ctx, name).has())
			ctx.check_ctx.diag(name, Diag::Kind::LocalShadowsParameter);
		if (find_local(ctx, name).has())
			ctx.check_ctx.diag(name, Diag::Kind::LocalShadowsLocal);

		ExpressionAndType init = check_and_infer(*ast.init, ctx);
		Ref<Let> l = ctx.check_ctx.arena.put(Let { init.type, Identifier { str(ctx.check_ctx.arena, name) }, init.expression, {}, {} });
		ctx.locals.push(l);
		l->then = check(*ast.then, ctx, expected);
		assert(ctx.locals.peek() == l);
		ctx.locals.pop();
		return { l, Expression::Kind::Let };
	}

	Expression check_seq(const SeqAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingVoidType);
			return Expression::bogus();
		}
		Expression first = check_and_expect(*ast.first, ctx, ctx.builtin_types.void_type.get());
		Expression then = check(*ast.then, ctx, expected);
		return Expression { ctx.check_ctx.arena.put(Seq { first, then }) };
	}

	Expression check_when(const WhenAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.bool_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingBoolType);
			return Expression::bogus();
		}

		Slice<Case> cases = map<Case>()(ctx.check_ctx.arena, ast.cases, [&](const CaseAst& c) {
			Expression cond = check_and_expect(c.condition, ctx, ctx.builtin_types.bool_type.get());
			Expression then = check(c.then, ctx, expected);
			return Case { cond, then };
		});
		Ref<Expression> elze = ctx.check_ctx.arena.put(check(*ast.elze, ctx, expected));
		return Expression { When { cases, elze } };
	}

	Expression check_assert(const AssertAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingVoidType);
			return Expression::bogus();
		}
		expected.check_no_infer(ctx.builtin_types.void_type.get());

		if (!ctx.builtin_types.bool_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingBoolType);
			return Expression::bogus();
		}
		Ref<Expression> asserted = ctx.check_ctx.arena.put(check_and_expect(ast.asserted, ctx, ctx.builtin_types.bool_type.get()));
		return Expression(asserted, Expression::Kind::Assert);
	}

	Expression check_pass(const SourceRange& range, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(range, Diag::Kind::MissingVoidType);
			return Expression::bogus();
		}
		expected.check_no_infer(ctx.builtin_types.void_type.get());
		return Expression(Expression::Kind::Pass);
	}

	const StringSlice LITERAL { "literal" };

	Expression check_no_call_literal_inner(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) expected.as_if_checked(); else expected.set_inferred(ctx.builtin_types.string_type.get());
		return Expression { str(ctx.check_ctx.arena, literal) };
	}

	Expression check_no_call_literal(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type.has()) {
			ctx.check_ctx.diag(literal, Diag::Kind::MissingStringType);
			return Expression::bogus();
		}
		const Type& string_type = ctx.builtin_types.string_type.get();
		expected.check_no_infer(string_type);
		return check_no_call_literal_inner(literal, ctx, expected);
	}

	Expression check_literal(const LiteralAst& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type.has()) {
			ctx.check_ctx.diag(literal.literal, Diag::Kind::MissingStringType);
			return Expression::bogus();
		}
		const Type& string_type = ctx.builtin_types.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (literal.type_arguments.size() == 0 && literal.arguments.size() == 0 && (!current_expectation.has() || current_expectation.get() == string_type)) {
			return check_no_call_literal_inner(literal.literal, ctx, expected);
		} else {
			SmallArrayBuilder<ExprAst> b;
			b.add(ExprAst { str(ctx.check_ctx.arena, literal.literal) }); // This is a NoCallLiteral
			for (const ExprAst &arg : literal.arguments)
				b.add(arg);
			return check_call(LITERAL, b.finish(ctx.scratch_arena), literal.type_arguments, ctx, expected);
		}
	}

	Expression check_identifier(const StringSlice& name, ExprContext& ctx, Expected& expected) {
		Option<Ref<const Parameter>> param_op = find_parameter(ctx, name);
		if (param_op.has()) {
			const Parameter& param = param_op.get();
			expected.check_no_infer(param.type);
			return Expression(&param);
		}

		Option<Ref<const Let>> let_op = find_local(ctx, name);
		if (let_op.has()) {
			Ref<const Let> let = let_op.get();
			expected.check_no_infer(let->type);
			return Expression(let, Expression::Kind::LocalReference);
		}

		return check_call(name, {}, {}, ctx, expected);
	}
}

Expression check(const ExprAst& ast, ExprContext& ctx, Expected& expected) {
	switch (ast.kind()) {
		case ExprAst::Kind::Identifier:
			return check_identifier(ast.identifier(), ctx, expected);
		case ExprAst::Kind::Literal:
			return check_literal(ast.literal(), ctx, expected);
		case ExprAst::Kind::NoCallLiteral:
			return check_no_call_literal(ast.no_call_literal(), ctx, expected);
		case ExprAst::Kind::Call: {
			CallAst c = ast.call();
			return check_call(c.fun_name, c.arguments, c.type_arguments, ctx, expected);
		}
		case ExprAst::Kind::StructCreate:
			return check_struct_create(ast.struct_create(), ctx, expected);
		case ExprAst::Kind::TypeAnnotate:
			return check_type_annotate(ast.type_annotate(), ctx, expected);
		case ExprAst::Kind::Let:
			return check_let(ast.let(), ctx, expected);
		case ExprAst::Kind::Seq:
			return check_seq(ast.seq(), ctx, expected);
		case ExprAst::Kind::When:
			return check_when(ast.when(), ctx, expected);
		case ExprAst::Kind::Assert:
			return check_assert(ast.assert_ast(), ctx, expected);
		case ExprAst::Kind::Pass:
			return check_pass(ast.pass(), ctx, expected);
	}
}
