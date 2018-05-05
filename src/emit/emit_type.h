#pragma once

#include "../compile/model/model.h"
#include "../util/Writer.h"
#include "./mangle.h"

void write_type(Writer& out, const Type& t, const Names& names);

inline void write_type_arguments(Writer& out, const Slice<Type>& type_arguments, const Names& names) {
	if (type_arguments.empty()) return;
	out << '<';
	for (uint i = 0; i != type_arguments.size(); ++i) {
		if (i != 0) out << ", ";
		write_type(out, type_arguments[i], names);
	}
	out << '>';
}

inline void write_inst_struct(Writer& out, const InstStruct& i, const Names& names) {
	out << names.get_name(i.strukt);
	write_type_arguments(out, i.type_arguments, names);
}

inline void substitute_and_write_inst_struct(Writer& out, const ConcreteFun& current_fun, const Type& type, const Names& names, Arena& scratch_arena, bool is_own) {
	write_inst_struct(out, substitute_type_arguments(type, current_fun, scratch_arena), names);
	if (!is_own) out << '&';
}

inline void write_type(Writer& out, const Type& t, const Names& names) {
	if (t.is_parameter())
		out << mangle{t.param()->name};
	else
		write_inst_struct(out, t.inst_struct(), names);
}
