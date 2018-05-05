#include "check_call.h"

#include "./check_expr.h"
#include "./convert_type.h"
#include "./scope.h"

namespace {
	using Candidates = MaxSizeVector<16, Candidate>;

	/** Returns an expected argument type if all candidates agree on it. */
	Expected get_common_overload_parameter_type(Candidates& candidates, uint arg_index) {
		Option <Type> expected;
		for (const Candidate& candidate : candidates) {
			// If we get a generic candidate and haven't inferred this parameter type yet, no expected type.
			ref<const Type> parameter_type = &candidate.signature->parameters[arg_index].type;
			if (parameter_type->is_parameter()) {
				Option<const Type&> inferred = get_type_argument(candidate.signature->type_parameters, candidate.inferring_type_arguments, parameter_type->param()).as_ref();
				if (!inferred.has()) return {}; // If there's at least one uninferred generic parameter here, can't have an expected type.
				parameter_type = &inferred.get();
			}
			if (!expected.has())
				expected = parameter_type;
			else if (parameter_type != expected.get())
				return {};
		}
		// .get() will succeed because we had at least one candidate coming in.
		return { expected.get() };
	}

	/** Remove functions that did not have that argument type. We will also have to fill in type parameters for some candidates here. */
	void remove_overloads_given_argument_type(Candidates& candidates, const Type& arg_type, uint arg_index) {
		filter_unordered(candidates, [arg_index, arg_type](Candidate& candidate) {
			return try_match_types(candidate.signature->parameters[arg_index].type, arg_type, candidate);
		});
		if (candidates.empty()) throw "todo";
	}

	template <typename /*CalledDeclaration => void*/ Cb>
	void each_initial_candidate(const ExprContext& ctx, const StringSlice& fun_name, Cb cb) {
		for (const SpecUse& spec_use : ctx.current_fun->signature.specs)
			for (const FunSignature& sig : spec_use.spec->signatures)
				if (sig.name == fun_name)
					cb(CalledDeclaration { SpecUseSig { &spec_use, &sig } });
		each_fun_with_name(ctx, fun_name, cb);
	}

	void get_initial_candidates(Candidates& candidates, ExprContext& ctx, const StringSlice& fun_name, const Arr<Type>& explicit_type_arguments, size_t arity) {
		each_initial_candidate(ctx, fun_name, [&](CalledDeclaration called) {
			const FunSignature& sig = called.sig();
			if (sig.arity() == arity && (explicit_type_arguments.empty() || sig.type_parameters.size() == explicit_type_arguments.size())) {
				Arr<Option<Type>> inferring_type_arguments = ctx.scratch_arena.fill_array<Option<Type>>()(sig.type_parameters.size(), [&](uint i) {
					return explicit_type_arguments.empty() ? Option<Type> {} : Option { explicit_type_arguments[i] };
				});
				candidates.push({ called, &sig, inferring_type_arguments });
			}
		});
	}

	struct TypeParametersAndArguments {
		Arr<TypeParameter> type_parameters;
		Arr<Type> type_arguments;
		TypeParametersAndArguments(Arr<TypeParameter> _type_parameters, Arr<Type> _type_arguments) : type_parameters(_type_parameters), type_arguments(_type_arguments) {
			assert(type_parameters.size() == type_arguments.size());
		}

		// Note: this is only a single-level substitute.
		const Type& substitute(const TypeParameter& p) const {
			return type_arguments[get_index<TypeParameter>(type_parameters, &p)];
		}
	};

	// Signature in a spec may have type arguments; spec itself may take type arguments via SpecUse; and the called candidate may take type arguments.
	// We don't perform substitution for the signature in a spec -- a generic spec must be matched by a generic function. Only the latter 2 have substitution.
	struct TypeArgumentsScope {
		TypeParametersAndArguments spec_use_type_args;
		TypeParametersAndArguments candidate_type_args;
	};

	template <typename /*(const Type&, const Type&) => bool*/ TypesEqual>
	bool inst_structs_equal(const InstStruct& a, const Type& bt, TypesEqual types_equal) {
		if (!bt.is_inst_struct()) return false;
		const InstStruct& b = bt.inst_struct();
		return a.strukt == b.strukt && each_corresponds(a.type_arguments, b.type_arguments, types_equal);
	}

	bool signature_types_equal_no_sub(const Type& a, const Type& actual) {
		switch (a.kind()) {
			case Type::Kind::Nil:
			case Type::Kind::Param:
				assert(false);
			case Type::Kind::Bogus:
				return false;
			case Type::Kind::InstStruct:
				return inst_structs_equal(a.inst_struct(), actual, signature_types_equal_no_sub);
		}
	}

