#pragma once

#include "../../util/StringSlice.h"

// https://youtrack.jetbrains.com/issue/CPP-7797
enum class Effect { EGet, ESet, EIo, EOwn };
namespace effect {
	inline Effect min(Effect a, Effect b) { return a < b ? a : b; }
	StringSlice name(Effect e);
}
