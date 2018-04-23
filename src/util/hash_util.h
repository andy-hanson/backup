#pragma once

//TODO:MOVE
//https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x#2595226
inline size_t hash_combine(size_t seed) { return seed; }
template <typename T, typename... Rest>
inline size_t hash_combine(size_t seed, const T& v, Rest... rest) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	return hash_combine(seed, rest...);
}

template <typename T>
size_t hash_dyn_array(const Arr<T>& d) {
	size_t h = 0;
	for (const T& t : d) {
		hash_combine(h, t);
	}
	return h;
}
