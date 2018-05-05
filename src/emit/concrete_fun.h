#pragma once

#include "../compile/model/model.h"
#include "../compile/model/expr.h" // Called

#include "../util/collection_util.h"
#include "../util/ptr.h"
#include "../util/MaxSizeMap.h"

struct ConcreteFun {
	ref<const FunDeclaration> fun_declaration;
	Arr<InstStruct> type_arguments;
	// Maps spec index -> signature index -> implementation
	Arr<Arr<ref<const ConcreteFun>>> spec_impls;

	ConcreteFun(ref<const FunDeclaration> _fun_declaration, Arr<InstStruct> _type_arguments, Arr<Arr<ref<const ConcreteFun>>> _spec_impls);

	struct hash {
		hash_t operator()(const ConcreteFun& c) const;
	};
};

bool operator==(const ConcreteFun& a, const ConcreteFun& b);

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena);

struct ConcreteFunAndCalled {
	ref<const ConcreteFun> fun;
	ref<const Called> called;

	struct hash {
		hash_t operator()(const ConcreteFunAndCalled& c) const;
	};
};
bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b);
inline bool operator!=(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) { return !(a == b); }

using FunInstantiations = MaxSizeMap<32, ref<const FunDeclaration>, NonEmptyList<ConcreteFun>, ref<const FunDeclaration>::hash>;
using ResolvedCalls = MaxSizeMap<32, ConcreteFunAndCalled, ref<const ConcreteFun>, ConcreteFunAndCalled::hash>;
struct EveryConcreteFun {
	FunInstantiations fun_instantiations;
	ResolvedCalls resolved_calls;
};
EveryConcreteFun get_every_concrete_fun(const Arr<Module>& modules, Arena& scratch_arena);
