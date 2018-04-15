#include "check_call.h"

#include "./check_expr.h"
#include "./convert_type.h"

namespace {
	using Candidates = MaxSizeVector<16, Candidate>;

	/** Returns an expected argument type if all candidates agree on it. */
	Expected get_common_overload_parameter_type(Candidates& candidates, uint arg_index) {
		Option <Type> expected;
		for (const Candidate& candidate : candidates) {
			// If we get a generic candidate and haven't inferred this parameter type yet, no expected type.
			ref<const Type> parameter_type = &candidate.signature->parameters[arg_index].type;
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
			return try_match_types(candidate.signature->parameters[arg_index].type, arg_type, candidate);
		});
		if (candidates.empty()) throw "todo";
	}

	// Special handling for unary calls because:
	// a) May be a struct access
	// b) We can be more efficient for overload resolution.
	Option <Expression> try_convert_struct_field_access(StringSlice fun_name, const ExprAst& argument, ExprContext& ctx, Expected& expected) {
		auto argAndType = check_and_infer(argument, ctx);
		Type arg_type = argAndType.type;
		if (arg_type.is_plain()) {
			const PlainType& plain = arg_type.plain();
			const StructDeclaration& strukt = plain.inst_struct.strukt;
			if (strukt.body.is_fields()) {
				if (!plain.inst_struct.type_arguments.empty()) throw "todo";
				//TODO: substitute type arguments here!
				Option<const StructField&> field = find(strukt.body.fields(), [fun_name](const StructField& f) { return f.name == fun_name; });
				if (field) {
					// TODO: also check plain.effect to narrow the field type
					expected.check_no_infer(field.get().type);
					return { StructFieldAccess { ctx.arena.emplace_copy(argAndType.expression), &field.get() }};
				}
			}
		}
		return {};
	}

	void maybe_add_candidate(Candidates& candidates, Called called, Arena& scratch_arena, const FunSignature& sig, const DynArray<Type>& explicit_type_arguments, size_t arity) {
		if (sig.arity() == arity && (explicit_type_arguments.empty() || sig.type_parameters.size() != explicit_type_arguments.size())) {
			DynArray<Option<Type>> inferring_type_arguments = scratch_arena.fill_array<Option<Type>>(sig.type_parameters.size())([&](uint i) {
				return explicit_type_arguments.empty() ? Option<Type> {} : Option<Type> { explicit_type_arguments[i] };
			});
			candidates.push({ called, &sig, inferring_type_arguments });
		}
	}

	Candidates get_initial_candidates(ExprContext& ctx, const StringSlice& fun_name, const DynArray<Type>& explicit_type_arguments, size_t arity) {
		Candidates candidates;

		Option<const OverloadGroup&> overloads = ctx.funs_table.get(fun_name);
		if (overloads)
			for (ref<const FunDeclaration> f : overloads.get().funs)
				maybe_add_candidate(candidates, f, ctx.scratch_arena, f->signature, explicit_type_arguments, arity);

		for (const SpecUse& spec_use : ctx.current_fun->signature.specs)
			for (const FunSignature& sig : spec_use.spec->signatures)
				if (sig.name == fun_name)
					maybe_add_candidate(candidates, SpecUseSig { &spec_use, &sig }, ctx.scratch_arena, sig, explicit_type_arguments, arity);

		return candidates;
	}

}

Expression check_call(const StringSlice& fun_name, const DynArray<ExprAst>& argument_asts, const DynArray<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected) {
	DynArray<Type> explicit_type_arguments = convert_type_arguments(type_argument_asts, ctx);
	size_t arity = argument_asts.size();
	if (arity == 1 && explicit_type_arguments.empty()) {
		Option<Expression> e = try_convert_struct_field_access(fun_name, argument_asts[0], ctx, expected);
		if (e) return e.get();
	}

	Candidates candidates = get_initial_candidates(ctx, fun_name, explicit_type_arguments, arity);

	if (candidates.empty()) throw "todo: no overload has that arity";

	// Can't just check each overload in order because we want each argument to have an expected type.
	bool already_checked_return_type = false;
	if (const Option<Type>& expected_return_type = expected.get_current_expectation()) {
		already_checked_return_type = true;
		expected.as_if_checked();
		filter_unordered(candidates, [&expected_return_type](Candidate& candidate) {
			return try_match_types(candidate.signature->return_type, expected_return_type.get(), candidate);
		});
		if (candidates.empty()) throw "todo: no overload returns what you wanted";
	}

	DynArray<Expression> arguments = ctx.arena.fill_array<Expression>(arity)([&](uint arg_idx) {
		Expected expected_this_arg = get_common_overload_parameter_type(candidates, arg_idx);
		Expression res = check(argument_asts[arg_idx], ctx, expected_this_arg);
		remove_overloads_given_argument_type(candidates, expected_this_arg, arg_idx);
		return res;
	});

	if (candidates.size() > 1) throw "todo: two identical candidates?";

	const Candidate& candidate = candidates[0];

	DynArray<Type> candidate_type_arguments = ctx.arena.map_array<Option<Type>, Type>(candidate.inferring_type_arguments)([](const Option<Type>& t){
		if (!t) throw "todo: didn't infer all type arguments";
		return t.get();
	});

	if (!already_checked_return_type) {
		expected.set_inferred(get_candidate_return_type(candidate));
	}

	return Call { { candidate.called, candidate_type_arguments }, arguments };
}
