#include "Candidate.h"

namespace {
	bool try_match_inst_structs(const InstStruct& expected, const Type& actual) {
		// If we expected a plain type, argument can't be a type parameter.
		// e.g: `fun Int takes-int(Int i) ... fun<T> Int f(T t) = takes-int t` is an error
		return actual.is_inst_struct() && expected == actual.inst_struct();
	}

	bool try_match_type_param(Ref<const TypeParameter> candidate_type_parameter, const Type& type_from_external, Candidate& candidate) {
		// If a fn returns a type parameter, it must have been a type declared in that function.
		Option<uint> candidate_index = try_get_index(candidate.signature->type_parameters, candidate_type_parameter);
		if (candidate_index.has()) {
			// E.g., we have a candidate `<T> f(T t)` and call `f(true)`
			Option<Type>& inferring = candidate.inferring_type_arguments[candidate_index.get()];
			if (inferring.has()) {
				// Already inferred a type for this type parameter, so use that.
				const Type& inferred = inferring.get();
				switch (inferred.kind()) {
					case Type::Kind::Nil:
						unreachable();
					case Type::Kind::Bogus:
						todo();
					case Type::Kind::InstStruct:
						return try_match_inst_structs(inferred.inst_struct(), type_from_external);
					case Type::Kind::Param:
						// The inferred type was another type parameter -- this can happen. Shouldn't be one of the function's own type parameters.
						assert(!contains_ref(candidate.signature->type_parameters, inferred.param()));
						return inferred.param() == type_from_external.param();
				}
			} else {
				// Infer it now.
				inferring = type_from_external;
				return true;
			}
		} else {
			// We have `<T> spec $S ... Void f(T t)`.
			const SpecUse& spec_use = *candidate.called.spec().spec_use;
			const Type& substituted = get_type_argument(spec_use.spec->type_parameters, spec_use.type_arguments, candidate_type_parameter);
			switch (substituted.kind()) {
				case Type::Kind::Nil: unreachable();
				case Type::Kind::Bogus: todo();
				case Type::Kind::InstStruct: return try_match_inst_structs(substituted.inst_struct(), type_from_external);
				case Type::Kind::Param: {
					Ref<const TypeParameter> param = substituted.param();
					// We just substituted, so this must be a type parameter from the containing function, meaning nothing to do.
					assert(!contains_ref(candidate.signature->type_parameters, param) && !contains_ref(spec_use.spec->type_parameters, param));
					return param == type_from_external.param();
				}
			}
			return try_match_types(substituted, type_from_external, candidate);
		}
	}
}

// type_from_external: either the expected return type, or the actual argument type.
bool try_match_types(const Type& type_from_candidate, const Type& type_from_external, Candidate& candidate) {
	switch (type_from_candidate.kind()) {
		case Type::Kind::Nil: unreachable();
		case Type::Kind::Bogus: todo();
		case Type::Kind::InstStruct:
			return try_match_inst_structs(type_from_candidate.inst_struct(), type_from_external);
		case Type::Kind::Param:
			return try_match_type_param(type_from_candidate.param(), type_from_external, candidate);
	}
}

const Type& get_candidate_return_type(const Candidate& candidate) {
	const Type& rt = candidate.signature->return_type;
	if (!rt.is_parameter()) return rt;

	const TypeParameter& candidate_type_parameter = rt.param();
	assert(contains_ref(candidate.signature->type_parameters, Ref<const TypeParameter>(&candidate_type_parameter)));
	// Should have checked before this that we inferred everything, so get() should succeed.
	return candidate.inferring_type_arguments[candidate_type_parameter.index].get();
}
