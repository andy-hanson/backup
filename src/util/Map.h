#pragma once

#include <unordered_map>
#include <unordered_set>

#include "./Option.h"

// Returns a pointer to the value, and a bool telling whether this was newly added.
template <typename V>
struct InsertResult { ref<const V> value; bool was_added; };

template <typename T>
class Sett { // TODO: clion has trouble if this is named Set
	std::unordered_set<T> inner;

public:
	InsertResult<T> insert(T value) {
		typename std::pair<typename std::unordered_set<T>::iterator, bool> inserted = inner.insert(value);
		return { &*inserted.first, inserted.second };
	}

	const T& only() const {
		assert(size() == 1);
		return *begin();
	}

	// Get the objecct in the set equivalent to the argument.
	const T& get(T value) const {
		typename std::unordered_set<T>::iterator found = inner.find(value);
		assert(found != inner.end());
		return *found;
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

	bool has(K key) const {
		return bool(inner.count(key));
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
			void operator++() { ++inner; }
			const V& operator*() { return inner->second; }
			bool operator==(const iterator& other) const { return inner == other.inner; }
			bool operator!=(const iterator& other) const { return inner != other.inner; }
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
	using iterator = typename std::unordered_multimap<K, V>::iterator;
	using const_iterator = typename std::unordered_multimap<K, V>::const_iterator;

	// According to http://en.cppreference.com/w/cpp/container/unordered_map:
	// References and pointers to either key or data stored in the container are only invalidated by erasing that element, even when the corresponding iterator is invalidated.
	// So safe to return a pointer to the value.
	V& add(K key, V value) {
		typename std::unordered_multimap<K, V>::iterator i = inner.insert({ key, value });
		return i->second;
	}

	size_t count(const K& key) const {
		return inner.count(key);
	}

	bool has(const K& key) const {
		return bool(count(key));
	}

	const_iterator begin() const {
		return inner.begin();
	}
	const_iterator end() const {
		return inner.end();
	}

	struct value_iterator {
		const_iterator inner;
		void operator++() { ++inner; }
		const V& operator*() { return inner->second; }
		bool operator==(const value_iterator& other) const { return inner == other.inner; }
		bool operator!=(const value_iterator& other) const { return inner != other.inner; }
	};

	struct AllWithKey {
		value_iterator _begin;
		value_iterator _end;
		value_iterator begin() { return _begin; }
		value_iterator end() { return _end; }
	};
	AllWithKey all_with_key(const K& key) const {
		typename std::pair<const_iterator, const_iterator> p = inner.equal_range(key);
		return AllWithKey { { p.first }, { p.second } };
	}

	void clear() { inner.clear(); }
};
