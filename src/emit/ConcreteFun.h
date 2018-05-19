#pragma once

#include "../compile/model/model.h"
#include "../compile/model/expr.h" // Called

#include "../util/store/collection_util.h"
#include "../util/store/Map.h"
#include "../util/store/NonEmptyList.h"
#include "../util/store/map_of_lists_util.h" // TODO: just for TryInsertResult, move that
#include "../util/Ref.h"

#include "./EmittableType.h"

struct ConcreteFun {
	Ref<const FunDeclaration> fun_declaration;
	Slice<EmittableType> type_arguments;
	// Maps spec index -> signature index -> implementation
	Slice<Slice<Ref<const ConcreteFun>>> spec_impls;

	EmittableType return_type;
	Slice<EmittableType> parameter_types;

private:
	friend class ConcreteFunsCache;
	ConcreteFun(
		Ref<const FunDeclaration> _fun_declaration, Slice<EmittableType> _type_arguments, Slice<Slice<Ref<const ConcreteFun>>> _spec_impls,
		EmittableType return_type, Slice<EmittableType> parameter_types);
};

class ConcreteFunsCache {
	Arena arena;
	Map<Ref<const FunDeclaration>, NonEmptyList<ConcreteFun>, Ref<const FunDeclaration>::hash> funs_map;

public:
	inline ConcreteFunsCache() : arena{}, funs_map{64, arena} {}

	Ref<const ConcreteFun> get_concrete_fun_for_main(const FunDeclaration& main, EmittableTypeCache& type_cache);
	TryInsertResult<ConcreteFun> get_concrete_fun_for_call(Ref<const ConcreteFun> current_concrete_fun, const Called& called, EmittableTypeCache& type_cache);
	inline Option<const NonEmptyList<ConcreteFun>&> get_funs_for_declaration(const FunDeclaration& decl) const {
		return funs_map.get(&decl);
	}

	template <typename /*Ref<const FunDeclaration>, const NonEmptyList<ConcreteFun>&*/ Cb>
	void each(Cb cb) const {
		funs_map.each(cb);
	}
};
