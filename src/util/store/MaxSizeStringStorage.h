#pragma once

#include "../assert.h"
#include "../int.h"
#include "./StringSlice.h"

struct MaxSizeStringWriter {
	char* cur;
	char* end;

	inline uint capacity() {
		return to_unsigned(end - cur);
	}

	inline MaxSizeStringWriter& operator<<(char c) {
		assert(cur != end);
		*cur = c;
		++cur;
		return *this;
	}

	MaxSizeStringWriter& operator<<(const StringSlice& s);
};

template <uint capacity>
class MaxSizeStringStorage {
	char values[capacity];

	inline char* end() { return values + capacity; }

public:
	inline char* begin() { return values; }

	inline MaxSizeStringWriter writer() {
		return { values, end() };
	}

	inline StringSlice finish(MaxSizeStringWriter& writer) {
		assert(writer.cur >= values && writer.cur < writer.end && writer.end == end());
		return { values, writer.cur };
	}
};
