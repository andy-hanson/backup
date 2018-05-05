#pragma once

#include "../compile/model/model.h"

BlockedList<char> emit(const Slice<Module>& modules, Arena& out);
