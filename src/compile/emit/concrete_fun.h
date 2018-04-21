#pragma once

#include "../model/model.h"
#include "../model/expr.h" // Called

#include "../../util/hash_util.h"
#include "../../util/ptr.h"
#include "../../util/collection_util.h"

#include "../check/type_utils.h"

struct ConcreteFun {
	const ref<const FunDeclaration> fun_declaration;
	const DynArray<PlainType> type_arguments;
	// Maps spec index -> signature index -> implementation
	const DynArray<DynArray<ref<const ConcreteFun>>> spec_impls;

	ConcreteFun(ref<const FunDeclaration> _fun_declaration, DynArray<PlainType> _type_arguments, DynArray<DynArray<ref<const ConcreteFun>>> _spec_impls)
		: fun_declaration(_fun_declaration), type_arguments(_type_arguments), spec_impls(_spec_impls) {
		assert(fun_declaration->signature.type_parameters.size() == type_arguments.size());
		assert(fun_declaration->signature.specs.size() == spec_impls.size());
		assert(each_corresponds(fun_declaration->signature.specs, spec_impls, [](const SpecUse& spec_use, const DynArray<ref<const ConcreteFun>>& sig_impls) {
			return spec_use.spec->signatures.size() == sig_impls.size();
		}));
		assert(every(type_arguments, [](const PlainType& p) { return p.is_deeply_plain(); }));
	}
};

inline bool operator==(const ConcreteFun& a, const ConcreteFun& b) {
	return a.fun_declaration == b.fun_declaration && a.type_arguments == b.type_arguments && a.spec_impls == b.spec_impls;
}

namespace std {
	template<>
	struct hash<ConcreteFun> {
		size_t operator()(const ConcreteFun& c) const {
			// Don't hash the spec_impls because that could lead to infinite recursion.
			return hash_combine(hash<::ref<const FunDeclaration> >{}(c.fun_declaration), hash_dyn_array(c.type_arguments));
		}
	};
};

struct ConcreteFunAndCalled {
	ref<const ConcreteFun> fun;
	ref<const Called> called;
};
namespace std {
	template <>
	struct hash<ConcreteFunAndCalled> {
		size_t operator()(const ConcreteFunAndCalled& c) const {
			return hash_combine(hash<::ref<const ConcreteFun> >{}(c.fun), hash<::ref<const Called> >{}(c.called));
		}
	};
}
inline bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) {
	return a.fun == b.fun && a.called == b.called;
}

inline PlainType substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena) {
	return substitute_type_arguments(type_argument, fun.fun_declaration->signature.type_parameters, fun.type_arguments, arena);
}

using FunInstantiations = Map<ref<const FunDeclaration>, Sett<ConcreteFun>>;
using ResolvedCalls = Map<ConcreteFunAndCalled, ref<const ConcreteFun>>;
struct EveryConcreteFun {
	FunInstantiations fun_instantiations;
	ResolvedCalls resolved_calls;
};
EveryConcreteFun get_every_concrete_fun(const Vec<ref<Module>>& modules, Arena& scratch_arena);
