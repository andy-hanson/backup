#pragma once

#include "../compile/model/model.h"
#include "../util/Arena.h"
#include "../util/MaxSizeMap.h"
#include "../util/StringSlice.h"

#include "./concrete_fun.h"

struct Names {
	MaxSizeMap<32, Ref<const StructDeclaration>, ArenaString, Ref<const StructDeclaration>::hash> struct_names;
	MaxSizeMap<32, Ref<const StructField>, ArenaString, Ref<const StructField>::hash> field_names;
	MaxSizeMap<32, Ref<const ConcreteFun>, ArenaString, Ref<const ConcreteFun>::hash> fun_names;

	inline StringSlice get_name(Ref<const StructDeclaration> s) const {
		return struct_names.must_get(s);
	}
	inline StringSlice get_name(Ref<const StructField> f) const {
		return field_names.must_get(f);
	}
	inline StringSlice get_name(Ref<const ConcreteFun> f) const {
		return fun_names.must_get(f);
	}
};

Names get_names(const Slice<Module>& modules, const FunInstantiations& fun_instantiations, Arena& arena);
