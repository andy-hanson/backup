#pragma once

#include "../compile/model/model.h"
#include "../compile/model/expr.h" // Called

#include "../util/store/collection_util.h"
#include "../util/store/MaxSizeMap.h"
#include "../util/store/NonEmptyList.h"
#include "../util/Ref.h"

struct ConcreteFun {
	Ref<const FunDeclaration> fun_declaration;
	Slice<InstStruct> type_arguments;
	// Maps spec index -> signature index -> implementation
	Slice<Slice<Ref<const ConcreteFun>>> spec_impls;

	ConcreteFun(Ref<const FunDeclaration> _fun_declaration, Slice<InstStruct> _type_arguments, Slice<Slice<Ref<const ConcreteFun>>> _spec_impls);

	struct hash {
		hash_t operator()(const ConcreteFun& c) const;
	};
};
bool operator==(const ConcreteFun& a, const ConcreteFun& b);

struct ConcreteFunAndCalled {
	Ref<const ConcreteFun> fun;
	Ref<const Called> called;

	struct hash {
		hash_t operator()(const ConcreteFunAndCalled& c) const;
	};
};
bool operator==(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b);
inline bool operator!=(const ConcreteFunAndCalled& a, const ConcreteFunAndCalled& b) { return !(a == b); }

using FunInstantiations = MaxSizeMap<32, Ref<const FunDeclaration>, NonEmptyList<ConcreteFun>, Ref<const FunDeclaration>::hash>;
using ResolvedCalls = MaxSizeMap<32, ConcreteFunAndCalled, Ref<const ConcreteFun>, ConcreteFunAndCalled::hash>;
struct EveryConcreteFun {
	FunInstantiations fun_instantiations;
	ResolvedCalls resolved_calls;
};
EveryConcreteFun get_every_concrete_fun(const Slice<Module>& modules, Arena& scratch_arena);
