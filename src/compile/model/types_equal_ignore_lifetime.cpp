#include "./types_equal_ignore_lifetime.h"

#include "../../util/store/slice_util.h"

bool types_equal_ignore_lifetime(const InstStruct& a, const InstStruct& b) {
	return a.strukt == b.strukt && each_corresponds(a.type_arguments, b.type_arguments, [](const Type& aa, const Type& bb) {
		return types_equal_ignore_lifetime(aa.stored_type(), bb.stored_type());
	});
}

bool types_equal_ignore_lifetime(const StoredType& a, const StoredType& b) {
	if (a.is_type_parameter()) {
		return b.is_type_parameter() && a.param() == b.param();
	} else {
		return b.is_inst_struct() && types_equal_ignore_lifetime(a.inst_struct(), b.inst_struct());
	}
}
