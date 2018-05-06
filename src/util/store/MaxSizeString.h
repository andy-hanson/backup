#pragma once

#include "../assert.h"
#include "../int.h"
#include "./StringSlice.h"

struct MaxSizeStringWriter {
	char* cur;
	char* end;

	inline uint remaining_capacity() {
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
class MaxSizeString {
	char data[capacity];
	uint length;

public:
	StringSlice slice() const { return { data, data + length }; }

	template <typename /*MaxSizeStringWriter& => void*/ Cb>
	static MaxSizeString make(Cb cb) {
		MaxSizeString s;
		char* end = s.data + capacity;
		MaxSizeStringWriter w { s.data, end };
		cb(w);
		assert(w.cur >= s.data && w.cur < w.end && w.end == end);
		s.length = to_unsigned(w.cur - s.data);
		return s;
	}
};
