#pragma once

#include "../util/store/ListBuilder.h"
#include "../util/store/StringSlice.h"
#include "../util/PathCache.h"
#include "./TestMode.h"
#include "./TestFailure.h"

void test_single(const StringSlice& root, TestMode mode, PathCache& paths, ListBuilder<TestFailure>& failures, Arena& failures_arena);
