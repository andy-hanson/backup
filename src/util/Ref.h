#pragma once

#include "./assert.h"
#include "./int.h"

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
class Ref {
	T* _ptr;

public:
	inline Ref(T* ptr) : _ptr{ptr} {
		assert(ptr != nullptr);
	}

	inline T* ptr() { return _ptr; }
	inline const T* ptr() const { return _ptr; }

	// Just as T& implicitly converts to const T&, Ref<T> is a Ref<const T>
	inline operator Ref<const T>() const {
		return Ref<const T>(_ptr);
	}
	// Since ref is non-null, might as well make coversion to const& implicit
	inline operator const T&() const {
		return *_ptr;
	}
	inline operator T&() {
		return *_ptr;
	}

	inline bool operator==(Ref<T> other) const {
		return _ptr == other._ptr;
	}
	inline bool operator!=(Ref<T> other) const {
		return _ptr != other._ptr;
	}

	inline T& operator*() { return *_ptr; }
	inline const T& operator*() const { return *_ptr; }
	inline T* operator->() { return _ptr; }
	inline const T* operator->() const { return _ptr; }

	struct hash {
		inline hash_t operator()(Ref<T> r) const {
			// https://stackoverflow.com/questions/20953390/what-is-the-fastest-hash-function-for-pointers
			static const hash_t shift = floor_log2(1 + sizeof(T));
			return hash_t(r.ptr()) >> shift;
		}
	};
};
