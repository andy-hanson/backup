#pragma once

#include "../util/store/Arena.h"
#include "../util/store/Map.h"
#include "../util/store/StringSlice.h"
#include "../util/Writer.h"
#include "../compile/model/model.h"

#include "ConcreteFun.h"

struct Names {
	// These will have an entry only if mangling is needed.
	Map<Ref<const EmittableStruct>, ArenaString, Ref<const EmittableStruct>::hash> struct_names;
	Map<Ref<const StructField>, ArenaString, Ref<const StructField>::hash> field_names;
	Map<Ref<const ConcreteFun>, ArenaString, Ref<const ConcreteFun>::hash> fun_names;

	struct StructNameWriter {
		const EmittableStruct& strukt;
		const Names& names;
		friend Writer& operator<<(Writer& out, const StructNameWriter& s);
	};
	inline StructNameWriter name(const EmittableStruct& e) const { return { e, *this }; }

	struct FieldNameWriter {
		const StructField& field;
		const Names& names;
		friend Writer& operator<<(Writer& out, const FieldNameWriter& f);
	};
	inline FieldNameWriter name(const StructField& f) const { return { f, *this }; }
	struct FunNameWriter {
		const ConcreteFun& fun;
		const Names& names;
		friend Writer& operator<<(Writer& out, const FunNameWriter& f);
	};
	inline FunNameWriter name(const ConcreteFun& f) const { return { f, *this }; }

	struct ParameterNameWriter {
		const Parameter& parameter;
		friend Writer& operator<<(Writer& out, const ParameterNameWriter& p);
	};
	inline ParameterNameWriter name(const Parameter& p) const { return { p }; }
};

Names get_names(const EmittableTypeCache& types, const ConcreteFunsCache& funs, Arena& out_arena);
