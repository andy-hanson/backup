#pragma once

#include <cassert>
#include <cstddef> // size_t
#include <functional> // hash

/** Type of non-null references. */
template <typename T>
class ref {
	T* _ptr;

public:
	ref(T* ptr) : _ptr(ptr) {
		assert(ptr != nullptr);
	}

	T* ptr() { return _ptr; }
	const T* ptr() const { return _ptr; }

	//ref<const T> as_const() const {
	//	return ref<const T>(_ptr);
	//}
	operator ref<const T>() const {
		return ref<const T>(_ptr);
	}
	// Since ref is non-null, might as well make coversion to const& implicit
	operator const T&() const {
		return *_ptr;
	}


	bool operator==(ref<T> other) const {
		return _ptr == other._ptr;
	}
	bool operator!=(ref<T> other) const {
		return _ptr != other._ptr;
	}

	T& operator*() {
		return *_ptr;
	}
	const T& operator*() const {
		return *_ptr;
	}

	T* operator->() {
		return _ptr;
	}
	const T* operator->() const {
		return _ptr;
	}
};

namespace std {
	template<typename T>
	struct hash<::ref<T>> {
		size_t operator()(::ref<T> r) const {
			// TODO: better hash
			return reinterpret_cast<size_t>(r.ptr());
		}
	} __attribute__((unused)); // clion thinks this is unused
}
