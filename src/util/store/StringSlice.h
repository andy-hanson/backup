#pragma once

#include "../assert.h"
#include "../int.h"

class StringSlice {
private:
	const char* _begin;
	const char* _end; // Note: `*end` is NOT guaranteed to be '\0'

public:
	StringSlice() : _begin(nullptr), _end(nullptr) {}

	template <uint N>
	// Note: the char array will include a '\0', but we don't want that included in the slice.
	constexpr StringSlice(char const (&c)[N]) : _begin{c}, _end{c + N - 1} {
		static_assert(N > 0);
	}

	StringSlice(const char* begin, const char* end);

	using const_iterator = const char*;
	inline const_iterator begin() const { return _begin; }
	inline const_iterator end() const { return _end; }

	inline constexpr uint size() const {
		return to_unsigned(_end - _begin);
	}

	inline bool is_empty() const {
		return _end == _begin;
	}

	struct hash {
		hash_t operator()(const StringSlice& slice) const;
	};
};

bool operator==(const StringSlice& a, const StringSlice& b);
inline bool operator!=(const StringSlice& a, const StringSlice& b) { return !(a == b); }
