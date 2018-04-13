#include "check_function_body.h"

#include "type_utils.h"
#include "../util/collection_util.h" // find, filter_unordered

namespace {
	struct ExprContext {
		Arena& arena;
		Arena& scratch_arena; // cleared after every convert call.
		const FunsTable& funs_table;
		const StructsTable& structs_table;
		const DynArray<Parameter>& parameters;
		// This is pushed and popped as we add locals and go out of scope.
		MaxSizeVector<8, ref<const Let>> locals;

		const Option<Type>& bool_type;
		const Option<Type>& string_type;

		ExprContext(const ExprContext& other) = delete;
		void operator=(const ExprContext& other) = delete;
	};

	class Expected {
		const bool _had_expectation;
		bool _was_checked = false; // For asserting
		Option<Type> type;

	public:
		Expected() : _had_expectation(false) {}
		__attribute__((unused)) // https://youtrack.jetbrains.com/issue/CPP-12376
		Expected(Type t) : _had_expectation(true), type(t) {}

		~Expected() {
			assert(_was_checked);
		}

		// Either an initial expectation, or one inferred from something previous.
		const Option<Type>& get_current_expectation() const {
			return type;
		}

		//TODO: when to use this vs has_inferred_type?
		bool had_expectation() const { return _had_expectation; }

		bool has_expectation_or_inferred_type() const {
			return type;
		}

		const Type& inferred_type() const {
			return type.get();
		}

		// Called when an operation behaves like checking this, even without calling 'check'.
		// E.g., removing overloads that don't return the expected type.
		void as_if_checked() {
			_was_checked = true;
		}

		void check_no_infer(Type actual) {
			_was_checked = true;
			if (type) {
				Type t = type.get();
				if (!does_type_match_no_infer(t, actual)) {
					throw "todo";
				}
			}
			else {
				assert(!_had_expectation);
				set_inferred(actual);
			}
		}

		void set_inferred(Type actual) {
			assert(!_had_expectation && !type);
			_was_checked = true;
			type = actual;
		}
	};

	Expression convert(const ExprAst& ast, ExprContext& ctx, Expected& expected);
	Expression convert_and_expect(const ExprAst& ast, ExprContext& ctx, Type expected_type) {
		Expected expected { expected_type };
		return convert(ast, ctx, expected);
	}
	struct ExpressionAndType { Expression expression; Type type; };
	ExpressionAndType convert_and_infer(const ExprAst& ast, ExprContext& ctx) {
		Expected infer;
		Expression res = convert(ast, ctx, infer);
		return { res, infer.inferred_type() };
	}

	using Candidates = MaxSizeVector<16, Candidate>;

	/** Returns an expected argument type if all candidates agree on it. */
	Expected get_common_overload_parameter_type(Candidates& candidates, uint arg_index) {
		Option<Type> expected;
		for (const Candidate& candidate : candidates) {
			// If we get a generic candidate and haven't inferred this parameter type yet, no expected type.
			ref<const Type> parameter_type = &candidate.fun->parameters[arg_index].type;
			if (parameter_type->is_parameter()) {
				Option<const Type&> inferred = candidate.inferring_type_arguments[parameter_type->param()->index].as_ref();
				if (!inferred) return {}; // If there's at least one uninferred generic parameter here, can't have an expected type.
				parameter_type = &inferred.get();
			}
			if (!expected)
				expected = parameter_type;
			else if (!types_exactly_equal(parameter_type, expected.get()))
				return {};
		}
		// .get() will succeed because we had at least one candidate coming in.
		return { expected.get() };
	}

	/** Remove functions that did not have that argument type. We will also have to fill in type parameters for some candidates here. */
	void remove_overloads_given_argument_type(Candidates& candidates, const Expected& expected, uint arg_index) {
		if (expected.had_expectation()) return; // Arg checked itself.

		const Type& arg_type = expected.inferred_type();
		filter_unordered(candidates, [arg_index, arg_type](Candidate& candidate) {
			return try_match_types(candidate.fun->parameters[arg_index].type, arg_type, candidate);
		});
		if (candidates.empty()) throw "todo";
	}

	Type convert_type(const TypeAst& ast, ExprContext& ctx) {
		Effect e __attribute__((unused)) = ast.effect;
		Option<const ref<const StructDeclaration>&> s __attribute__((unused)) = ctx.structs_table.get(ast.type_name);
		DynArray<TypeAst> ta __attribute__((unused)) = ast.type_arguments;
		throw "todo";
	}

	DynArray<Type> convert_type_arguments(const DynArray<TypeAst>& type_arguments, ExprContext& ctx) {
		return ctx.arena.map_array<TypeAst, Type>(type_arguments)([&ctx](const TypeAst& t) { return convert_type(t, ctx); });
	}

