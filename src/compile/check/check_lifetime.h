#pragma once

#include "../model/model.h"

// TODO: compile error if attempting to combine 'new' with borrowed lifetime
Lifetime common_lifetime(const Slice<Lifetime>& cases, const Lifetime& elze);

void check_return_lifetime(const Lifetime& declared, const Lifetime& actual);
