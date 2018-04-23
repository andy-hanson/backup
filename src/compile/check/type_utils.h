#pragma once

#include "../../util/collection_util.h" // try_get_index
#include "../model/model.h"
#include "../model/expr.h"

struct Candidate {
	CalledDeclaration called;
	ref<const FunSignature> signature;
	// DynArray in a scratch arena.
	// Note: if explicit type arguments are provided, these will already be filled.
	// These will be None if not yet inferred.
	Arr<Option<Type>> inferring_type_arguments;

	Candidate(CalledDeclaration _called, ref<const FunSignature> _signature, Arr<Option<Type>> _inferring_type_arguments)
		: called(_called), signature(_signature), inferring_type_arguments(_inferring_type_arguments) {
		assert(signature->type_parameters.size() == inferring_type_arguments.size());
	}
};

template <typename T>
const T& get_type_argument(const Arr<TypeParameter>& type_parameters, const Arr<T>& type_arguments, ref<const TypeParameter> type_parameter) {
	assert(type_parameters.size() == type_arguments.size());
	return type_arguments[try_get_index(type_parameters, type_parameter).get()];
}

// Recursively replaces every type parameter with a corresponding type argument.
InstStruct substitute_type_arguments(const TypeParameter& t, const Arr<TypeParameter>& type_parameters, const Arr<InstStruct>& type_arguments);

/**
Either: try_match_types(expected return type, candidate return type)
Or: try_match_type(actual argument type, candidate parameter type)

In both cases, the first argument is a type from outside imposed on the candidate overload.
Note that this function may infer type arguments.

@return false if this candidate can't satisfy the latest constraint; true if that's still possible.
*/
bool try_match_types(const Type& type_from_candidate, const Type& type_from_external, Candidate& candidate);

bool does_type_match_no_infer(const Type& a, const Type& b);

const Type& get_candidate_return_type(const Candidate& candidate);
