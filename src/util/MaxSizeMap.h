#pragma once

#include "./Option.h"
#include "./KeyValuePair.h"

struct InsertResult { bool was_added; };

template <uint capacity, typename K, typename V, typename Hash>
class MaxSizeMap {
	Option<KeyValuePair<K, V>> arr[capacity];

	Option<const KeyValuePair<K, V>&> get_entry(const K& key) const {
		size_t hash = Hash{}(key);
		const Option<KeyValuePair<K, V>> op_entry = arr[hash % capacity];
		return op_entry.has() && op_entry.get().key == key ? Option<const KeyValuePair<K, V>&> { op_entry.get() } : Option<const KeyValuePair<K, V>&>{};
	}

public:
	bool has(const K& key) {
		return get_entry(key).has();
	}

	InsertResult insert(const K& key, const V& value) {
		size_t hash = Hash{}(key);
		Option<KeyValuePair<K, V>>& op_entry = arr[hash % capacity];
		if (op_entry.has()) {
			if (op_entry.get().key != key) throw "todo";
			return { false };
		} else {
			op_entry = KeyValuePair<K, V> { key, value };
			return { true };
		}

	}

	ref<KeyValuePair<K, V>> must_insert(K key, V value) {
		size_t hash = Hash{}(key);
		Option<KeyValuePair<K, V>>& op_entry = arr[hash % capacity];
		if (op_entry.has()) throw "todo";
		op_entry = KeyValuePair<K, V> { key, value };
		return &op_entry.get();
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
	bool has(const T& value) {
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

	InsertResult insert(const T& value) {
		return inner.insert(value, {});
	}
};
