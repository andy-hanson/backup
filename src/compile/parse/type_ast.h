#pragma once

#include "../model/effect.h"
#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"

struct TypeAst {
	Effect effect;
	StringSlice type_name;
	DynArray<TypeAst> type_arguments;
};