	// Special handling for unary calls because:
	// a) May be a struct access
	// b) We can be more efficient for overload resolution.
	Option<Expression> try_convert_struct_field_access(StringSlice fun_name, const ExprAst &argument, ExprContext &ctx, Expected &expected) {
		auto argAndType = convert_and_infer(argument, ctx);
		Type arg_type = argAndType.type;
		if (arg_type.is_plain()) {
			const PlainType& plain = arg_type.plain();
			const StructDeclaration& strukt = plain.inst_struct.strukt;
			if (strukt.body.kind() == StructBody::Kind::Fields) {
				if (!plain.inst_struct.type_arguments.empty()) throw "todo";
				//TODO: substitute type arguments here!
				Option<const StructField&> field = find(strukt.body.fields(), [fun_name](const StructField& f) { return f.name == fun_name; });
				if (field) {
					// TODO: also check plain.effect to narrow the field type
					expected.check_no_infer(field.get().type);
					return { StructFieldAccess { ctx.arena.emplace_copy(argAndType.expression), &field.get() } };
				}
			}
		}
		return {};
	}

	Expression convert_call(const StringSlice& fun_name, const DynArray<ExprAst>& argument_asts, const DynArray<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected) {
		DynArray<Type> explicit_type_arguments = convert_type_arguments(type_argument_asts, ctx);
		size_t arity = argument_asts.size();
		assert(arity != 0);
		if (arity == 1 && explicit_type_arguments.empty()) {
			Option<Expression> e = try_convert_struct_field_access(fun_name, argument_asts[0], ctx, expected);
			if (e) return e.get();
		}

		Option<const OverloadGroup&> overloads = ctx.funs_table.get(fun_name);
		if (!overloads) throw "todo: did not get any overloads";

		Candidates candidates;
		for (ref<const Fun> f : overloads.get().funs) {
			if (f->arity() != arity) continue;
			DynArray<Option<Type>> type_arguments;
			if (!explicit_type_arguments.empty()) {
				if (f->type_parameters.size() != explicit_type_arguments.size()) continue;
				type_arguments = ctx.scratch_arena.map_array<Type, Option<Type>>(explicit_type_arguments)([](const Type& t) -> Option<Type> { return { t }; });
			} else {
				type_arguments = ctx.scratch_arena.fill_array<Option<Type>>(f->type_parameters.size())([](uint _ __attribute__((unused))) -> Option<Type> { return {}; });
			}
			candidates.emplace(f, type_arguments);
		}

		if (candidates.empty()) throw "todo: no overload has that arity";

		// Can't just check each overload in order because we want each argument to have an expected type.
		bool already_checked_return_type = false;
		if (const Option<Type>& expected_return_type = expected.get_current_expectation()) {
			already_checked_return_type = true;
			expected.as_if_checked();
			filter_unordered(candidates, [&expected_return_type](Candidate& candidate) {
				return try_match_types(candidate.fun->return_type, expected_return_type.get(), candidate);
			});
			if (candidates.empty()) throw "todo: no overload returns what you wanted";
		}

		DynArray<Expression> arguments = ctx.arena.fill_array<Expression>(arity)([&](uint arg_idx) {
			Expected expected_this_arg = get_common_overload_parameter_type(candidates, arg_idx);
			Expression res = convert(argument_asts[arg_idx], ctx, expected_this_arg);
			remove_overloads_given_argument_type(candidates, expected_this_arg, arg_idx);
			return res;
		});

		if (candidates.size() > 1) throw "todo: two identical candidates?";

		Candidate candidate = candidates[0];

		DynArray<Type> candidate_type_arguments = ctx.arena.map_array<Option<Type>, Type>(candidate.inferring_type_arguments)([](const Option<Type>& t){
			if (!t) throw "todo: didn't infer all type arguments";
			return t.get();
		});

		if (!already_checked_return_type) {
			expected.set_inferred(get_candidate_return_type(candidate));
		}

		return Call { { candidate.fun, candidate_type_arguments }, arguments };
	}

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

	StructCreate convert_struct_create(const StructCreateAst& create, ExprContext& ctx, Expected& expected) {
		Option<const ref<const StructDeclaration>&> struct_op = ctx.structs_table.get(create.struct_name);
		if (!struct_op) throw "todo";
		const StructDeclaration& strukt = *struct_op.get();
		if (strukt.body.kind() != StructBody::Kind::Fields) throw "todo";

		InstStruct inst_struct = struct_create_type(strukt, ctx, create.type_arguments, expected);

		size_t size = strukt.body.fields().size();
		if (create.arguments.size() != size) throw "todo";

		DynArray<Expression> arguments = ctx.arena.fill_array<Expression>(size)([&](uint i) {
			return convert_and_expect(create.arguments[i], ctx, struct_field_type(inst_struct, i));
		});

		expected.check_no_infer({Effect::Io, inst_struct});
		return { inst_struct, arguments };
	}

