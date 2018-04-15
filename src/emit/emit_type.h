#pragma once

#include "../model/model.h"

#include "./mangle.h"
#include "./Writer.h"

Writer& operator<<(Writer& out, const Type& t);

inline Writer& operator<<(Writer& out, const DynArray<Type>& type_arguments) {
	if (type_arguments.empty()) return out;
	out << '<';
	for (uint i = 0; i != type_arguments.size(); ++i) {
		if (i != 0) out << ", ";
		out << type_arguments[i];
	}
	return out << '>';
}

inline Writer& operator<<(Writer& out, const InstStruct& i) {
	return out << mangle{i.strukt->name} << i.type_arguments;
}

inline Writer& operator<<(Writer& out, const PlainType& p) {
	//ignore the effect
	return out << p.inst_struct;
}

inline Writer& operator<<(Writer& out, const Type& t) {
	return t.is_parameter() ? out << mangle{t.param()->name} : out << t.plain();
}
