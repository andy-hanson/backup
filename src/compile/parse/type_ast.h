#pragma once

#include "../../util/Alloc.h"
#include "../../util/StringSlice.h"

struct TypeAst {
	bool is_type_parameter; // If true, type_arguments should be unused
	StringSlice type_name;
	Arr<TypeAst> type_arguments;
};
