#include "./check_expr_call_common.h"

#include "../model/types_equal_ignore_lifetime.h"

namespace {
	bool does_type_match_no_infer(const StoredType& expected, const StoredType& actual) {
		assert(actual.kind() != StoredType::Kind::Nil);
		if (expected.kind() != actual.kind())
			return false;
		switch (expected.kind()) {
			case StoredType::Kind::Nil: unreachable();
			case StoredType::Kind::Bogus:
				return true;
			case StoredType::Kind::InstStruct:
				return actual.is_inst_struct() && types_equal_ignore_lifetime(actual.inst_struct(), expected.inst_struct());
			case StoredType::Kind::TypeParameter:
				return expected.param() == actual.param();
		}
	}
}

void Expected::check_no_infer(StoredType actual) {
	_was_checked = true;
	if (type.has()) {
		const StoredType& t = type.get();
		if (!does_type_match_no_infer(t, actual))
			todo(); // diagnostic and infer Bogus
	}
	else {
		assert(!_had_expectation);
		set_inferred(actual);
	}
}

void Expected::set_inferred(StoredType actual) {
	assert(!_had_expectation && !type.has());
	_was_checked = true;
	type = actual;
}
