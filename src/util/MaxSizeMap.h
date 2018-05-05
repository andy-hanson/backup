#pragma once

#include "./Option.h"
#include "./KeyValuePair.h"

struct InsertResult { bool was_added; };

template <uint capacity, typename K, typename V, typename Hash>
class MaxSizeMap {
	static_assert(capacity > 16, "Use SmallMap");

	Option<KeyValuePair<K, V>> arr[capacity];

	Option<KeyValuePair<K, V>>& get_raw_entry(const K& key) {
		return arr[Hash{}(key) % capacity];
	}
	const Option<KeyValuePair<K, V>>& get_raw_entry(const K& key) const {
		return arr[Hash{}(key) % capacity];
	}

	Option<KeyValuePair<K, V>&> get_entry(const K& key) {
		Option<KeyValuePair<K, V>>& op_entry = get_raw_entry(key);
		if (op_entry.has()) {
			KeyValuePair<K, V>& entry = op_entry.get();
			if (entry.key != key) throw "todo"; // false conflict
			return Option<KeyValuePair<K, V>&> { entry };
		} else
			return {};
	}
	Option<const KeyValuePair<K, V>&> get_entry(const K& key) const {
		const Option<KeyValuePair<K, V>>& op_entry = get_raw_entry(key);
		if (op_entry.has()) {
			const KeyValuePair<K, V>& entry = op_entry.get();
			if (entry.key != key) throw "todo"; // false conflict
			return Option<const KeyValuePair<K, V>&> { entry };
		} else
			return {};
	}

public:
	bool has(const K& key) const {
		return get_entry(key).has();
	}

	InsertResult try_insert(const K& key, const V& value) {
		Option<KeyValuePair<K, V>>& op_entry = get_raw_entry(key);
		if (op_entry.has()) {
			if (op_entry.get().key != key) throw "todo"; // false conflict
			return { false };
		} else {
			op_entry = KeyValuePair<K, V> { key, value };
			return { true };
		}
	}

	V& get_or_insert_default(const K& key) {
		Option<KeyValuePair<K, V>>& op_entry = get_raw_entry(key);
		if (op_entry.has()) {
			KeyValuePair<K, V>& entry = op_entry.get();
			if (entry.key != key) throw "todo"; // false conflict
			return entry.value;
		} else {
			op_entry = KeyValuePair<K, V> { key, {} };
			return op_entry.get().value;
		}
	}

	ref<KeyValuePair<K, V>> must_insert(K key, V value) {
		Option<KeyValuePair<K, V>>& op_entry = get_raw_entry(key);
		if (op_entry.has()) {
			KeyValuePair<K, V>& entry = op_entry.get();
			if (entry.key == key) throw "todo"; // false conflict
			assert(false); // true conflict
		} else {
			op_entry = KeyValuePair<K, V> { key, value };
			return &op_entry.get();
		}
	}

	V& must_get(const K& key) {
		return get_entry(key).get().value;
	}
	const V& must_get(const K& key) const {
		return get_entry(key).get().value;
	}

	Option<const K&> get_key_in_map(const K& key) const {
		Option<const KeyValuePair<K, V>&> op_entry = get_entry(key);
		return op_entry.has() ? Option<const K&> { op_entry.get().key } : Option<const K&>{};
	}

	Option<const V&> get(const K& key) const {
		Option<const KeyValuePair<K, V>&> op_entry = get_entry(key);
		return op_entry.has() ? Option<const V&> { op_entry.get().value } : Option<const V&> {};
	}
};

template <uint capacity, typename T, typename Hash>
class MaxSizeSet {
	struct Dummy {};
	MaxSizeMap<capacity, T, Dummy, Hash> inner;

public:
	bool has(const T& value) const {
		return inner.has(value);
	}

	ref<const T> must_insert(T value) {
		return &inner.must_insert(value, {})->key;
	}

	// Get the object in the set equivalent to the argument.
	Option<const T&> get_in_set(const T& value) const {
		return inner.get_key_in_map(value);
	}

	ref<const T> get_in_set_or_insert(T&& value) {
		Option<const T&> already = get_in_set(value);
		if (already.has())
			return ref<const T> { &already.get() };
		else
			return must_insert(value);
	}

	InsertResult try_insert(const T& value) {
		return inner.try_insert(value, {});
	}
};
