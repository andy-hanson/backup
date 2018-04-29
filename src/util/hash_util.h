#pragma once

//TODO:MOVE
//https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x#2595226
//inline size_t hash_combine(size_t seed) { return seed; }

inline size_t hash_combine(size_t a, size_t b) {
	return b + 0x9e3779b9 + (a << 6) + (a >> 2);
}

/*
template <typename T, typename Hash, typename... Rest>
inline size_t hash_combine(size_t seed, Hash hasher, const T& v, Rest... rest) {
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	return hash_combine(seed, rest...);
}*/

template <typename T, typename Hash>
size_t hash_dyn_array(const Arr<T>& d, Hash hash) {
	size_t h = 0;
	for (const T& t : d)
		h = hash_combine(h, hash(t));
	return h;
}
