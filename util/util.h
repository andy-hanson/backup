#pragma once

inline size_t to_unsigned(std::ptrdiff_t s) {
	assert(s >= 0);
	return size_t(s);
}
