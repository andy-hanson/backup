#pragma once

#include "../../util/store/slice_util.h" // get_index
#include "../model/model.h"
#include "../model/expr.h"

struct Candidate {
	CalledDeclaration called;
	Ref<const FunSignature> signature;
	// DynArray in a scratch arena.
	// Note: if explicit type arguments are provided, these will already be filled.
	// These will be None if not yet inferred.
	Slice<Option<Type>> inferring_type_arguments;

	inline Candidate(CalledDeclaration _called, Ref<const FunSignature> _signature, Slice<Option<Type>> _inferring_type_arguments)
		: called(_called), signature(_signature), inferring_type_arguments(_inferring_type_arguments) {
		assert(signature->type_parameters.size() == inferring_type_arguments.size());
	}
};

template <typename T>
const T& get_type_argument(const Slice<TypeParameter>& type_parameters, const Slice<T>& type_arguments, Ref<const TypeParameter> type_parameter) {
	assert(type_parameters.size() == type_arguments.size());
	return type_arguments[get_index(type_parameters, type_parameter)];
}

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
