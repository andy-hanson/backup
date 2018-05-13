#pragma once

#include "../compile/model/model.h"
#include "ConcreteFun.h"

// Returns an InstStrict with is_deeply_concrete true.
InstStruct substitute_type_arguments(const StoredType& type, const ConcreteFun& fun, Arena& arena);
