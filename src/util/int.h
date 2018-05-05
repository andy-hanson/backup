#pragma once

#include "./assert.h"

using hash_t = unsigned long;
using uint = unsigned int;
using ushort = unsigned short;
using ulong = unsigned long;

inline constexpr int to_signed(uint u) {
	assert(u < 1042); //todo: higher limit
	return int(u);
}
inline constexpr uint to_unsigned(long l) {
	assert(l >= 0);
	return uint(l);
}
inline constexpr uint to_unsigned(int i) {
	assert(i >= 0);
	return uint(i);
}
ushort to_ushort(uint u);
