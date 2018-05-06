#pragma once

#include "../compile/model/model.h"
#include "../util/Writer.h"
#include "./mangle.h"

#include "./Names.h"

void write_type(Writer& out, const Type& t, const Names& names);

void write_type_arguments(Writer& out, const Slice<Type>& type_arguments, const Names& names);

void write_inst_struct(Writer& out, const InstStruct& i, const Names& names);

void substitute_and_write_inst_struct(Writer& out, const ConcreteFun& current_fun, const Type& type, const Names& names, Arena& scratch_arena, bool is_own);
