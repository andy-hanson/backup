#pragma once

#include "../util/Writer.h"
#include "ConcreteFun.h"
#include "./Names.h"

void emit_body(Writer& out, Ref<const ConcreteFun> f, const Names& names, const ResolvedCalls& resolved_calls, Arena& scratch_arena);
