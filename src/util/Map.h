#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "./Option.h"

template <typename T>
class Set {
	std::unordered_set<T> inner;

public:
	void must_insert(T value) {
		assert(!inner.count(value));
		inner.insert(value);
	}
};

template <typename K, typename V>
class Map {
	std::unordered_map<K, V> inner;

public:
	void must_insert(K key, V value) {
		bool inserted = try_insert(key, value);
		assert(inserted);
	}

	bool try_insert(K key, V value) {
		return inner.insert({ key, value }).second;
	}

	V& get_or_create(K key) {
		return inner[key];
	}

	Option<const V&> get_op(K key) const {
		try {
			return inner.at(key);
		} catch (std::out_of_range) {
			return {};
		}
	}

	void clear() {
		inner.clear();
	}

	const V& get(K key) const {
		return inner.at(key);
	}

	struct Values {
		struct iterator {
			typename std::unordered_map<K, V>::const_iterator inner;

			void operator++() {
				++inner;
			}

			const V& operator*() {
				return inner->second;
			}

			bool operator==(const iterator& other) const {
				return inner == other.inner;
			}

			bool operator!=(const iterator& other) const {
				return inner != other.inner;
			}
		};

		const Map<K, V>& map;
		iterator begin() const {
			return iterator { map.inner.begin() };
		}
		iterator end() const {
			return iterator { map.inner.end() };
 		}
	};

	Values values() const {
		return { *this };
	}
};

template <typename K, typename V>
class MultiMap {
	//unordered_multimap doesn't seem to have an easy way to iterate over groups.
	std::unordered_map<K, std::vector<V>> inner;

public:
	void add(K key, V value) {
		typename std::unordered_map<K, std::vector<V>>::iterator found = inner.find(key);
		if (found == inner.end()) {
			inner.insert({ key, { value } });
		} else {
			found->second.push_back(value);
		}
		//inner.insert({ key, value });
	}

	typename std::unordered_map<K, std::vector<V>>::const_iterator begin() const {
		return inner.begin();
	}
	typename std::unordered_map<K, std::vector<V>>::const_iterator end() const {
		return inner.end();
	}
};

