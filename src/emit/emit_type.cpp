#include "./emit_type.h"

#include "./substitute_type_arguments.h"

void write_type(Writer& out, const Type& t, const Names& names) {
	const StoredType& s = t.stored_type();
	if (s.is_type_parameter())
		out << mangle{s.param()->name};
	else
		write_inst_struct(out, s.inst_struct(), names);

	if (t.lifetime().is_borrow()) out << '*';
}

void write_type_arguments(Writer& out, const Slice<Type>& type_arguments, const Names& names) {
	if (type_arguments.is_empty()) return;
	out << '<';
	for (uint i = 0; i != type_arguments.size(); ++i) {
		if (i != 0) out << ", ";
		write_type(out, type_arguments[i], names);
	}
	out << '>';
}

void write_inst_struct(Writer& out, const InstStruct& i, const Names& names) {
	out << names.get_name(i.strukt);
	write_type_arguments(out, i.type_arguments, names);
}

void substitute_and_write_inst_struct(Writer& out, const ConcreteFun& current_fun, const StoredType& type, const Names& names) {
	Arena scratch;
	write_inst_struct(out, substitute_type_arguments(type, current_fun, scratch), names);
}
