#include "Candidate.h"

#include "../model/types_equal_ignore_lifetime.h"

namespace {
	bool try_match_nested_types(const Type& from_candidate, const Type& from_external, Candidate& candidate);

	bool try_match_inst_structs(const InstStruct& from_candidate, const StoredType& from_external, Candidate& candidate) {
		if (from_external.is_type_parameter())
			// e.g:
			// 	takes-int Void(i Int)
			//  f Void(t ?T)
			//      t takes-int
			// This is an error because ?T is not guaranteed to be Int.
			return false;

		const InstStruct& actual_inst_struct = from_external.inst_struct();
		if (from_candidate.strukt != actual_inst_struct.strukt)
			return false;

		return each_corresponds(from_candidate.type_arguments, actual_inst_struct.type_arguments, [&](const Type& expected_type_argument, const Type& actual_type_argument) {
			return try_match_nested_types(expected_type_argument, actual_type_argument, candidate);
		});
	}

	bool try_match_with_inferring(InferringType& inferring, const StoredType& stored_type_from_external, Option<const Lifetime&> op_lifetime_from_external, Candidate& candidate) {
		if (inferring.stored_type.has()) {
			// Already inferred a type for this type parameter, so use that.
			const StoredType& inferred = inferring.stored_type.get();
			switch (inferred.kind()) {
				case StoredType::Kind::Nil:
					unreachable();
				case StoredType::Kind::Bogus:
					todo();
				case StoredType::Kind::InstStruct:
					if (!try_match_inst_structs(inferred.inst_struct(), stored_type_from_external, candidate))
						return false;
					break;
				case StoredType::Kind::TypeParameter:
					// The inferred type was another type parameter -- this can happen. Shouldn't be one of the function's own type parameters.
					assert(!contains_ref(candidate.signature->type_parameters, inferred.param()));
					if (inferred.param() != stored_type_from_external.param())
						return false;
					break;
			}
		} else {
			// Infer it now.
			inferring.stored_type = stored_type_from_external;
		}

		if (op_lifetime_from_external.has()) {
			const Lifetime& lifetime_from_external = op_lifetime_from_external.get();
			if (inferring.lifetime.has()) {
				// Lifetimes must match. (TODO: better way?)
				todo();
			} else {
				inferring.lifetime = lifetime_from_external;
			}
		}

		return true;
	}

	bool try_match_from_spec(Ref<const TypeParameter> candidate_type_parameter, const StoredType& stored_type_from_external, Candidate& candidate) {
		// If it wasn't a type parameter on the signature itself, must come from an enclosing spec.
		//   $S ?T
		//       Void f(?T t)
		const SpecUse& spec_use = *candidate.called.spec().spec_use;
		const StoredType& substituted = get_type_argument(spec_use.spec->type_parameters, spec_use.type_arguments, candidate_type_parameter).stored_type_or_bogus();
		switch (substituted.kind()) {
			case StoredType::Kind::Nil:
				unreachable();
			case StoredType::Kind::Bogus:
				todo();
			case StoredType::Kind::InstStruct:
				return try_match_inst_structs(substituted.inst_struct(), stored_type_from_external, candidate);
			case StoredType::Kind::TypeParameter: {
				Ref<const TypeParameter> param = substituted.param();
				// We just substituted, so this must be a type parameter from the function that had the SpecUse. So, nothing to do.
				assert(!contains_ref(candidate.signature->type_parameters, param) && !contains_ref(spec_use.spec->type_parameters, param));
				return param == stored_type_from_external.param();
			}
		}
		return try_match_stored_types(substituted, stored_type_from_external, candidate);
	}

	bool try_match_type_param(Ref<const TypeParameter> candidate_type_parameter, const StoredType& stored_type_from_external, Option<const Lifetime&> op_lifetime_from_external, Candidate& candidate) {
		// If a fn returns a type parameter, it must have been a type declared in that function.
		Option<uint> candidate_index = try_get_index(candidate.signature->type_parameters, candidate_type_parameter);
		if (candidate_index.has())
			// E.g., we have a candidate `f(?T t)` and call `f(true)`
			return try_match_with_inferring(candidate.inferring_type_arguments[candidate_index.get()], stored_type_from_external, op_lifetime_from_external, candidate);
		else
			return try_match_from_spec(candidate_type_parameter, stored_type_from_external, candidate);
	}

	// In type arguments, we *do* consider lifetimes.
	bool try_match_nested_types(const Type& from_candidate, const Type& from_external, Candidate& candidate) {
		const StoredType& stored_from_candidate = from_candidate.stored_type();
		if (stored_from_candidate.is_type_parameter()) {
			if (from_candidate.lifetime().is_borrow()) todo();
			return try_match_type_param(stored_from_candidate.param(), from_external.stored_type(), Option<const Lifetime&> { from_external.lifetime() }, candidate);
		} else {
			if ((1)) todo(); //lifetimes must match
			return try_match_inst_structs(stored_from_candidate.inst_struct(), from_external.stored_type(), candidate);
		}
	}
}

// type_from_external: either the expected return type, or the actual argument type.
bool try_match_stored_types(const StoredType& type_from_candidate, const StoredType& type_from_external, Candidate& candidate) {
	switch (type_from_candidate.kind()) {
		case StoredType::Kind::Nil:
			unreachable();
		case StoredType::Kind::Bogus:
			todo(); //Accept everything? Accept nothing?
		case StoredType::Kind::InstStruct:
			return try_match_inst_structs(type_from_candidate.inst_struct(), type_from_external, candidate);
		case StoredType::Kind::TypeParameter:
			// In a direct case, don't use parameter/return lifetime for type inference.
			// So given `f Void(t ?T)` and `l = 0; f(l)`, don't infer `?T` as `Int *loc`, just infer it as `Int`.
			// Note that *recursively* in a type we do use the actual lifetime:
			// Given `f Void(Vec<?T> v)` and `v = make-vec<Int *loc>; f(v)`, ?T is `Int`.
			return try_match_type_param(type_from_candidate.param(), type_from_external, /*lifetime_from_external*/ {}, candidate);
	}
}
