#pragma once

#include <cassert>
#include <limits>

inline size_t to_unsigned(std::ptrdiff_t s) {
	assert(s >= 0);
	return size_t(s);
}

inline uint to_uint(size_t u) {
	assert(u < std::numeric_limits<uint>::max());
	return uint(u);
}
