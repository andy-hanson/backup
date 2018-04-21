#include "./int.h"

#include <cassert>
#include <limits>

size_t to_unsigned(std::ptrdiff_t s) {
	assert(s >= 0);
	return size_t(s);
}

uint to_uint(size_t u) {
	assert(u < std::numeric_limits<uint>::max());
	return uint(u);
}

ushort to_ushort(size_t u) {
	assert(u < std::numeric_limits<ushort>::max());
	return ushort(u);
}
