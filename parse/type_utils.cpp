#include "type_utils.h"

#include "../util/collection_util.h" // contains_ref, each_corresponds

namespace {
	bool plain_types_exactly_equal(const PlainType& a, const PlainType& b) {
		return a.effect == b.effect && a.inst_struct.strukt == b.inst_struct.strukt && each_corresponds(a.inst_struct.type_arguments, b.inst_struct.type_arguments, types_exactly_equal);
	}

	bool try_match_plain_types(const PlainType& expected_plain, const PlainType& actual_plain) {
		if (actual_plain.effect < expected_plain.effect) return false;
		if (actual_plain.inst_struct.strukt != expected_plain.inst_struct.strukt) return false;
		return each_corresponds(actual_plain.inst_struct.type_arguments, expected_plain.inst_struct.type_arguments, types_exactly_equal);
	}
}

bool types_exactly_equal(const Type& a, const Type& b) {
	if (a.kind() != b.kind()) return false;
	switch (a.kind()) {
		case Type::Kind::Plain:
			return plain_types_exactly_equal(a.plain(), b.plain());
		case Type::Kind::Param:
			return a.param() == b.param();
	}
}


bool try_match_types(const Type& type_from_candidate, const Type& type_from_external, Candidate& candidate) {
	switch (type_from_candidate.kind()) {
		case Type::Kind::Plain:
			// If we expected a plain type, argument can't be a type parameter.
			// e.g: `fun Int takes-int(Int i) ... fun<T> Int f(T t) = takes-int t` is an error
			return type_from_external.is_plain() && try_match_plain_types(type_from_candidate.plain(), type_from_external.plain());
		case Type::Kind::Param: {
			// If a fn returns a type parameter, it must have been a type declared in that function.
			const TypeParameter& candidate_type_parameter = type_from_candidate.param();
			assert(contains_ref(candidate.fun->type_parameters, ref<const TypeParameter>(&candidate_type_parameter)));
			Option<Type>& inferring = candidate.inferring_type_arguments[candidate_type_parameter.index];
			if (inferring) {
				// Already inferred a type for this type parameter, so use that.
				const Type& inferred = inferring.get();
				switch (inferred.kind()) {
					case Type::Kind::Plain:
						// Same as top case.
						return type_from_external.is_plain() && try_match_plain_types(inferred.plain(), type_from_external.plain());
					case Type::Kind::Param:
						// The inferred type was another type parameter -- this can happen. Shouldn't be one of the function's own type parameters.
						assert(!contains_ref(candidate.fun->type_parameters, inferred.param()));
						return inferred.param() == type_from_external.param();
				}
			} else {
				// Infer it now.
				inferring = type_from_external;
				return true;
			}
		}
	}
}

bool does_type_match_no_infer(const Type& expected, const Type& actual) {
	if (expected.kind() != actual.kind()) return false;
	switch (expected.kind()) {
		case Type::Kind::Plain:
			return try_match_plain_types(expected.plain(), actual.plain());
		case Type::Kind::Param:
			return expected.param() == actual.param();
	}
}

const Type& get_candidate_return_type(const Candidate& candidate) {
	const Type& rt = candidate.fun->return_type;
	switch (rt.kind()) {
		case Type::Kind::Plain:
			return rt;
		case Type::Kind::Param: {
			const TypeParameter& candidate_type_parameter = rt.param();
			assert(contains_ref(candidate.fun->type_parameters, ref<const TypeParameter>(&candidate_type_parameter)));
			// Should have checked before this that we inferred everything, so get() should succeed.
			return candidate.inferring_type_arguments[candidate_type_parameter.index].get();
		}
	}
}
