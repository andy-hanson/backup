#pragma once

#include "../util/store/StringSlice.h"
#include "./TestMode.h"

/*abstract*/ class TestFilter {
public:
	virtual bool should_test(const StringSlice& directory) const = 0;
	virtual ~TestFilter();
};
class EveryTestFilter final : public TestFilter {
	bool should_test(const StringSlice& directory) const override;
};

// Returns exit code
int test(const StringSlice& test_dir, const TestFilter& filter, TestMode mode);
