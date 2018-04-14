#pragma once

#include <cassert>
#include <cstring>
#include <string>

#include "util.h"

class StringSlice {
private:
	const char* _begin;
	const char* _end;

public:
	StringSlice() {}

	template <size_t N>
	// Note: the char array will include a '\0', but we don't want that included in the slice.
	constexpr StringSlice(char const (&c)[N]) : StringSlice(c, c + N - 1) {
		static_assert(N > 0);
	}

	constexpr StringSlice(const char* begin, const char* end) : _begin(begin), _end(end) {}

	const char* begin() const { return _begin; }
	const char* end() const { return _end; }

	bool empty() const { return _end == _begin; }

	inline size_t size() const {
		return to_unsigned(_end - _begin);
	}

	char operator[](size_t index) const {
		const char* ptr = _begin + index;
		assert(ptr < _end);
		return *ptr;
	}

	friend bool operator==(StringSlice a, StringSlice b);
};

namespace std {
	template<>
	struct hash<StringSlice> {
		size_t operator()(StringSlice slice) const {
			// Just use the first 8 characters as the hash.
			static_assert(sizeof(size_t) == 8, "!");
			auto at = [slice](size_t i) {
				return i >= slice.size() ? 0 : uint64_t(slice[i]) << (8 * i);
			};
			return at(0) | at(1) | at(2) | at(3) | at(4) | at(5) | at(6) | at(7);
		}
	} __attribute__((unused)); // clion thinks this is unused;
}
