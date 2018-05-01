#pragma once

#include <unordered_map>
#include "./Option.h"

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
