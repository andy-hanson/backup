#pragma once

#include "../util/Writer.h"
#include "../compile/model/BuiltinTypes.h"
#include "./CAst.h"
#include "./ConcreteFun.h"
#include "./Names.h"

using ToEmit = MaxSizeVector<16, Ref<const ConcreteFun>>;

CFunctionBody emit_body(
	Ref<const ConcreteFun> f,
	const BuiltinTypes& builtin_types,
	ConcreteFunsCache& concrete_funs,
	EmittableTypeCache& types_cache,
	ToEmit& to_emit,
	Arena& out_arena);
