#include "check_call.h"

#include "../model/types_equal_ignore_lifetime.h"
#include "./Candidate.h"
#include "./check_expr.h"
#include "./convert_type.h"

namespace {
	template <typename /*CalledDeclaration => void*/ Cb>
	void each_fun_with_name(const ExprContext& ctx, const StringSlice& name, Cb cb) {
		ctx.funs_table.each_with_key(name, [&](Ref<const FunDeclaration> f) { cb(CalledDeclaration { f }); });
		for (Ref<const Module> m : ctx.check_ctx.imports)
			m->funs_table.each_with_key(name, [&](Ref<const FunDeclaration> f) {
				if (f->is_public) cb(CalledDeclaration { f });
			});
	}

	template <uint capacity, typename T, typename Pred>
	void filter_unordered(MaxSizeVector<capacity, T>& collection, Pred pred) {
		for (uint i = 0; i != collection.size(); ) {
			if (pred(collection[i])) {
				++i;
			} else {
				collection[i] = collection[collection.size() - 1];
				collection.pop();
			}
		}
	}

	using Candidates = MaxSizeVector<16, Candidate>;

	/** Returns an expected argument type if all candidates agree on it. */
	Expected get_common_overload_parameter_expected_stored_type(Candidates& candidates, uint arg_index) {
		Option<StoredType> expected;
		for (const Candidate& candidate : candidates) {
			// If we get a generic candidate and haven't inferred this parameter type yet, no expected type.
			Ref<const StoredType> parameter_stored_type = &candidate.signature->parameters[arg_index].type.stored_type();
			if (parameter_stored_type->is_bogus())
				return {};
			if (parameter_stored_type->is_type_parameter()) {
				const InferringType& inferred = get_type_argument(candidate.signature->type_parameters, candidate.inferring_type_arguments, parameter_stored_type->param());
				if (!inferred.stored_type.has()) return {}; // If there's at least one uninferred generic parameter here, can't have an expected type.
				parameter_stored_type = &inferred.stored_type.get();
				if (parameter_stored_type->is_bogus())
					return {};
			}
			if (!expected.has())
				expected = parameter_stored_type;
			else if (!types_equal_ignore_lifetime(parameter_stored_type, expected.get()))
				return {};
		}
		// .get() will succeed because we had at least one candidate coming in.
		return { expected.get() };
	}

	/** Remove functions that did not have that argument type. We will also have to fill in type parameters for some candidates here. */
	void remove_overloads_given_argument_type(Candidates& candidates, const StoredType& arg_type, uint arg_index) {
		filter_unordered(candidates, [arg_index, arg_type](Candidate& candidate) {
			return try_match_stored_types(candidate.signature->parameters[arg_index].type.stored_type_or_bogus(), arg_type, candidate);
		});
		if (candidates.is_empty()) todo();
	}

	template <typename /*CalledDeclaration => void*/ Cb>
	void each_initial_candidate(const ExprContext& ctx, const StringSlice& fun_name, Cb cb) {
		for (const SpecUse& spec_use : ctx.current_fun->signature.specs)
			for (const FunSignature& sig : spec_use.spec->signatures)
				if (sig.name == fun_name)
					cb(CalledDeclaration { SpecUseSig { &spec_use, &sig } });
		each_fun_with_name(ctx, fun_name, cb);
	}

	void get_initial_candidates(Candidates& candidates, ExprContext& ctx, const StringSlice& fun_name, const Slice<Type>& explicit_type_arguments, uint arity) {
		each_initial_candidate(ctx, fun_name, [&](CalledDeclaration called) {
			const FunSignature& sig = called.sig();
			if (sig.arity() == arity && (explicit_type_arguments.is_empty() || sig.type_parameters.size() == explicit_type_arguments.size())) {
				Slice<InferringType> inferring_type_arguments = fill_array<InferringType>()(ctx.scratch_arena, sig.type_parameters.size(), [&](uint i) {
					return explicit_type_arguments.is_empty() ? InferringType {} : InferringType { explicit_type_arguments[i] };
				});
				candidates.push(Candidate { called, &sig, inferring_type_arguments });
			}
		});
	}

