#include "./StringSlice.h"

bool operator==(const StringSlice& a, const StringSlice& b) {
	assert(!a.empty() && !b.empty()); // Empty StringSlice uses nullptr, and std::equal segfaults on that
	const char* ca = a._begin;
	const char* cb = b._begin;
	while (ca != a._end) {
		if (*ca != *cb) return false;
		++ca; ++cb;
	}
	assert(cb == b._end);
	return true;
}