	// substituted_type_from_spec will be the type argument to the SpecUse.
	// This may further need to be substituted by a type argument from the candidate.
	bool signature_types_equal_one_sub(const Type& substituted_from_spec, const Type& actual, const TypeParametersAndArguments& candidate_type_args) {
		switch (substituted_from_spec.kind()) {
			case Type::Kind::Nil:
				assert(false);
			case Type::Kind::Bogus:
				return false;
			case Type::Kind::Param:
				return signature_types_equal_no_sub(candidate_type_args.substitute(substituted_from_spec.param()), actual);
			case Type::Kind::InstStruct:
				return inst_structs_equal(substituted_from_spec.inst_struct(), actual, [&](const Type& spec_type, const Type& actual_type) {
					return signature_types_equal_one_sub(spec_type, actual_type, candidate_type_args);
				});
		}
	}

	// Like `==`, but allows corresponding type parameters to be considered equal.
	bool signature_types_equal(
		const FunSignature& spec_signature, const Type& type_from_spec,
		const FunSignature& actual_signature, const Type& actual,
		const TypeArgumentsScope& type_arguments_scope
	) {
		switch (type_from_spec.kind()) {
			case Type::Kind::Nil:
				assert(false);
			case Type::Kind::Bogus:
				return false; // Don't use a signature if we didn't resolve its types.
			case Type::Kind::Param: {
				const TypeParameter& p = type_from_spec.param();
				// It must be a type param of the spec signature, or of the spec itself.
				Option<uint> spec_index = try_get_index<TypeParameter>(spec_signature.type_parameters, &p);
				if (spec_index.has()) {
					// It's a type parameter from the spec signature, e.g. `T` in `<T> T foo()`
					// The actual signature must be using a type parameter too, and at the same index.
					return actual.is_parameter() && spec_index.get() == get_index(actual_signature.type_parameters, actual.param());
				} else {
					return signature_types_equal_one_sub(type_arguments_scope.spec_use_type_args.substitute(p), actual, type_arguments_scope.candidate_type_args);
				}
			}
			case Type::Kind::InstStruct:
				return inst_structs_equal(type_from_spec.inst_struct(), actual, [&](const Type& spec_type, const Type& actual_type) {
					return signature_types_equal(spec_signature, spec_type, actual_signature, actual_type, type_arguments_scope);
				});
		}
	}

	//TODO: support a function needing a sig here too. But beware infinite recursion.
	bool signature_matches(const FunSignature& spec_signature, const FunSignature& actual, const TypeArgumentsScope& type_arguments_scope) {
		assert(spec_signature.name == actual.name);
		if (!spec_signature.specs.empty() || !actual.specs.empty()) throw "todo";
		return each_corresponds(spec_signature.type_parameters, actual.type_parameters, [](const TypeParameter& a, const TypeParameter& b) { return a.name == b.name; })
			&& signature_types_equal(spec_signature, spec_signature.return_type, actual, actual.return_type, type_arguments_scope)
			&& each_corresponds(spec_signature.parameters, actual.parameters, [&](const Parameter& a, const Parameter& b) {
				return a.name == b.name && signature_types_equal(spec_signature, a.type, actual, b.type, type_arguments_scope);
			});
	}

	//Note: requires an exact match. So a concrete signature can't be matched by a generic function.
	//Also, we won't look in the current specs, because that's what we're trying to resolve. Specs can't be resolved by other specs.
	CalledDeclaration find_spec_signature_implementation(const ExprContext& ctx, const FunSignature& spec_signature, const TypeArgumentsScope& type_arguments_scope) {
		Option<CalledDeclaration> match;
		each_initial_candidate(ctx, spec_signature.name, [&](CalledDeclaration called) {
			if (signature_matches(spec_signature, called.sig(), type_arguments_scope)) {
				if (match.has()) throw "todo";
				match = called;
			}
		});
		if (!match.has()) throw "todo";
		return match.get();
	}

