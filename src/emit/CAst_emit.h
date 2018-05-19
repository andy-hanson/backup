#pragma once

#include "../util/Writer.h"
#include "./CAst.h"
#include "./Names.h"

void write_emittable_struct(Writer& out, const EmittableStruct& e, const Names& names);
void write_fun_header(Writer& out, const ConcreteFun& cf, const Names& names);
void write_fun_implementation(Writer& out, const ConcreteFun& cf, const CFunctionBody& body, const Names& names);
