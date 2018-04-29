#include "./StringSlice.h"

#include <algorithm>

bool operator==(StringSlice a, StringSlice b) {
	assert(!a.empty() && !b.empty()); // Empty StringSlice uses nullptr, and std::equal segfaults on that
	return std::equal(a._begin, a._end, b._begin, b._end);
}
