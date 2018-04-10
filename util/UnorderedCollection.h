#pragma once

#include <vector>

template <typename T>
class UnorderedCollection {
	std::vector<T> inner;

public:
	size_t size() const {
		return inner.size();
	}
	bool empty() const {
		return inner.empty();
	}
	T only() const {
		assert(inner.size() == 1);
		return inner[0];
	}

	template <typename... Arguments>
	void add(Arguments&&... arguments) {
		inner.emplace_back(std::forward<Arguments>(arguments)...);
	}

	template <typename Pred>
	void filter(Pred pred) {
		for (uint i = 0; i != inner.size(); ) {
			if (pred(inner[i])) {
				++i;
			} else {
				inner[i] = inner[inner.size() - 1];
				inner.pop_back();
			}
		}
	}

	typename std::vector<T>::const_iterator begin() const __attribute__((unused)) { // https://youtrack.jetbrains.com/issue/CPP-2151
		return inner.cbegin();
	}

	typename std::vector<T>::const_iterator end() const __attribute__((unused)) { // https://youtrack.jetbrains.com/issue/CPP-2151
		return inner.cend();
	}
};
