#pragma once

#include "../util/StringSlice.h"
#include "../util/Path.h"
#include "../util/List.h"
#include "./TestMode.h"
#include "./TestFailure.h"

void test_single(const StringSlice& root, TestMode mode, PathCache& paths, List<TestFailure>::Builder& failures, Arena& failures_arena);
