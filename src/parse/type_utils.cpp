#include "type_utils.h"

#include "../util/collection_util.h" // contains_ref, each_corresponds

namespace {
	bool try_match_plain_types(const PlainType& expected_plain, const Type& actual) {
		// If we expected a plain type, argument can't be a type parameter.
		// e.g: `fun Int takes-int(Int i) ... fun<T> Int f(T t) = takes-int t` is an error
		if (!actual.is_plain()) return false;
		const PlainType& actual_plain = actual.plain();
		if (actual_plain.effect < expected_plain.effect) return false;
		if (actual_plain.inst_struct.strukt != expected_plain.inst_struct.strukt) return false;
		//TODO: actually, we should recurse into these.
		return actual_plain.inst_struct.type_arguments == expected_plain.inst_struct.type_arguments;
	}

	bool try_match_type_param(ref<const TypeParameter> candidate_type_parameter, const Type& type_from_external, Candidate& candidate) {
		// If a fn returns a type parameter, it must have been a type declared in that function.
		Option<uint> candidate_index = try_get_index(candidate.signature->type_parameters, candidate_type_parameter);
		if (candidate_index) {
			// E.g., we have a candidate `<T> f(T t)` and call `f(true)`
			Option<Type>& inferring = candidate.inferring_type_arguments[candidate_index.get()];
			if (inferring) {
				// Already inferred a type for this type parameter, so use that.
				const Type& inferred = inferring.get();
				switch (inferred.kind()) {
					case Type::Kind::Nil:
						assert(false);
					case Type::Kind::Plain:
						// Same as top case.
						return type_from_external.is_plain() && try_match_plain_types(inferred.plain(), type_from_external);
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
				case Type::Kind::Nil: assert(false);
				case Type::Kind::Plain: return try_match_plain_types(substituted.plain(), type_from_external);
				case Type::Kind::Param: {
					ref<const TypeParameter> param = substituted.param();
					// We just substituted, so this must be a type parameter from the containing function, meaning nothing to do.
					assert(!contains_ref(candidate.signature->type_parameters, param) && !contains_ref(spec_use.spec->type_parameters, param));
					return param == type_from_external.param();
				}
			}
			return try_match_types(substituted, type_from_external, candidate);
		}
	}
}

PlainType substitute_type_arguments(const Type& t, const DynArray<TypeParameter>& type_parameters, const DynArray<PlainType>& type_arguments, Arena& arena) {
	switch (t.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Param:
			return substitute_type_arguments(t.param(), type_parameters, type_arguments);
		case Type::Kind::Plain: {
			const PlainType& p = t.plain();
			if (!some(p.inst_struct.type_arguments, [](const Type& ta) { return ta.is_parameter(); }))
				return p;
			DynArray<Type> new_type_arguments = arena.map_array<Type>()(p.inst_struct.type_arguments, [&](const Type& ta) {
				return Type { substitute_type_arguments(ta, type_parameters, type_arguments, arena) };
			});
			return PlainType { p.effect, { p.inst_struct.strukt, new_type_arguments } };
		}
	}
}

// Recursively replaces every type parameter with a corresponding type argument.
PlainType substitute_type_arguments(const TypeParameter& t, const DynArray<TypeParameter>& type_parameters, const DynArray<PlainType>& type_arguments) {
	assert(&type_parameters[t.index] == &t);
	return type_arguments[t.index];
}

// type_from_external: either the expected return type, or the actual argument type.
bool try_match_types(const Type& type_from_candidate, const Type& type_from_external, Candidate& candidate) {
	switch (type_from_candidate.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Plain:
			return try_match_plain_types(type_from_candidate.plain(), type_from_external);
		case Type::Kind::Param:
			return try_match_type_param(type_from_candidate.param(), type_from_external, candidate);
	}
}

bool does_type_match_no_infer(const Type& expected, const Type& actual) {
	if (expected.kind() != actual.kind()) return false;
	switch (expected.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Plain:
			return try_match_plain_types(expected.plain(), actual);
		case Type::Kind::Param:
			return expected.param() == actual.param();
	}
}

const Type& get_candidate_return_type(const Candidate& candidate) {
	const Type& rt = candidate.signature->return_type;
	switch (rt.kind()) {
		case Type::Kind::Nil: assert(false);
		case Type::Kind::Plain:
			return rt;
		case Type::Kind::Param: {
			const TypeParameter& candidate_type_parameter = rt.param();
			assert(contains_ref(candidate.signature->type_parameters, ref<const TypeParameter>(&candidate_type_parameter)));
			// Should have checked before this that we inferred everything, so get() should succeed.
			return candidate.inferring_type_arguments[candidate_type_parameter.index].get();
		}
	}
}
