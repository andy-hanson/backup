#pragma once

#include "../util/store/StringSlice.h"
#include "./TestMode.h"

// Returns exit code
int test(const StringSlice& test_dir, TestMode mode);
