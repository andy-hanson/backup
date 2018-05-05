#include "Arena.h"

#include "./assert.h"

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
