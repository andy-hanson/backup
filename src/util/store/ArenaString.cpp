#include "./ArenaString.h"

StringBuilder& StringBuilder::operator<<(uint u) {
	//TODO: duplicate code in Writer.cpp
	if (u < 10) {
		*this << char('0' + char(u));
	} else if (u < 100) {
		*this << char('0' + char(u / 10));
		*this << char('0' + char(u % 10));
	} else
		todo();
	return *this;
}

StringBuilder& StringBuilder::operator<<(StringSlice s) {
	for (char c : s)
		*this << c;
	return *this;
}

ArenaString StringBuilder::finish() {
	assert(arena.alloc_next == slice._end); // no intervening allocations
	arena.alloc_next = ptr; // Only used up this much space, don't waste the rest
	return { slice._begin, ptr };
}

ArenaString copy_string(Arena& arena, const StringSlice& slice) {
	uint size = slice.size();
	char* const begin = static_cast<char*>(arena.allocate(size));
	char* end = begin;
	for (char c : slice) {
		*end = c;
		++end;
	}
	assert(end - begin == size);
	//TODO: assert(arena.alloc_next == end);
	return { begin, end };
}
