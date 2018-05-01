#pragma once

#include "../util/StringSlice.h"

enum TestMode { Test, Accept };

void test(const StringSlice& test_dir, const StringSlice& test_name, TestMode mode);