	Expression convert_type_annotate(const TypeAnnotateAst& ast, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) {
			throw "todo"; // we already have a type, you shouldn't provide one
		}
		Type type = convert_type(ast.type, ctx);
		expected.set_inferred(type);
		return convert_and_expect(ast.expression, ctx, type);
	}

	Expression convert_let(const LetAst& ast, ExprContext& ctx, Expected& expected) {
		StringSlice name = ast.name;
		if (find(ctx.parameters, [name](const Parameter& p) { return p.name == name; }))
			throw "todo";
		auto init = convert_and_infer(*ast.init, ctx);
		ref<Let> l = ctx.arena.emplace<Let>()(init.type, Identifier { ctx.arena.str(name) }, init.expression, Expression {});
		ctx.locals.push(l);
		l->then = convert(*ast.then, ctx, expected);
		assert(ctx.locals.peek() == l);
		ctx.locals.pop();
		return { l, Expression::Kind::Let };
	}

	Expression convert_when(const WhenAst& ast, ExprContext& ctx, Expected& expected) {
		if (!ctx.bool_type) throw "todo: must declare Bool somewhere in order to use 'when'";
		DynArray<Case> cases = ctx.arena.map_array<CaseAst, Case>(ast.cases)([&](const CaseAst& c) {
			Expression cond = convert_and_expect(c.condition, ctx, ctx.bool_type.get());
			Expression then = convert(c.then, ctx, expected);
			return Case { cond, then };
		});
		ref<Expression> elze = ctx.arena.emplace_copy<Expression>(convert(*ast.elze, ctx, expected));
		return When { cases, elze };
	}

	const StringSlice LITERAL { "literal" };

	Expression convert_no_call_literal_inner(const ArenaString& literal, ExprContext& ctx, Expected& expected) {
		if (expected.has_expectation_or_inferred_type()) expected.as_if_checked(); else expected.set_inferred(ctx.string_type.get());
		return Expression(literal);
	}

	Expression convert_no_call_literal(const ArenaString& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.string_type) throw "todo: string type missing";
		const Type& string_type = ctx.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (current_expectation && !types_exactly_equal(current_expectation.get(), string_type)) throw "todo";
		return convert_no_call_literal_inner(literal, ctx, expected);
	}

	Expression convert_literal(const LiteralAst& literal, ExprContext& ctx, Expected& expected) {
		if (!ctx.string_type) throw "todo: string type missing";

		const Type& string_type = ctx.string_type.get();
		const Option<Type>& current_expectation = expected.get_current_expectation();
		if (literal.type_arguments.size() == 0 && literal.arguments.size() == 0 && (!current_expectation || types_exactly_equal(current_expectation.get(), string_type))) {
			return convert_no_call_literal_inner(literal.literal, ctx, expected);
		} else {
			//TODO:PERF
			Arena::SmallArrayBuilder<ExprAst> b = ctx.scratch_arena.small_array_builder<ExprAst>();
			b.add(literal.literal); // This is a NoCallLiteral
			for (const ExprAst &arg : literal.arguments)
				b.add(arg);
			return convert_call(LITERAL, b.finish(), literal.type_arguments, ctx, expected);
		}
	}

	Expression convert(const ExprAst& ast, ExprContext& ctx, Expected& expected) {
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
				return convert_literal(ast.literal(), ctx, expected);
			case ExprAst::Kind::NoCallLiteral:
				return convert_no_call_literal(ast.no_call_literal(), ctx, expected);
			case ExprAst::Kind::Call: {
				CallAst c = ast.call();
				return convert_call(c.fun_name, c.arguments, c.type_arguments, ctx, expected);
			}
			case ExprAst::Kind::StructCreate:
				return convert_struct_create(ast.struct_create(), ctx, expected);
			case ExprAst::Kind::TypeAnnotate: {
				return convert_type_annotate(ast.type_annotate(), ctx, expected);
			}
			case ExprAst::Kind::Let:
				return convert_let(ast.let(), ctx, expected);
			case ExprAst::Kind::When:
				return convert_when(ast.when(), ctx, expected);
		}
	}
}

Expression convert(
	const ExprAst& ast,
	Arena& arena,
	Arena& scratch_arena,
	const FunsTable& funs_table,
	const StructsTable& structs_table,
	const DynArray<Parameter>& parameters,
	const Option<Type>& bool_type,
	const Option<Type>& string_type,
	const Type& return_type) {
	ExprContext ctx { arena, scratch_arena, funs_table, structs_table, parameters, {}, bool_type, string_type };
	return convert_and_expect(ast, ctx, return_type);
}
