#include "./Alloc.h"

#include <algorithm> // std::copy

void* Arena::allocate(size_t n_bytes) {
	void* res = alloc_next;
	alloc_next = static_cast<char*>(alloc_next) + n_bytes;
	assert(alloc_next <= alloc_end);
	return res;
}

Arena::Arena() {
	size_t size = 10000;
	alloc_begin = ::operator new(size);
	alloc_end = static_cast<char*>(alloc_begin) + size;
	alloc_next = alloc_begin;
}

Arena::~Arena() {
	::operator delete(alloc_begin);
}

Arena::StringBuilder& Arena::StringBuilder::operator<<(uint u) {
	assert(ptr != slice._end);
	//TODO: better
	if (u < 10) {
		*ptr = '0' + char(u);
		++ptr;
	} else if (u < 100) {
		assert(ptr + 1 != slice._end);
		*ptr = '0' + char(u % 10);
		*(ptr + 1) = '0' + char(u / 10);
		++ptr;
	} else
		throw "todo";
	return *this;
}

Arena::StringBuilder& Arena::StringBuilder::operator<<(StringSlice s) {
	for (char c : s)
		*this << c;
	return *this;
}

ArenaString Arena::StringBuilder::finish() {
	assert(arena.alloc_next == slice._end); // no intervening allocations
	arena.alloc_next = ptr; // Only used up this much space, don't waste the rest
	return { slice._begin, ptr };
}

Arena::StringBuilder Arena::string_builder(size_t max_size) {
	return StringBuilder(*this, allocate_slice(max_size));
}

ArenaString Arena::allocate_slice(size_t size) {
	char* begin = static_cast<char*>(allocate(size));
	return { begin, begin + size };
}

ArenaString Arena::str(const StringSlice& slice) {
	size_t size = slice.size();
	char* begin = static_cast<char*>(allocate(size));
	char* end = std::copy(slice.begin(), slice.end(), begin);
	assert(size_t(end - begin) == size);
	assert(alloc_next == end);
	return { begin, end };
}
