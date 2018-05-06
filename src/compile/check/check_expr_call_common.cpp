#include "./check_expr_call_common.h"

namespace {
	bool does_type_match_no_infer(const Type& expected, const Type& actual) {
		if (expected.kind() != actual.kind()) return false;
		switch (expected.kind()) {
			case Type::Kind::Nil: unreachable();
			case Type::Kind::Bogus:
				return true;
			case Type::Kind::InstStruct:
				return actual.is_inst_struct() && actual.inst_struct() == expected.inst_struct();
			case Type::Kind::Param:
				return expected.param() == actual.param();
		}
	}
}

void Expected::check_no_infer(Type actual) {
	_was_checked = true;
	if (type.has()) {
		Type t = type.get();
		if (!does_type_match_no_infer(t, actual))
			todo(); // diagnostic and infer Bogus
	}
	else {
		assert(!_had_expectation);
		set_inferred(actual);
	}
}

void Expected::set_inferred(Type actual) {
	assert(!_had_expectation && !type.has());
	_was_checked = true;
	type = actual;
}
