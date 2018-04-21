#pragma once

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
	MaxSizeVector(const MaxSizeVector& other) = delete;
	~MaxSizeVector() {
		for (uint i = 0; i < _size; ++i)
			data.values[i].~T();
	}
	void operator=(const MaxSizeVector& other) = delete;

	size_t size() const { return _size; }

	bool empty() const { return _size == 0; }

	void push(T value) {
		assert(_size != capacity);
		data.values[_size] = value;
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

	__attribute__((unused)) // https://youtrack.jetbrains.com/issue/CPP-2151
	const T* begin() const {
		return &data.values[0];
	}
	__attribute__((unused)) // https://youtrack.jetbrains.com/issue/CPP-2151
	const T* end() const {
		return begin() + _size;
	}
};
