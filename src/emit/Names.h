#pragma once

#include "../compile/model/model.h"
#include "../util/Arena.h"
#include "../util/MaxSizeMap.h"
#include "../util/StringSlice.h"

#include "./concrete_fun.h"

struct Names {
	MaxSizeMap<32, ref<const StructDeclaration>, ArenaString, ref<const StructDeclaration>::hash> struct_names;
	MaxSizeMap<32, ref<const StructField>, ArenaString, ref<const StructField>::hash> field_names;
	MaxSizeMap<32, ref<const ConcreteFun>, ArenaString, ref<const ConcreteFun>::hash> fun_names;

	inline StringSlice get_name(ref<const StructDeclaration> s) const {
		return struct_names.must_get(s);
	}
	inline StringSlice get_name(ref<const StructField> f) const {
		return field_names.must_get(f);
	}
	inline StringSlice get_name(ref<const ConcreteFun> f) const {
		return fun_names.must_get(f);
	}
};

Names get_names(const Arr<Module>& modules, const FunInstantiations& fun_instantiations, Arena& arena);
