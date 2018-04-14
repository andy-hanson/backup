#pragma once

#include "Option.h"

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
	MaxSizeVector(MaxSizeVector&& other) {
		_size = other._size;
		for (uint i = 0; i < _size; ++i) {
			data.values[i] = std::move(other.data.values[i]);
		}
	}

	size_t size() const { return _size; }

	template <typename... Arguments>
	ref<T> emplace(Arguments&&... arguments) {
		assert(_size != capacity);
		T* ref = new (data.values + _size) T(std::forward<Arguments>(arguments)...);
		++_size;
		return ref;
	}

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

//TODO: reduce duplicate code
template <uint size, typename T, typename Pred>
Option<const T&> find(const MaxSizeVector<size, T>& collection, Pred pred) {
	for (const T& t : collection)
		if (pred(t))
			return { t };
	return {};
}
