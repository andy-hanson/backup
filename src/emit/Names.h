#pragma once

#include "../compile/model/model.h"
#include "../util/Alloc.h"
#include "../util/StringSlice.h"

#include "./concrete_fun.h"

struct Names {
	Map<ref<const StructDeclaration>, ArenaString> struct_names;
	Map<ref<const StructField>, ArenaString> field_names;
	Map<ref<const ConcreteFun>, ArenaString> fun_names;

	StringSlice get_name(ref<const StructDeclaration> s) const {
		return struct_names.must_get(s);
	}
	StringSlice get_name(ref<const StructField> f) const {
		return field_names.must_get(f);
	}
	StringSlice get_name(ref<const ConcreteFun> f) const {
		return fun_names.must_get(f);
	}
};

Names get_names(const Vec<ref<Module>>& modules, const FunInstantiations& fun_instantiations, Arena& arena);
