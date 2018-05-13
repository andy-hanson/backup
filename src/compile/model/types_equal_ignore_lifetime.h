#pragma once

#include "../model/model.h"

bool types_equal_ignore_lifetime(const InstStruct& a, const InstStruct& b);
bool types_equal_ignore_lifetime(const StoredType& a, const StoredType& b);
