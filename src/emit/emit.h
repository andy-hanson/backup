#pragma once

#include "../util/Writer.h"
#include "../compile/model/BuiltinTypes.h"
#include "../compile/model/model.h"

Writer::Output emit(const Slice<Module>& modules, const BuiltinTypes& builtin_types, Arena& out_arena);