	struct TypeParametersAndArguments {
		Slice<TypeParameter> type_parameters;
		Slice<Type> type_arguments;
		TypeParametersAndArguments(Slice<TypeParameter> _type_parameters, Slice<Type> _type_arguments) : type_parameters{_type_parameters}, type_arguments{_type_arguments} {
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
	bool inst_structs_equal(const InstStruct& a, const Type& b, TypesEqual types_equal) {
		if (b.is_bogus()) return true;
		const StoredType& bs = b.stored_type();
		if (!bs.is_inst_struct()) return false;
		const InstStruct& bis = bs.inst_struct();
		return a.strukt == bis.strukt && each_corresponds(a.type_arguments, bis.type_arguments, types_equal);
	}

	// signature_types_equal where no type parameter substitutions need to be made.
	bool signature_types_equal_no_sub(const Type& expected, const Type& actual) {
		if (expected.is_bogus() || actual.is_bogus())
			return true;
		const StoredType& expected_stored = expected.stored_type();
		assert(!expected_stored.is_type_parameter());
		return inst_structs_equal(expected_stored.inst_struct(), actual, signature_types_equal_no_sub);
	}

	// substituted_type_from_spec will be the type argument to the SpecUse.
	// This may further need to be substituted by a type argument from the candidate.
	bool signature_types_equal_one_sub(const Type& substituted_from_spec, const Type& actual, const TypeParametersAndArguments& candidate_type_args) {
		if (substituted_from_spec.is_bogus() || actual.is_bogus())
			return true;
		const StoredType& substituted_from_spec_stored = substituted_from_spec.stored_type();
		if (substituted_from_spec_stored.is_type_parameter())
			return signature_types_equal_no_sub(candidate_type_args.substitute(substituted_from_spec_stored.param()), actual);
		else
			return inst_structs_equal(substituted_from_spec_stored.inst_struct(), actual, [&](const Type& spec_type, const Type& actual_type) {
				return signature_types_equal_one_sub(spec_type, actual_type, candidate_type_args);
			});
	}

	// Like `==`, but allows corresponding type parameters to be considered equal.
	bool signature_types_equal(
		const FunSignature& spec_signature, const Type& type_from_spec,
		const FunSignature& actual_signature, const Type& actual,
		const TypeArgumentsScope& type_arguments_scope
	) {
		if (type_from_spec.is_bogus() || actual.is_bogus()) return true;
		const StoredType& type_from_spec_stored = type_from_spec.stored_type();
		if (type_from_spec_stored.is_type_parameter()) {
			const TypeParameter& p = type_from_spec_stored.param();
			// It must be a type param of the spec signature, or of the spec itself.
			Option<uint> spec_index = try_get_index<TypeParameter>(spec_signature.type_parameters, &p);
			if (spec_index.has())
				// It's a type parameter from the spec signature, e.g. `T` in `<T> T foo()`
				// The actual signature must be using a type parameter too, and at the same index.
				return actual.stored_type().is_type_parameter() && spec_index.get() == get_index(actual_signature.type_parameters, actual.stored_type().param());
			else
				return signature_types_equal_one_sub(type_arguments_scope.spec_use_type_args.substitute(p), actual, type_arguments_scope.candidate_type_args);
		} else
			return inst_structs_equal(type_from_spec_stored.inst_struct(), actual, [&](const Type& spec_type, const Type& actual_type) {
				return signature_types_equal(spec_signature, spec_type, actual_signature, actual_type, type_arguments_scope);
			});
	}

	//TODO: support a function needing a sig here too. But beware infinite recursion.
	bool signature_matches(const FunSignature& spec_signature, const FunSignature& actual, const TypeArgumentsScope& type_arguments_scope) {
		assert(spec_signature.name == actual.name);
		if (!spec_signature.specs.is_empty() || !actual.specs.is_empty()) todo();
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
				if (match.has()) todo();
				match = called;
			}
		});
		if (!match.has()) todo();
		return match.get();
	}

	// If the candidate we resolved has specs, fill them in.
	Called check_specs(ExprContext& ctx, CalledDeclaration called, Slice<Type> type_arguments) {
		if (called.kind() == CalledDeclaration::Kind::Spec) {
			if (called.sig().specs.size() != 0) todo(); // specs that have specs?
			return { called, type_arguments, {} };
		}

		Slice<Slice<CalledDeclaration>> spec_impls = map<Slice<CalledDeclaration>>()(ctx.check_ctx.arena, called.sig().specs, [&](const SpecUse& spec_use) {
			TypeArgumentsScope type_arguments_scope { { spec_use.spec->type_parameters, spec_use.type_arguments }, { called.sig().type_parameters, type_arguments } };
			return map<CalledDeclaration>()(ctx.check_ctx.arena, spec_use.spec->signatures, [&](const FunSignature& sig) {
				return find_spec_signature_implementation(ctx, sig, type_arguments_scope);
			});
		});

		return { called, type_arguments, spec_impls };
	}

	Type struct_field_access_type(const Lifetime& target_lifetime __attribute__((unused)), const InstStruct& inst __attribute__((unused)), const StructField& field __attribute__((unused))) {
		todo();
	}

	// Special handling for unary calls because:
	// a) May be a struct access
	// b) We can be more efficient for overload resolution.
	Option<StructFieldAccess> try_convert_struct_field_access(StringSlice fun_name, const ExpressionAndType& argument_and_type, ExprContext& ctx, Expected& expected) {
		const Type& arg_type = argument_and_type.type;
		if (!arg_type.stored_type().is_inst_struct()) return {};

		const InstStruct& inst = arg_type.stored_type().inst_struct();
		const StructDeclaration& strukt = inst.strukt;
		if (!strukt.body.is_fields()) return {};

		Option<Ref<const StructField>> field = find(strukt.body.fields(), [fun_name](const StructField& f) { return f.name == fun_name; });
		if (!field.has()) return {};

		if (!inst.type_arguments.is_empty()) todo(); // TODO: Need to instantiate field type

		const StoredType& stored_type = field.get()->type.stored_type_or_bogus();
		expected.check_no_infer(stored_type);
		return Option { StructFieldAccess { struct_field_access_type(arg_type.lifetime(), inst, field.get()), ctx.check_ctx.arena.put(argument_and_type.expression), field.get() } };
	}

	Slice<Type> get_candidate_type_arguments_from_inferred(Arena& arena, const Slice<InferringType>& inferred) {
		return map<Type>()(arena, inferred, [&](const InferringType& i) {
			if (!i.stored_type.has() || !i.lifetime.has()) todo();
			return Type { i.stored_type.get(), i.lifetime.get() };
		});
	}

	Lifetime get_return_lifetime(const Lifetime& declared_return_lifetime, const Slice<Parameter>& parameters, const Slice<Lifetime>& argument_lifetimes) {
		if (!declared_return_lifetime.is_pointer())
			return Lifetime::noborrow();

		if (declared_return_lifetime.has_ret() || declared_return_lifetime.has_loc()) {
			todo(); // `f Int *ret()` is silly, and `f Int *loc()` is illegal (can't return a pointer to a local)
		}
		if (declared_return_lifetime.has_lifetime_variables()) todo();

		//For each parameter: if it borrows from that parameter, returned lifetime needs to be scoped by that.
		Lifetime::Builder builder;
		for (const Parameter& p : parameters) {
			if (declared_return_lifetime.has_parameter(p.index)) {
				const Lifetime& argument_lifetime = argument_lifetimes[p.index];
				// If the argument is new, it gets made into a local.
				builder.add(argument_lifetime.is_pointer() ? argument_lifetime : Lifetime::local_borrow());
			}
		}

		Lifetime res = builder.finish();
		assert(res.is_pointer());
		return res;
	}

	Type get_concrete_return_type(const FunSignature& signature, const Slice<Type>& type_arguments, const Slice<Lifetime>& argument_lifetimes) {
		const Type& rt = signature.return_type;
		if (!rt.stored_type().is_type_parameter()) {
			//TODO: if this type borrows from parameters, need to look at parameter lifetimes.
			const Lifetime& declared_return_lifetime = rt.lifetime();
			return { rt.stored_type(), get_return_lifetime(declared_return_lifetime, signature.parameters, argument_lifetimes) };
		} else {
			const TypeParameter& type_parameter = rt.stored_type().param();
			assert(contains_ref(signature.type_parameters, Ref<const TypeParameter>(&type_parameter)));
			// Should have checked before this that we inferred everything, so get() should succeed.
			const Type& type_argument __attribute__((unused)) = type_arguments[type_parameter.index];
			todo();
		}
	}

	Pair<ExpressionAndLifetime, StoredType> check_call_after_choosing_overload(
		const Candidate& candidate, const Slice<Type>& explicit_type_arguments, const Slice<Expression>& arguments, const Slice<Lifetime>& argument_lifetimes, ExprContext& ctx) {
		Slice<Type> candidate_type_arguments = candidate.signature->type_parameters.is_empty() ? Slice<Type> {}
			: !explicit_type_arguments.is_empty() ? explicit_type_arguments
			: get_candidate_type_arguments_from_inferred(ctx.check_ctx.arena, candidate.inferring_type_arguments);
		//TODO: check that arguments satisfy *their* lifetime bounds here.
		Type concrete_return_type = get_concrete_return_type(candidate.signature, candidate_type_arguments, argument_lifetimes);

		return {
			ExpressionAndLifetime { Expression { Call { concrete_return_type, check_specs(ctx, candidate.called, candidate_type_arguments), arguments } }, concrete_return_type.lifetime() },
			concrete_return_type.stored_type()
		};
	}
}

