#include "./check_expr.h"

#include "../../util/store/ArenaArrayBuilders.h"
#include "../../util/store/slice_util.h"
#include "../model/types_equal_ignore_lifetime.h"
#include "./check_call.h"
#include "./check_param_or_local_shadows_fun.h"
#include "./convert_type.h"

namespace {
	Option<Ref<const Parameter>> find_parameter(const ExprContext& ctx, StringSlice name) {
		return find(ctx.current_fun->signature.parameters, [&](const Parameter& p) { return p.name == name; });
	}
	Option<Ref<const Let>> find_local(const ExprContext& ctx, StringSlice name) {
		Option<Ref<const Ref<const Let>>> o = find(ctx.locals, [name](const Ref<const Let>& l) { return l->name == name; });
		return o.has() ? Option<Ref<const Let>> { o.get() } : Option<Ref<const Let>> {};
	}

	InstStruct struct_create_type(Ref<const StructDeclaration> strukt, ExprContext& ctx, const Slice<TypeAst> type_arguments, Expected& expected) {
		if (strukt->type_parameters.is_empty()) {
			if (!type_arguments.is_empty()) todo();
			return InstStruct { strukt, {} };
		}

		if (type_arguments.is_empty()) {
			const Option<StoredType>& ex = expected.get_current_expectation();
			if (ex.has()) {
				const StoredType& s = ex.get();
				if (s.is_type_parameter()) todo();
				const InstStruct& is = s.inst_struct();
				if (is.strukt != strukt) todo();
				return is;
			} else {
				todo(); //TODO: support inferring type arguments from argument types too, like with call.
			}
		} else {
			const FunSignature& cur_sig = ctx.current_fun->signature;
			return InstStruct { strukt, type_arguments_from_asts(type_arguments, ctx.check_ctx, ctx.structs_table, Option<const Slice<Parameter>&> { cur_sig.parameters }, cur_sig.type_parameters) };
		}
	}

	Type struct_field_type(const InstStruct& inst_struct, uint i) {
		const StructDeclaration& strukt = inst_struct.strukt;
		const StructField& field = strukt.body.fields()[i];
		const Type& declared_type = field.type;
		if (declared_type.lifetime().is_borrow() && declared_type.stored_type().is_type_parameter()) todo();

		if (declared_type.stored_type().is_type_parameter()) {
			if (declared_type.lifetime().is_borrow()) todo(); // If the field is `?T *a`, need `Int *a` here, not just `Int`
			assert(contains_ref(strukt.type_parameters, declared_type.stored_type().param()));
			return inst_struct.type_arguments[declared_type.stored_type().param()->index];
		} else {
			return declared_type;
		}
	}

	ExpressionAndLifetime check_struct_create(const StructCreateAst& create, ExprContext& ctx, Expected& expected) {
		Option<const Ref<const StructDeclaration>&> struct_op = ctx.structs_table.get(create.struct_name);
		if (!struct_op.has()) {
			ctx.check_ctx.diag(create.struct_name, Diag::Kind::StructNameNotFound);
			return expected.bogus();
		}

		Ref<const StructDeclaration> strukt = struct_op.get();
		if (!strukt->body.is_fields()) {
			ctx.check_ctx.diag(create.struct_name, Diag::Kind::CantCreateNonStruct);
			return expected.bogus();
		}

		InstStruct inst_struct = struct_create_type(strukt, ctx, create.type_arguments, expected);

		uint size = strukt->body.fields().size();
		if (create.arguments.size() != size) {
			ctx.check_ctx.diag(create.struct_name, Diag { Diag::Kind::WrongNumberNewStructArguments, WrongNumber { size, create.arguments.size() } });
			return expected.bogus();
		}

		Slice<Expression> arguments = fill_array<Expression>()(ctx.check_ctx.arena, size, [&](uint i) {
			return check_and_expect_stored_type_and_lifetime(create.arguments[i], ctx, struct_field_type(inst_struct, i));
		});

		expected.check_no_infer(StoredType { inst_struct });
		return { Expression { StructCreate { inst_struct, arguments } }, Lifetime::noborrow() };
	}

	ExpressionAndLifetime check_let(const LetAst& ast, ExprContext& ctx, Expected& expected) {
		StringSlice name = ast.name;
		check_param_or_local_shadows_fun(ctx.check_ctx, name, ctx.funs_table, ctx.current_fun->signature.specs);
		if (find_parameter(ctx, name).has())
			ctx.check_ctx.diag(name, Diag::Kind::LocalShadowsParameter);
		if (find_local(ctx, name).has())
			ctx.check_ctx.diag(name, Diag::Kind::LocalShadowsLocal);

		ExpressionAndType init = check_and_infer(*ast.init, ctx);
		// We'll infer the lifetime in the lifetime checker, not here.
		Ref<Let> let = ctx.check_ctx.arena.put(Let { init.type, Identifier { copy_string(ctx.check_ctx.arena, name) }, init.expression, {}, {} });
		ctx.locals.push(let);
		ExpressionAndLifetime then = check_expr(*ast.then, ctx, expected);
		let->then = then.expression;
		assert(ctx.locals.peek() == let);
		ctx.locals.pop();
		return { Expression { let, Expression::Kind::Let }, then.lifetime };
	}

	inline Expression check_and_expect_builtin_type(const ExprAst& ast, ExprContext& ctx, const Type& expected_type) {
		assert(expected_type.stored_type().inst_struct().strukt->copy);
		return check_and_expect_stored_type(ast, ctx, expected_type.stored_type()).expression;
	}

