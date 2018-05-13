#pragma once

#include "../../util/store/slice_util.h" // get_index
#include "../model/model.h"
#include "../model/expr.h"

// Supports inferring the stored_type separately from inferring the lifetime.
struct InferringType {
	Option<StoredType> stored_type;
	Option<Lifetime> lifetime;

	inline InferringType() {}
	inline explicit InferringType(Type t) : stored_type{t.stored_type()}, lifetime{t.lifetime()} {}
};

struct Candidate {
	CalledDeclaration called;
	Ref<const FunSignature> signature;
	// Slice in a scratch arena.
	// Note: if explicit type arguments are provided, these will already be filled.
	// These will be None if not yet inferred.
	Slice<InferringType> inferring_type_arguments;

	inline Candidate(CalledDeclaration _called, Ref<const FunSignature> _signature, Slice<InferringType> _inferring_type_arguments)
		: called{_called}, signature{_signature}, inferring_type_arguments{_inferring_type_arguments} {
		assert(signature->type_parameters.size() == inferring_type_arguments.size());
	}
};

template <typename T>
const T& get_type_argument(const Slice<TypeParameter>& type_parameters, const Slice<T>& type_arguments, Ref<const TypeParameter> type_parameter) {
	assert(type_parameters.size() == type_arguments.size());
	return type_arguments[get_index(type_parameters, type_parameter)];
}

// Either: `try_match_types(candidate return type, expected return type)` or `try_match_types(candidate parameter type, actual argument type)`
//
// In both cases, the first argument is a type from outside imposed on the candidate signature.
// Note that this function infers type arguments and writes to candidate.inferring_type_arguments
//
// @return false if this candidate can't satisfy the latest constraint; true if that's still possible.
bool try_match_stored_types(const StoredType& type_from_candidate, const StoredType& type_from_external, Candidate& candidate);
