#pragma once

#include "../../util/StringSlice.h"

enum class Effect { Get, Set, Io, Own };
namespace effect {
	inline Effect min(Effect a, Effect b) { return a < b ? a : b; }
	StringSlice name(Effect e);
}
