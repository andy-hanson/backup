#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "./Option.h"

// Returns a pointer to the value, and a bool telling whether this was newly added.
template <typename V>
struct Added { ref<const V> value; bool was_added; };

template <typename T>
class Sett { // TODO: clion has trouble if this is named Set
	std::unordered_set<T> inner;

public:
	Added<T> insert(T value) {
		typename std::pair<typename std::unordered_set<T>::iterator, bool> inserted = inner.insert(value);
		return { &*inserted.first, inserted.second };
	}

	size_t size() const {
		return inner.size();
	}

	void must_insert(T value) {
		assert(!inner.count(value));
		inner.insert(value);
	}

	typename std::unordered_set<T>::const_iterator begin() const { return inner.begin(); }
	typename std::unordered_set<T>::const_iterator end() const { return inner.end(); }
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

	Option<const V&> get(K key) const {
		try {
			return inner.at(key);
		} catch (std::out_of_range) {
			return {};
		}
	}

	void clear() {
		inner.clear();
	}

	const V& must_get(K key) const {
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

	typename std::unordered_map<K, V>::const_iterator begin() const {
		return inner.begin();
	}

	typename std::unordered_map<K, V>::const_iterator end() const {
		return inner.end();
	}
};

template <typename K, typename V>
class MultiMap {
	//unordered_multimap doesn't seem to have an easy way to iterate over groups.
	//Note: we
	std::unordered_multimap<K, V> inner;

public:
	// According to http://en.cppreference.com/w/cpp/container/unordered_map:
	// References and pointers to either key or data stored in the container are only invalidated by erasing that element, even when the corresponding iterator is invalidated.
	// So safe to return a pointer to the value.
	V& add(K key, V value) {
		typename std::unordered_multimap<K, V>::iterator i = inner.insert({ key, value });
		return i->second;

		/*typename std::unordered_map<K, std::vector<V>>::iterator found = inner.find(key);
		if (found == inner.end()) {
			inner.insert({ key, { value } });
		} else {
			found->second.push_back(value);
		}*/
		//inner.insert({ key, value });
	}

	size_t count(K key) const {
		return inner.count(key);
	}

	typename std::unordered_multimap<K, V>::const_iterator begin() const {
		return inner.begin();
	}
	typename std::unordered_multimap<K, V>::const_iterator end() const {
		return inner.end();
	}
};

