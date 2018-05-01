#pragma once

#include <cassert>
#include "./int.h"
#include "./ptr.h"

template <typename T>
class Arr {
	T* _begin;
	size_t _size;

	friend class Arena;
	template<typename> friend class Vec;

	Arr(T* _data, size_t _len) : _begin(_data), _size(_len) {}

public:
	Arr() : _begin(nullptr), _size(0) {}

	T& operator[](size_t index) {
		assert(index < _size);
		return _begin[index];
	}

	const T& operator[](size_t index) const {
		assert(index < _size);
		return _begin[index];
	}

	bool contains_ref(ref<const T> r) const {
		return begin() <= r.ptr() && r.ptr() < end();
	}

	using iterator = T*;
	using const_iterator = const T*;

	size_t size() const { return _size; }
	bool empty() const { return _size == 0; }
	iterator begin() { return _begin; }
	iterator end() { return _begin + _size; }
	const_iterator begin() const { return _begin; }
	const_iterator end() const { return _begin + _size; }
};
