#include "./StringSlice.h"

StringSlice::StringSlice(const char* begin, const char* end) : _begin(begin), _end(end) {
	assert(end > begin);
	assert(begin != nullptr && end != nullptr);
	assert(size() < 1000); // Indicates memory was corrupted, since identifiers shouldn't get this big.
}

bool operator==(const StringSlice& a, const StringSlice& b) {
	if (a.size() != b.size()) return false;
	const char* ca = a.begin();
	const char* cb = b.begin();
	while (ca != a.end()) {
		if (*ca != *cb) return false;
		++ca; ++cb;
	}
	assert(cb == b.end());
	return true;
}

hash_t StringSlice::hash::operator()(const StringSlice& slice) const {
	// https://stackoverflow.com/questions/98153/whats-the-best-hashing-algorithm-to-use-on-a-stl-string-when-using-hash-map
	hash_t h = 0;
	for (char c : slice)
		h = 101 * h + hash_t(c);
	return h;
}
