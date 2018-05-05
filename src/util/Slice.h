#pragma once

#include "./assert.h"
#include "./int.h"
#include "./Ref.h"

template <typename T>
class Slice {
	T* _begin;
	uint _size;

public:
	Slice() : _begin(nullptr), _size(0) {}
	Slice(T* _data, uint _len) : _begin(_data), _size(_len) {}

	Slice slice(uint lo, uint hi) {
		assert(lo < hi && hi < _size);
		return { _begin + lo, hi - lo };
	}

	T& operator[](uint index) {
		assert(index < _size);
		return _begin[index];
	}

	const T& operator[](uint index) const {
		assert(index < _size);
		return _begin[index];
	}

	bool contains_ref(Ref<const T> r) const {
		return begin() <= r.ptr() && r.ptr() < end();
	}

	using iterator = T*;
	using const_iterator = const T*;

	uint size() const { return _size; }
	bool empty() const { return _size == 0; }
	iterator begin() { return _begin; }
	iterator end() { return _begin + _size; }
	const_iterator begin() const { return _begin; }
	const_iterator end() const { return _begin + _size; }
};
