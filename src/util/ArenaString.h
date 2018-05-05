#pragma once

#include "./Alloc.h"
#include "./StringSlice.h"

// Wrapper around StringSlice that ensures it's in an arena.
class ArenaString {
	friend class StringBuilder;
	friend ArenaString allocate_slice(Arena& arena, size_t size);
	friend ArenaString str(Arena& arena, const StringSlice& slice);
	char* _begin;
	char* _end;
	// Keep this private to avoid using a non-arena slice.
	ArenaString(char* begin, char* end) : _begin(begin), _end(end) {}
public:
	ArenaString(const ArenaString& other) = default;
	ArenaString& operator=(const ArenaString& other) = default;
	ArenaString() : _begin(nullptr), _end(nullptr) {}
	char* begin() { return _begin; }
	char* end() { return _end; }
	inline operator StringSlice() const { return slice(); }
	inline StringSlice slice() const { return { _begin, _end }; }
	inline char& operator[](size_t index) {
		assert(index < slice().size());
		return *(_begin + index);
	}

	struct hash {
		inline size_t operator()(const ArenaString& a) const { return StringSlice::hash{}(a); }
	};
};
inline bool operator==(const ArenaString& a, const ArenaString& b) { return a.slice() == b.slice(); }
inline bool operator==(const ArenaString& a, const StringSlice& b) { return a.slice() == b; }

inline ArenaString allocate_slice(Arena& arena, size_t size) {
	char* begin = static_cast<char*>(arena.allocate(size));
	return { begin, begin + size };
}

class StringBuilder {
	Arena& arena;
	ArenaString slice;
	char* ptr;

	friend class Arena;

public:
	StringBuilder(Arena& _arena, size_t max_size) : arena(_arena), slice(allocate_slice(arena, max_size)), ptr(slice._begin) {}

	inline StringBuilder& operator<<(char c) {
		assert(ptr != slice._end);
		*ptr = c;
		++ptr;
		return *this;
	}

	StringBuilder& operator<<(uint u);

	StringBuilder& operator<<(StringSlice s);

	inline bool empty() const { return ptr == slice._begin; }

	inline char back() const {
		assert(ptr != slice._begin);
		return *(ptr - 1);
	}

	inline void pop() {
		assert(ptr != slice._begin);
		--ptr;
	}

	ArenaString finish();
};

ArenaString str(Arena& arena, const StringSlice& slice);
