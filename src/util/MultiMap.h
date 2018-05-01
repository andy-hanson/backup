#pragma once

#include <unordered_map>

template <typename K, typename V, typename Hash>
class MultiMap {
	using Inner = typename std::unordered_multimap<K, V, Hash>;
	Inner inner;

public:
	using iterator = typename Inner::iterator;
	using const_iterator = typename Inner::const_iterator;

	// According to http://en.cppreference.com/w/cpp/container/unordered_map:
	// References and pointers to either key or data stored in the container are only invalidated by erasing that element, even when the corresponding iterator is invalidated.
	// So safe to return a pointer to the value.
	V& add(K key, V value) {
		return inner.insert({ key, value })->second;
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
};
