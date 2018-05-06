#pragma once

#include "./assert.h"
#include "./int.h"

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

	inline uint size() const { return _size; }

	inline bool is_empty() const { return _size == 0; }

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

	inline const T& peek() const {
		assert(!is_empty());
		return (*this)[_size - 1];
	}

	inline T& operator[](uint i) {
		assert(i < _size);
		return data.values[i];
	}
	inline const T& operator[](uint i) const {
		assert(i < _size);
		return data.values[i];
	}

	inline void pop() {
		assert(_size != 0);
		--_size;
	}

	inline T pop_and_return() {
		T res = peek();
		pop();
		return res;
	}

	using const_iterator = const T*;
	inline const_iterator begin() const { return &data.values[0]; }
	inline const_iterator end() const { return begin() + _size; }
};
