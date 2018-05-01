#pragma once

#include <cassert>
#include "./int.h"

// TODO: MOVE
template <uint capacity, typename T>
class MaxSizeVector {
	uint _size;
	// Use a union to avoid initializing automatically
	union Data {
		char dummy __attribute__((unused));
		T values[capacity];

		Data() {} // uninitialized
		~Data() {} // done by ~MaxSizeVector()
	};
	Data data;

public:
	MaxSizeVector() : _size(0) {}
	MaxSizeVector(const MaxSizeVector& other) { *this = other; }
	~MaxSizeVector() {
		for (uint i = 0; i < _size; ++i)
			data.values[i].~T();
	}
	void operator=(const MaxSizeVector& other) {
		_size = other._size;
		for (uint i = 0; i < _size; ++i)
			data.values[i] = other.data.values[i];
	}

	MaxSizeVector(T first) : MaxSizeVector() {
		push(first);
	}

	size_t size() const { return _size; }

	bool empty() const { return _size == 0; }

	void push(T value) {
		assert(_size != capacity);
		data.values[_size] = value;
		++_size;
	}

	void insert(uint index, T value) {
		assert(_size != capacity);
		assert(index <= _size);
		for (uint i = _size; i != index; --i)
			data.values[i] = data.values[i - 1];
		data.values[index] = value;
		++_size;
	}

	const T& peek() const {
		return (*this)[_size - 1];
	}

	T& operator[](size_t i) {
		assert(i < _size);
		return data.values[i];
	}
	const T& operator[](size_t i) const {
		assert(i < _size);
		return data.values[i];
	}

	void pop() {
		assert(_size != 0);
		--_size;
	}

	T pop_and_return() {
		T res = peek();
		pop();
		return res;
	}

	using const_iterator = const T*;
	const_iterator begin() const { return &data.values[0]; }
	const_iterator end() const { return begin() + _size; }
};
