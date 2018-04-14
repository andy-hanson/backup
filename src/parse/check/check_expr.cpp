#include "./check_expr.h"

#include "../../util/collection_util.h" // find, filter_unordered

#include "./check_call.h"
#include "./convert_type.h"

namespace {
	InstStruct struct_create_type(
		const StructDeclaration& containing __attribute__((unused)),
		ExprContext& ctx __attribute__((unused)),
		const DynArray<TypeAst> type_arguments __attribute__((unused)),
		Expected& expected __attribute__((unused))) {
		throw "todo";
	}

	Type struct_field_type(const InstStruct& inst_struct __attribute__((unused)), uint i __attribute__((unused))) {
		throw "todo";
	}

	StructCreate check_struct_create(const StructCreateAst& create, ExprContext& ctx, Expected& expected) {
		Option<const ref<const StructDeclaration>&> struct_op = ctx.structs_table.get(create.struct_name);
		if (!struct_op) throw "todo";
		const StructDeclaration& strukt = *struct_op.get();
		if (!strukt.body.is_fields()) throw "todo";

		InstStruct inst_struct = struct_create_type(strukt, ctx, create.type_arguments, expected);

		size_t size = strukt.body.fields().size();
		if (create.arguments.size() != size) throw "todo";

		DynArray<Expression> arguments = ctx.arena.fill_array<Expression>(size)([&](uint i) {
			return check_and_expect(create.arguments[i], ctx, struct_field_type(inst_struct, i));
		});

		expected.check_no_infer({Effect::Io, inst_struct});
		return { inst_struct, arguments };
	}

	Expression check_type_annotate(const TypeAnnotateAst& ast, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) {
			throw "todo"; // we already have a type, you shouldn't provide one
		}
		Type type = convert_type(ast.type, ctx);
		expected.set_inferred(type);
		return check_and_expect(ast.expression, ctx, type);
	}

	Expression check_let(const LetAst& ast, ExprContext& ctx, Expected& expected) {
		StringSlice name = ast.name;
		if (find(ctx.parameters, [name](const Parameter& p) { return p.name == name; }))
			throw "todo";
		ExpressionAndType init = check_and_infer(*ast.init, ctx);
		ref<Let> l = ctx.arena.emplace<Let>()(init.type, Identifier { ctx.arena.str(name) }, init.expression, Expression {});
		ctx.locals.push(l);
		l->then = check(*ast.then, ctx, expected);
		assert(ctx.locals.peek() == l);
		ctx.locals.pop();
		return { l, Expression::Kind::Let };
	}

	Expression check_seq(const SeqAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.void_type) throw "todo";
		Expression first = check_and_expect(*ast.first, ctx, ctx.void_type.get());
		Expression then = check(*ast.then, ctx, expected);
		return {  ctx.arena.emplace<Seq>()(first, then) };
	}

	Expression check_when(const WhenAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.bool_type) throw "todo: must declare Bool somewhere in order to use 'when'";
		DynArray<Case> cases = ctx.arena.map_array<CaseAst, Case>(ast.cases)([&](const CaseAst& c) {
			Expression cond = check_and_expect(c.condition, ctx, ctx.bool_type.get());
			Expression then = check(c.then, ctx, expected);
			return Case { cond, then };
		});
		ref<Expression> elze = ctx.arena.emplace_copy<Expression>(check(*ast.elze, ctx, expected));
		return When { cases, elze };
	}

	const StringSlice LITERAL { "literal" };

	Expression check_no_call_literal_inner(const ArenaString& literal, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) expected.as_if_checked(); else expected.set_inferred(ctx.string_type.get());
		return Expression(literal);
	}

	Expression check_no_call_literal(const ArenaString& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.string_type) throw "todo: string type missing";
		const Type& string_type = ctx.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (current_expectation && !types_exactly_equal(current_expectation.get(), string_type)) throw "todo";
		return check_no_call_literal_inner(literal, ctx, expected);
	}

	Expression check_literal(const LiteralAst& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.string_type) throw "todo: string type missing";

		const Type& string_type = ctx.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (literal.type_arguments.size() == 0 && literal.arguments.size() == 0 && (!current_expectation || types_exactly_equal(current_expectation.get(), string_type))) {
			return check_no_call_literal_inner(literal.literal, ctx, expected);
		} else {
			//TODO:PERF
			Arena::SmallArrayBuilder<ExprAst> b = ctx.scratch_arena.small_array_builder<ExprAst>();
			b.add(literal.literal); // This is a NoCallLiteral
			for (const ExprAst &arg : literal.arguments)
				b.add(arg);
			return check_call(LITERAL, b.finish(), literal.type_arguments, ctx, expected);
		}
	}
}

Expression check(const ExprAst& ast, ExprContext& ctx, Expected& expected) {
	switch (ast.kind()) {
		case ExprAst::Kind::Identifier: {
			StringSlice name = ast.identifier();
			Option<const Parameter&> p_op = find(ctx.parameters, [name](const Parameter& p) { return p.name == name; });
			if (p_op) {
				const Parameter& p = p_op.get();
				expected.check_no_infer(p.type);
				return Expression(&p);
			}
			Option<const ref<const Let>&> l_op = find(ctx.locals, [name](const ref<const Let>& l) { return l->name == name; });
			if (l_op) {
				ref<const Let> l = l_op.get();
				expected.check_no_infer(l->type);
				return Expression(l, Expression::Kind::LocalReference);
			}
			throw "todo: unrecognized identifier";
		}
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
