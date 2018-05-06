#pragma once

#include "../util/io.h"
#include "../util/store/MaxSizeString.h"

struct TestFailure {
	enum class Kind { BaselineAdded, BaselineChanged, BaselineRemoved, CppCompilationFailed };
	Kind kind;
	MaxSizeString<128> loc;
};
