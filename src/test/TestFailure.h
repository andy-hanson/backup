#pragma once

#include "../util/io.h"

struct TestFailure {
	enum class Kind { UnexpectedBaseline, CppCompilationFailed };
	Kind kind;
	FileLocator loc;
};
