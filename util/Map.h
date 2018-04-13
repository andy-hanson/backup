#pragma once

#include <unordered_map>

template <typename K, typename V>
class Map {
	std::unordered_map<K, V> inner;

public:
	void must_insert(K key, V value) {
		assert(inner.count(key) == 0);
		inner.insert(std::make_pair(key, value));
	}

	Option<const V&> get_op(K key) const {
		try {
			return inner.at(key);
		} catch (std::out_of_range) {
			return {};
		}
	}

	const V& get(K key) const {
		return inner.at(key);
	}
};
