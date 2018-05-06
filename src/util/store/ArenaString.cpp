#include "./ArenaString.h"

namespace {
	static char encoding_table[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};
	char base64(uint u) {
		assert(u < 64);
		return encoding_table[u];
	}
}

void StringBuilder::write_base_64(uint u) {
	if (u < 64) {
		*this << base64(u);
	} else {
		assert(u < 64 * 64);
		*this << base64(u / 64) + base64(u % 64);
	}
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