	ExpressionAndLifetime check_seq(const SeqAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingVoidType);
			return expected.bogus();
		}
		// Don't check lifetime because Void should be copy.
		Expression first = check_and_expect_builtin_type(*ast.first, ctx, ctx.builtin_types.void_type.get());
		ExpressionAndLifetime then = check_expr(*ast.then, ctx, expected);
		return { Expression { ctx.check_ctx.arena.put(Seq { first, then.expression }) }, then.lifetime };
	}

	ExpressionAndLifetime check_when(const WhenAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.bool_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingBoolType);
			return expected.bogus();
		}

		Pair<Slice<Case>, Slice<Lifetime>> cases = map_to_two_arrays<Case, Lifetime>()(ctx.check_ctx.arena, ctx.scratch_arena, ast.cases, [&](const CaseAst& c) {
			Expression cond = check_and_expect_builtin_type(c.condition, ctx, ctx.builtin_types.bool_type.get());
			ExpressionAndLifetime then = check_expr(c.then, ctx, expected);
			return Pair<Case, Lifetime> { Case { cond, then.expression }, then.lifetime };
		});
		ExpressionAndLifetime elze = check_expr(*ast.elze, ctx, expected);

		Lifetime lifetime = common_lifetime(cases.second, elze.lifetime);
		Type type = Type { expected.inferred_stored_type(), lifetime };
		return { Expression { When { cases.first, ctx.check_ctx.arena.put(elze.expression), type } }, lifetime };
	}

	ExpressionAndLifetime check_assert(const AssertAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingVoidType);
			return expected.bogus();
		}
		expected.check_no_infer(ctx.builtin_types.void_type.get().stored_type());

		if (!ctx.builtin_types.bool_type.has()) {
			ctx.check_ctx.diag(ast.range, Diag::Kind::MissingBoolType);
			return expected.bogus();
		}
		Ref<Expression> asserted = ctx.check_ctx.arena.put(check_and_expect_builtin_type(ast.asserted, ctx, ctx.builtin_types.bool_type.get()));
		return { Expression { asserted, Expression::Kind::Assert }, Lifetime::noborrow() };
	}

	ExpressionAndLifetime check_pass(const SourceRange& range, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.void_type.has()) {
			ctx.check_ctx.diag(range, Diag::Kind::MissingVoidType);
			return expected.bogus();
		}
		expected.check_no_infer(ctx.builtin_types.void_type.get().stored_type());
		return { Expression { Expression::Kind::Pass }, Lifetime::noborrow() };
	}

	const StringSlice LITERAL { "literal" };

	ExpressionAndLifetime check_no_call_literal_inner(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type())
			expected.as_if_checked();
		else
			expected.set_inferred(ctx.builtin_types.string_type.get().stored_type());
		return { Expression { copy_string(ctx.check_ctx.arena, literal) }, Lifetime::noborrow() };
	}

	ExpressionAndLifetime check_no_call_literal(const StringSlice& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type.has()) {
			ctx.check_ctx.diag(literal, Diag::Kind::MissingStringType);
			return expected.bogus();
		}
		expected.check_no_infer(ctx.builtin_types.string_type.get().stored_type());
		return check_no_call_literal_inner(literal, ctx, expected);
	}

	ExpressionAndLifetime check_literal(const LiteralAst& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.builtin_types.string_type.has()) {
			ctx.check_ctx.diag(literal.literal, Diag::Kind::MissingStringType);
			return expected.bogus();
		}
		const Option<StoredType>& current_expectation = expected.get_current_expectation();
		if (literal.type_arguments.size() == 0
			&& literal.arguments.size() == 0
			&& (!current_expectation.has() || types_equal_ignore_lifetime(current_expectation.get(), ctx.builtin_types.string_type.get().stored_type()))) {
			return check_no_call_literal_inner(literal.literal, ctx, expected);
		} else {
			SmallArrayBuilder<ExprAst> b;
			b.add(ExprAst { copy_string(ctx.check_ctx.arena, literal.literal) }); // This is a NoCallLiteral
			for (const ExprAst &arg : literal.arguments)
				b.add(arg);
			return check_call(LITERAL, b.finish(ctx.scratch_arena), literal.type_arguments, ctx, expected);
		}
	}

	ExpressionAndLifetime check_identifier(const StringSlice& name, ExprContext& ctx, Expected& expected) {
		Option<Ref<const Parameter>> param_op = find_parameter(ctx, name);
		if (param_op.has()) {
			Ref<const Parameter> param = param_op.get();
			expected.check_no_infer(param->type.stored_type_or_bogus());
			return { Expression{param}, Lifetime::of_parameter(param->index) };
		}

		Option<Ref<const Let>> let_op = find_local(ctx, name);
		if (let_op.has()) {
			Ref<const Let> let = let_op.get();
			expected.check_no_infer(let->type.stored_type_or_bogus());
			const Lifetime& let_life = let->type.lifetime();
			// If it is already a pointer, leave it that way (don't point to a pointer).
			// If it is stored in a local variable, use a pointer to that.
			Lifetime lifetime = let_life.is_borrow() ? let_life : Lifetime::local_borrow();
			return { Expression(let, Expression::Kind::LocalReference), lifetime };
		}

		return check_call(name, {}, {}, ctx, expected);
	}
}

ExpressionAndLifetime check_expr(const ExprAst& ast, ExprContext& ctx, Expected& expected) {
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
