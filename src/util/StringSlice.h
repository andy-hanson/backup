#pragma once

#include <cassert>

#include "./int.h"

struct SourceRange {
	ushort begin;
	ushort end;
	inline SourceRange(ushort _begin, ushort _end) : begin(_begin), end(_end) {
		assert(end >= begin);
	}
};

class StringSlice {
private:
	const char* _begin;
	const char* _end; // Note: `*end` is NOT guaranteed to be '\0'

public:
	StringSlice() : _begin(nullptr), _end(nullptr) {}

	template <size_t N>
	// Note: the char array will include a '\0', but we don't want that included in the slice.
	constexpr StringSlice(char const (&c)[N]) : StringSlice(c, c + N - 1) {
		static_assert(N > 0);
	}

	constexpr StringSlice(const char* begin, const char* end) : _begin(begin), _end(end) {
		assert(end > begin);
		assert(begin != nullptr && end != nullptr);
		assert(size() < 1000); // Or else we probably screwed up
	}

	inline static StringSlice from_range(const StringSlice& slice, const SourceRange& range) {
		size_t len = slice.size();
		assert(range.end < len);
		return { slice.begin() + range.begin, slice.begin() + range.end };
	}

	inline const char* begin() const { return _begin; }
	inline const char* end() const { return _end; }

	inline const char* cstr() const {
		assert(!empty() && *(_end - 1) == '\0');
		return _begin;
	}

	inline StringSlice slice(uint begin) const {
		assert(begin < size());
		return { _begin + begin, _end };
	}

	inline bool empty() const { return _end == _begin; }

	inline constexpr size_t size() const {
		assert(_end > _begin);
		return size_t(_end - _begin);
	}

	inline char operator[](size_t index) const {
		const char* ptr = _begin + index;
		assert(ptr < _end);
		return *ptr;
	}

	inline SourceRange range_from_inner_slice(const StringSlice& inner) const {
		assert(_begin <= inner._begin && inner._end <= _end);
		return { to_ushort(to_unsigned(inner._begin - _begin)), to_ushort(to_unsigned(inner._end - _begin)) };
	}

	friend bool operator==(const StringSlice& a, const StringSlice& b);

	struct hash {
		size_t operator()(StringSlice slice) const {
			size_t h = 0;
			for (char c : slice)
				h = 31 * h + size_t(c + 128);
			return h;
		}
	};
};

inline bool operator!=(const StringSlice& a, const StringSlice& b) { return !(a == b); }
