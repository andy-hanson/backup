#pragma once

#include "Arena.h" // Arr

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x#2595226
inline size_t hash_combine(size_t a, size_t b) {
	return b + 0x9e3779b9 + (a << 6) + (a >> 2);
}

template <typename T, typename Hash>
size_t hash_arr(const Arr<T>& d, Hash hash) {
	size_t h = 0;
	for (const T& t : d)
		h = hash_combine(h, hash(t));
	return h;
}
