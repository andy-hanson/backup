#pragma once

#include <vector>

template <typename T>
class Vec {
	std::vector<T> inner;

public:
	void push(T value) { inner.push_back(value); }
	bool empty() { return inner.empty(); }
	T pop() {
		T res = inner.back();
		inner.pop_back();
		return res;
	}

	using const_iterator = typename std::vector<T>::const_iterator;
	using iterator = typename std::vector<T>::iterator;

	const_iterator begin() const { return inner.begin(); }
	const_iterator end() const { return inner.end(); }

	iterator begin() { return inner.begin(); }
	iterator end() { return inner.end(); }
};