	// If the candidate we resolved has specs, fill them in.
	Called check_specs(ExprContext& ctx, CalledDeclaration called, Arr<Type> type_arguments) {
		if (called.kind() == CalledDeclaration::Kind::Spec) {
			if (called.sig().specs.size() != 0) throw "Todo: specs that have specs";
			return { called, type_arguments, {} };
		}

		Arr<Arr<CalledDeclaration>> spec_impls = map<Arr<CalledDeclaration>>()(ctx.check_ctx.arena, called.sig().specs, [&](const SpecUse& spec_use) {
			TypeArgumentsScope type_arguments_scope { { spec_use.spec->type_parameters, spec_use.type_arguments }, { called.sig().type_parameters, type_arguments } };
			return map<CalledDeclaration>()(ctx.check_ctx.arena, spec_use.spec->signatures, [&](const FunSignature& sig) {
				return find_spec_signature_implementation(ctx, sig, type_arguments_scope);
			});
		});

		return { called, type_arguments, spec_impls };
	}


	// Special handling for unary calls because:
	// a) May be a struct access
	// b) We can be more efficient for overload resolution.
	Option<Expression> try_convert_struct_field_access(StringSlice fun_name, const ExpressionAndType& argument_and_type, ExprContext& ctx, Expected& expected) {
		const Type& arg_type = argument_and_type.type;
		if (arg_type.is_inst_struct()) {
			const InstStruct& inst = arg_type.inst_struct();
			const StructDeclaration& strukt = inst.strukt;
			if (strukt.body.is_fields()) {
				if (!inst.type_arguments.empty()) throw "todo";
				//TODO: substitute type arguments here!
				Option<ref<const StructField>> field = find(strukt.body.fields(), [fun_name](const StructField& f) { return f.name == fun_name; });
				if (field.has()) {
					// TODO: also check plain.effect to narrow the field type
					expected.check_no_infer(field.get()->type);
					return Option { Expression { StructFieldAccess { ctx.check_ctx.arena.put(argument_and_type.expression), field.get() } } };
				}
			}
		}
		return {};
	}
}

Expression check_call(const StringSlice& fun_name, const Arr<ExprAst>& argument_asts, const Arr<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected) {
	Arr<Type> explicit_type_arguments = type_arguments_from_asts(type_argument_asts, ctx.check_ctx, ctx.structs_table, ctx.current_fun->signature.type_parameters);
	size_t arity = argument_asts.size();

	// Can never use an expected type in a unary call, because it might be a struct field access.
	const Option<ExpressionAndType> first_arg_and_type = arity == 1 && explicit_type_arguments.empty() ? Option{check_and_infer(argument_asts[0], ctx)} : Option<ExpressionAndType>{};
	if (first_arg_and_type.has()) {
		Option<Expression> e = try_convert_struct_field_access(fun_name, first_arg_and_type.get(), ctx, expected);
		if (e.has()) return e.get();
	}

	Candidates candidates;
	get_initial_candidates(candidates, ctx, fun_name, explicit_type_arguments, arity);
	if (candidates.empty()) throw "todo: no overload has that arity";

	// Can't just check each overload in order because we want each argument to have an expected type.
	bool already_checked_return_type = false;
	const Option<Type>& expected_return_type = expected.get_current_expectation();
	if (expected_return_type.has()) {
		already_checked_return_type = true;
		expected.as_if_checked();
		filter_unordered(candidates, [&](Candidate& candidate) {
			return try_match_types(candidate.signature->return_type, expected_return_type.get(), candidate);
		});
		if (candidates.empty()) throw "todo: no overload returns what you wanted";
	}

	Arr<Expression> arguments = ctx.check_ctx.arena.fill_array<Expression>()(arity, [&](uint arg_idx) {
		if (arg_idx == 0 && first_arg_and_type.has()) {
			const ExpressionAndType& f = first_arg_and_type.get();
			remove_overloads_given_argument_type(candidates, f.type, arg_idx);
			return f.expression;
		} else {
			Expected expected_this_arg = get_common_overload_parameter_type(candidates, arg_idx);
			Expression res = check(argument_asts[arg_idx], ctx, expected_this_arg);
			if (!expected.had_expectation())
				remove_overloads_given_argument_type(candidates, expected.inferred_type(), arg_idx);
			return res;
		}
	});

	if (candidates.size() > 1) throw "todo: two identical candidates?";
	const Candidate& candidate = candidates[0];

	Arr<Type> candidate_type_arguments = map<Type>()(ctx.check_ctx.arena, candidate.inferring_type_arguments, [](const Option<Type>& t){
		if (!t.has()) throw "todo: didn't infer all type arguments";
		return t.get();
	});

	if (!already_checked_return_type)
		expected.set_inferred(get_candidate_return_type(candidate));

	return Expression { Call { check_specs(ctx, candidate.called, candidate_type_arguments), arguments } };
}
