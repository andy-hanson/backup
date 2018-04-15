#pragma once

#include "../model/model.h"
#include "../model/expr.h"

struct Candidate {
	CalledDeclaration called;
	ref<const FunSignature> signature;
	// DynArray in a scratch arena.
	// Note: if explicit type arguments are provided, these will already be filled.
	// These will be None if not yet inferred.
	DynArray<Option<Type>> inferring_type_arguments;
};


PlainType substitute_type_arguments(const Type& t, const DynArray<TypeParameter>& type_parameters, const DynArray<PlainType>& type_arguments, Arena& arena);

// Recursively replaces every type parameter with a corresponding type argument.
PlainType substitute_type_arguments(const TypeParameter& t, const DynArray<TypeParameter>& type_parameters, const DynArray<PlainType>& type_arguments, Arena& arena);

bool types_exactly_equal(const Type& a, const Type& b);

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
