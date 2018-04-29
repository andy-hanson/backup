#pragma once

#include "../compile/model/model.h"
#include "../compile/model/expr.h" // Called

#include "../util/hash_util.h"
#include "../util/ptr.h"
#include "../util/collection_util.h"

struct ConcreteFun {
	const ref<const FunDeclaration> fun_declaration;
	const Arr<InstStruct> type_arguments;
	// Maps spec index -> signature index -> implementation
	const Arr<Arr<ref<const ConcreteFun>>> spec_impls;

	ConcreteFun(ref<const FunDeclaration> _fun_declaration, Arr<InstStruct> _type_arguments, Arr<Arr<ref<const ConcreteFun>>> _spec_impls)
		: fun_declaration(_fun_declaration), type_arguments(_type_arguments), spec_impls(_spec_impls) {
		assert(fun_declaration->signature.type_parameters.size() == type_arguments.size());
		assert(fun_declaration->signature.specs.size() == spec_impls.size());
		assert(each_corresponds(fun_declaration->signature.specs, spec_impls, [](const SpecUse& spec_use, const Arr<ref<const ConcreteFun>>& sig_impls) {
			return spec_use.spec->signatures.size() == sig_impls.size();
		}));
		assert(every(type_arguments, [](const InstStruct& p) { return p.is_deeply_concrete(); }));
	}

	struct hash {
		size_t operator()(const ConcreteFun& c) const {
			// Don't hash the spec_impls because that could lead to infinite recursion.
			return hash_combine(ref<const FunDeclaration>::hash{}(c.fun_declaration), hash_dyn_array(c.type_arguments, InstStruct::hash{}));
		}
	};
};

bool operator==(const ConcreteFun& a, const ConcreteFun& b);

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena);

struct ConcreteFunAndCalled {
	ref<const ConcreteFun> fun;
	ref<const Called> called;

	struct hash {
		size_t operator()(const ConcreteFunAndCalled& c) const {
			return hash_combine(ref<const ConcreteFun>::hash{}(c.fun), ref<const Called>::hash{}(c.called));
		}
	};
};
inline bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) {
	return a.fun == b.fun && a.called == b.called;
}

using FunInstantiations = Map<ref<const FunDeclaration>, Set<ConcreteFun, ConcreteFun::hash>, ref<const FunDeclaration>::hash>;
using ResolvedCalls = Map<ConcreteFunAndCalled, ref<const ConcreteFun>, ConcreteFunAndCalled::hash>;
struct EveryConcreteFun {
	FunInstantiations fun_instantiations;
	ResolvedCalls resolved_calls;
};
EveryConcreteFun get_every_concrete_fun(const Vec<ref<Module>>& modules, Arena& scratch_arena);