ExpressionAndLifetime check_call(const StringSlice& fun_name, const Slice<ExprAst>& argument_asts, const Slice<TypeAst>& type_argument_asts, ExprContext& ctx, Expected& expected) {
	Slice<Type> explicit_type_arguments = type_arguments_from_asts(
		type_argument_asts, ctx.check_ctx, ctx.structs_table, Option<const Slice<Parameter>&> { ctx.current_fun->signature.parameters }, ctx.current_fun->signature.type_parameters);
	uint arity = argument_asts.size();

	// Can never use an expected type in a unary call, because it might be a struct field access.
	const Option<ExpressionAndType> first_arg_and_type = arity == 1 && explicit_type_arguments.is_empty() ? Option{check_and_infer(argument_asts[0], ctx)} : Option<ExpressionAndType>{};
	if (first_arg_and_type.has()) {
		Option<StructFieldAccess> fa = try_convert_struct_field_access(fun_name, first_arg_and_type.get(), ctx, expected);
		if (fa.has()) return { Expression { fa.get() }, fa.get().accessed_field_type.lifetime() };
	}

	Candidates candidates;
	get_initial_candidates(candidates, ctx, fun_name, explicit_type_arguments, arity);
	if (candidates.is_empty()) todo(); // Diagnostic: no overload has that arity

	// Can't just check each overload in order because we want each argument to have an expected type.
	bool already_checked_return_type = false;
	const Option<StoredType>& expected_return_type = expected.get_current_expectation();
	if (expected_return_type.has()) {
		already_checked_return_type = true;
		expected.as_if_checked();
		filter_unordered(candidates, [&](Candidate& candidate) {
			return try_match_stored_types(candidate.signature->return_type.stored_type(), expected_return_type.get(), candidate);
		});
		if (candidates.is_empty()) todo(); // no overload returns what you wanted
	}

	Pair<Slice<Expression>, Slice<Lifetime>> arguments = fill_two_arrays<Expression, Lifetime>()(ctx.check_ctx.arena, ctx.scratch_arena, arity, [&](uint arg_idx) -> Pair<Expression, Lifetime> {
		if (arg_idx == 0 && first_arg_and_type.has()) {
			const ExpressionAndType& f = first_arg_and_type.get();
			remove_overloads_given_argument_type(candidates, f.type.stored_type(), arg_idx);
			return { f.expression, f.type.lifetime() };
		} else {
			Expected expected_this_arg = get_common_overload_parameter_expected_stored_type(candidates, arg_idx);
			ExpressionAndLifetime res = check_expr(argument_asts[arg_idx], ctx, expected_this_arg);
			if (!expected.had_expectation())
				remove_overloads_given_argument_type(candidates, expected.inferred_stored_type(), arg_idx);
			return { res.expression, res.lifetime };
		}
	});

	if (candidates.size() > 1) todo(); // two StoredType-identical candidates. Note: we don't allow overloading by lifetime.

	Pair<ExpressionAndLifetime, StoredType> res = check_call_after_choosing_overload(candidates[0], explicit_type_arguments, arguments.first, arguments.second, ctx);
	if (!already_checked_return_type)
		expected.set_inferred(res.second);
	return res.first;
}
