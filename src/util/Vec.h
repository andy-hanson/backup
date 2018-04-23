#pragma once

#include <vector>
#include "./int.h"

template <typename T>
class Vec {
	std::vector<T> inner;

	Vec(std::vector<T> _inner) : inner(_inner) {}

public:
	Vec() = default;
	Vec(const Vec<T>& other) = delete;
	Vec<T> clone() const { return { inner }; }
	Vec(Vec<T>&& other) = default;
	Vec<T>& operator=(Vec<T>&& other) = default;
	explicit Vec(T value) : inner{value} {}

	void push(T value) { inner.push_back(value); }
	bool empty() const { return inner.empty(); }
	T pop() {
		T res = inner.back();
		inner.pop_back();
		return res;
	}
	size_t size() const { return inner.size(); }

	void insert(uint index, T value) {
		assert(index <= size());
		inner.insert(inner.begin() + index, value);
	}

	const T& operator[](uint i) const {
		return inner[i];
	}

	using const_iterator = typename std::vector<T>::const_iterator;
	using iterator = typename std::vector<T>::iterator;

	const_iterator begin() const { return inner.begin(); }
	const_iterator end() const { return inner.end(); }

	iterator begin() { return inner.begin(); }
	iterator end() { return inner.end(); }
};
