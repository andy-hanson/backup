#pragma once

#include "../compile/model/model.h"
#include "ConcreteFun.h"

InstStruct substitute_type_arguments(const Type& type_argument, const ConcreteFun& fun, Arena& arena);
