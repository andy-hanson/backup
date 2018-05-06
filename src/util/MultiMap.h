#pragma once

#include "Arena.h"
#include "./ArenaArrayBuilders.h"
#include "./Option.h"

//Key=None means this is a non-first entry
template<typename K, typename V>
struct MultiMapPair {
	Option<K> key;
	V value;
};
template <typename K, typename V, typename Hash>
class MultiMap {
	template <typename, typename, typename> friend struct BuildMultiMap;
	Slice<Option<MultiMapPair<K, V>>> arr;

	MultiMap(Slice<Option<MultiMapPair<K, V>>> _arr) : arr(_arr) {}

public:
	MultiMap() : arr() {}

	bool has(const K& key) const {
		hash_t hash = Hash{}(key);
		const Option<MultiMapPair<K, V>>& op_entry = arr[hash % arr.size()];
		return op_entry.has() && op_entry.get().key.has() && op_entry.get().key.get() == key;
	}

	template <typename /*const V& => void*/ Cb>
	void each_with_key(const K& key, Cb cb) const {
		assert(!arr.is_empty());

		hash_t hash = Hash{}(key);
		const Option<MultiMapPair<K, V>>* op_entry_ptr = &arr[hash % arr.size()];
		if (!op_entry_ptr->has()) return;

		{
			const MultiMapPair<K, V>& entry = op_entry_ptr->get();
			if (!entry.key.has()) return; // This is a continuation entry of something else.
			if (entry.key.get() != key) return;
			cb(entry.value);
		}

		while (true) {
			++op_entry_ptr;
			if (op_entry_ptr == arr.end()) return;
			if (!op_entry_ptr->has()) return;
			const MultiMapPair<K, V>& entry = op_entry_ptr->get();
			if (entry.key.has()) return; // This is a different group.
			cb(entry.value);
		}
	}
};

template <typename K, typename V, typename Hash>
struct BuildMultiMap {
	Arena& arena;

	template <typename /*const V& => K*/ CbGetKey>
	MultiMap<K, Ref<const V>, Hash> operator()(const Slice<V>& values, CbGetKey get_key) {
		if (values.is_empty())
			return {};

		uint arr_size = values.size() * 2;
		Slice<Option<MultiMapPair<K, Ref<const V>>>> arr = fill_array<Option<MultiMapPair<K, Ref<const V>>>>()(
			arena, arr_size, [](uint i __attribute__((unused))) { return Option<MultiMapPair<K, Ref<const V>>> {}; });

		for (const V& value : values) {
			const K& key = get_key(value);
			hash_t hash = Hash{}(key);
			Option<MultiMapPair<K, Ref<const V>>>& op_entry = arr[hash % arr.size()];
			if (op_entry.has()) {
				const MultiMapPair<K, Ref<const V>>& entry = op_entry.get();
				if (entry.key.has() && entry.key.get() == key) {
					//Add this to the end of the group
					throw "tod!o";
				} else {
					todo(); // false conflict, must re-hash
				}
			} else {
				op_entry = MultiMapPair<K, Ref<const V>> { Option<K> { key }, Ref<const V>(&value) };
			}
		}

		return { arr };
	}
};

template <typename K, typename V, typename Hash>
BuildMultiMap<K, V, Hash> build_multi_map(Arena& arena) {
	return { arena };
};
