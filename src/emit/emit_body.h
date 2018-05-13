#pragma once

#include "../util/Writer.h"
#include "../compile/model/BuiltinTypes.h"
#include "ConcreteFun.h"
#include "./Names.h"

void emit_body(Writer& out, Ref<const ConcreteFun> f, const Names& names, const BuiltinTypes& builtin_types, const ResolvedCalls& resolved_calls);
