#pragma once

#include "../util/StringSlice.h"
#include "./TestMode.h"

// Returns exit code
int test(const StringSlice& test_dir, TestMode mode);
