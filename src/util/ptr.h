#pragma once

#include "./assert.h"
#include "./int.h" // size_t

//TODO:MOVE
inline constexpr uint floor_log2(uint size) {
	uint res = 0;
	uint power = 1;
	while (power * 2 <= size) {
		++res;
		power *= 2;
	}
	return res;
}

/** Type of non-null references. */
template <typename T>
class ref {
	T* _ptr;

public:
	inline ref(T* ptr) : _ptr(ptr) {
		assert(ptr != nullptr);
	}

	inline T* ptr() { return _ptr; }
	inline const T* ptr() const { return _ptr; }

	// Just as T& implicitly converts to const T&, ref<T> is a ref<const T>
	inline operator ref<const T>() const {
		return ref<const T>(_ptr);
	}
	// Since ref is non-null, might as well make coversion to const& implicit
	inline operator const T&() const {
		return *_ptr;
	}
	inline operator T&() {
		return *_ptr;
	}

	inline bool operator==(ref<T> other) const {
		return _ptr == other._ptr;
	}
	inline bool operator!=(ref<T> other) const {
		return _ptr != other._ptr;
	}

	inline T& operator*() { return *_ptr; }
	inline const T& operator*() const { return *_ptr; }
	inline T* operator->() { return _ptr; }
	inline const T* operator->() const { return _ptr; }

	struct hash {
		inline hash_t operator()(ref<T> r) const {
			// https://stackoverflow.com/questions/20953390/what-is-the-fastest-hash-function-for-pointers
			static const hash_t shift = floor_log2(1 + sizeof(T));
			return hash_t(r.ptr()) >> shift;
		}
	};
};
