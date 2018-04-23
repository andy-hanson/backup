#include "./check_expr.h"

#include "./check_call.h"
#include "./convert_type.h"

namespace {
	Option<ref<const Parameter>> find_parameter(const ExprContext& ctx, StringSlice name) {
		return find(ctx.current_fun->signature.parameters, [&](const Parameter& p) { return p.name == name; });
	}
	Option<ref<const Let>> find_local(const ExprContext& ctx, StringSlice name) {
		return un_ref(find(ctx.locals, [name](const ref<const Let>& l) { return l->name == name; }));
	}

	InstStruct struct_create_type(
		const StructDeclaration& containing __attribute__((unused)),
		ExprContext& ctx __attribute__((unused)),
		const Arr<TypeAst> type_arguments __attribute__((unused)),
		Expected& expected __attribute__((unused))) {
		throw "todo";
	}

	Type struct_field_type(const InstStruct& inst_struct __attribute__((unused)), uint i __attribute__((unused))) {
		throw "todo";
	}

	Expression check_struct_create(const StructCreateAst& create, ExprContext& ctx, Expected& expected) {
		Option<const ref<const StructDeclaration>&> struct_op = ctx.structs_table.get(create.struct_name);
		if (!struct_op) {
			ctx.al.diag(create.struct_name, Diag::Kind::StructNameNotFound);
			return Expression::bogus();
		}

		const StructDeclaration& strukt = *struct_op.get();
		if (!strukt.body.is_fields()) {
			ctx.al.diag(create.struct_name, Diag::Kind::CantCreateNonStruct);
			return Expression::bogus();
		}

		InstStruct inst_struct = struct_create_type(strukt, ctx, create.type_arguments, expected);

		size_t size = strukt.body.fields().size();
		if (create.arguments.size() != size) {
			ctx.al.diag(create.struct_name, Diag::Kind::WrongNumberNewStructArguments);
			return Expression::bogus();
		}

		Arr<Expression> arguments = ctx.al.arena.fill_array<Expression>()(size, [&](uint i) {
			return check_and_expect(create.arguments[i], ctx, struct_field_type(inst_struct, i));
		});

		expected.check_no_infer(Type { inst_struct });
		return StructCreate { inst_struct, arguments };
	}

	Expression check_type_annotate(const TypeAnnotateAst& ast, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) {
			ctx.al.diag(ast.type.type_name, Diag::Kind::UnnecessaryTypeAnnotate);
			return Expression::bogus(); // Note: could continue anyway, but must check that the actual type here matches the actual type.
		}
		Type type = type_from_ast(ast.type, ctx.al, ctx.structs_table, ctx.current_fun->signature.type_parameters);
		expected.set_inferred(type);
		return check_and_expect(ast.expression, ctx, type);
	}

	Expression check_let(const LetAst& ast, ExprContext& ctx, Expected& expected) {
		StringSlice name = ast.name;
		check_param_or_local_shadows_fun(ctx.al, name, ctx.funs_table, ctx.current_fun->signature.specs);
		if (find_parameter(ctx, name))
			ctx.al.diag(name, Diag::Kind::LocalShadowsParameter);
		if (find_local(ctx, name))
			ctx.al.diag(name, Diag::Kind::LocalShadowsLocal);

		ExpressionAndType init = check_and_infer(*ast.init, ctx);
		ref<Let> l = ctx.al.arena.put(Let { init.type, Identifier { ctx.al.arena.str(name) }, init.expression, {}});
		ctx.locals.push(l);
		l->then = check(*ast.then, ctx, expected);
		assert(ctx.locals.peek() == l);
		ctx.locals.pop();
		return { l, Expression::Kind::Let };
	}

	Expression check_seq(const SeqAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type) {
			ctx.al.diag(ast.range, Diag::Kind::MissingVoidType);
			return Expression::bogus();
		}
		Expression first = check_and_expect(*ast.first, ctx, ctx.builtin_types.void_type.get());
		Expression then = check(*ast.then, ctx, expected);
		return { ctx.al.arena.put(Seq { first, then }) };
	}

	Expression check_when(const WhenAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.bool_type) {
			ctx.al.diag(ast.range, Diag::Kind::MissingBoolType);
			return Expression::bogus();
		}

		Arr<Case> cases = ctx.al.arena.map<Case>()(ast.cases, [&](const CaseAst& c) {
			Expression cond = check_and_expect(c.condition, ctx, ctx.builtin_types.bool_type.get());
			Expression then = check(c.then, ctx, expected);
			return Case { cond, then };
		});
		ref<Expression> elze = ctx.al.arena.put(check(*ast.elze, ctx, expected));
		return When { cases, elze };
	}

	const StringSlice LITERAL { "literal" };

	Expression check_no_call_literal_inner(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) expected.as_if_checked(); else expected.set_inferred(ctx.builtin_types.string_type.get());
		return Expression{ctx.al.arena.str(literal)};
	}

	Expression check_no_call_literal(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type) {
			ctx.al.diag(literal, Diag::Kind::MissingStringType);
			return Expression::bogus();
		}
		const Type& string_type = ctx.builtin_types.string_type.get();
		expected.check_no_infer(string_type);
		return check_no_call_literal_inner(literal, ctx, expected);
	}

	Expression check_literal(const LiteralAst& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type) {
			ctx.al.diag(literal.literal, Diag::Kind::MissingStringType);
			return Expression::bogus();
		}
		const Type& string_type = ctx.builtin_types.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (literal.type_arguments.size() == 0 && literal.arguments.size() == 0 && (!current_expectation || current_expectation.get() == string_type)) {
			return check_no_call_literal_inner(literal.literal, ctx, expected);
		} else {
			Arena::SmallArrayBuilder<ExprAst> b = ctx.scratch_arena.small_array_builder<ExprAst>();
			b.add(ctx.al.arena.str(literal.literal)); // This is a NoCallLiteral
			for (const ExprAst &arg : literal.arguments)
				b.add(arg);
			return check_call(LITERAL, b.finish(), literal.type_arguments, ctx, expected);
		}
	}

	Expression check_identifier(const StringSlice& name, ExprContext& ctx, Expected& expected) {
		Option<ref<const Parameter>> param_op = find_parameter(ctx, name);
		if (param_op) {
			const Parameter& param = param_op.get();
			expected.check_no_infer(param.type);
			return Expression(&param);
		}

		Option<ref<const Let>> let_op = find_local(ctx, name);
		if (let_op) {
			ref<const Let> let = let_op.get();
			expected.check_no_infer(let->type);
			return Expression(let, Expression::Kind::LocalReference);
		}

		ctx.al.diag(name, Diag::UnrecognizedParameterOrLocal);
		return Expression::bogus();
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
	}
}
