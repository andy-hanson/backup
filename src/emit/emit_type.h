#pragma once

#include "../model/model.h"

#include "./mangle.h"
#include "./Writer.h"

void write_type(Writer& out, const Type& t, const Names& names);

inline void write_type_arguments(Writer& out, const DynArray<Type>& type_arguments, const Names& names) {
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

inline void write_plain_type(Writer& out, const PlainType& p, const Names& names) {
	write_inst_struct(out, p.inst_struct, names);
}

inline void write_type(Writer& out, const Type& t, const Names& names) {
	if (t.is_parameter())
		out << mangle{t.param()->name};
	else
		write_plain_type(out, t.plain(), names);
}
