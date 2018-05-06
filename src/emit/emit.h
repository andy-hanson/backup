#pragma once

#include "../util/Writer.h"
#include "../compile/model/model.h"

Writer::Output emit(const Slice<Module>& modules, Arena& out);
