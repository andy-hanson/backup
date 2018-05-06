#pragma once

#include "../util/io.h"

struct TestFailure {
	enum class Kind { BaselineAdded, BaselineChanged, BaselineRemoved, CppCompilationFailed };
	Kind kind;
	FileLocator loc;
};
