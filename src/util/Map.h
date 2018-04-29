#pragma once

#include <unordered_map>
#include <unordered_set>

#include "./Option.h"

// Returns a pointer to the value, and a bool telling whether this was newly added.
template <typename V>
struct InsertResult { ref<const V> value; bool was_added; };

template <typename T, typename Hash>
class Set {
	using Inner = typename std::unordered_set<T, Hash>;
	Inner inner;

public:
	using iterator = typename Inner::iterator;
	using const_iterator = typename Inner::const_iterator;

	InsertResult<T> insert(T value) {
		typename std::pair<iterator, bool> inserted = inner.insert(value);
		return { &*inserted.first, inserted.second };
	}

	bool has(const T& value) const {
		return bool(inner.count(value));
	}

	const T& only() const {
		assert(size() == 1);
		return *begin();
	}

	// Get the object in the set equivalent to the argument.
	Option<ref<const T>> get_in_set(const T& value) const {
		const_iterator found = inner.find(value);
		return found == inner.end() ? Option<ref<const T>>{} : Option<ref<const T>>{&*found};
	}

	ref<const T> get_in_set_or_insert(T&& value) {
		Option<ref<const T>> already = get_in_set(value);
		if (already.has())
			return already.get();
		else
			return must_insert(std::forward<T>(value));
	}

	size_t size() const {
		return inner.size();
	}

	ref<const T> must_insert(T value) {
		std::pair<iterator, bool> inserted = inner.insert(std::forward<T>(value));
		assert(inserted.second);
		return &*inserted.first;
	}

	const_iterator begin() const { return inner.begin(); }
	const_iterator end() const { return inner.end(); }
};

template <typename K, typename V, typename Hash>
class Map {
	using Inner = typename std::unordered_map<K, V, Hash>;
	Inner inner;

public:
	using const_iterator = typename Inner::const_iterator;

	void must_insert(K key, V value) {
		bool inserted = try_insert(key, value);
		assert(inserted);
	}

	bool try_insert(K key, V value) {
		return inner.insert({ key, value }).second;
	}

	inline V& get_or_create(K key) {
		return inner[key];
	}

	bool has(K key) const {
		return bool(inner.count(key));
	}

	Option<const V&> get(K key) const {
		return has(key) ? Option<const V&>{inner.at(key)} : Option<const V&> {};
	}

	inline void clear() {
		inner.clear();
	}

	inline const V& must_get(K key) const {
		return inner.at(key);
	}

	struct Values {
		struct iterator {
			const_iterator inner;
			inline void operator++() { ++inner; }
			inline const V& operator*() { return inner->second; }
			inline bool operator==(const iterator& other) const { return inner == other.inner; }
			inline bool operator!=(const iterator& other) const { return inner != other.inner; }
		};

		const Map<K, V, Hash>& map;
		inline iterator begin() const { return iterator { map.inner.begin() }; }
		inline iterator end() const { return iterator { map.inner.end() }; }
	};
	inline Values values() const { return { *this }; }

	struct Keys {
		struct iterator {
			const_iterator inner;
			inline void operator++() { ++inner; }
			inline const K& operator*() { return inner->first; }
			inline bool operator==(const iterator& other) const { return inner == other.inner; }
			inline bool operator!=(const iterator& other) const { return inner != other.inner; }
		};

		const Map<K, V, Hash>& map;
		inline iterator begin() const { return iterator { map.inner.begin() }; }
		inline iterator end() const { return iterator { map.inner.end() }; }
	};
	inline Keys keys() const { return { *this }; }

	const_iterator begin() const {
		return inner.begin();
	}

	const_iterator end() const {
		return inner.end();
	}
};

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

	void clear() { inner.clear(); }
};
