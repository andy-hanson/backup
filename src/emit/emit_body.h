#pragma once

#include "./concrete_fun.h"
#include "./Names.h"
#include "./Writer.h"

void emit_body(Writer& out, ref<const ConcreteFun> f, const Names& names, const ResolvedCalls& resolved_calls, Arena& scratch_arena);
