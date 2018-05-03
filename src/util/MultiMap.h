#pragma once

#include "./Alloc.h"
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
	Arr<Option<MultiMapPair<K, V>>> arr;

	MultiMap(Arr<Option<MultiMapPair<K, V>>> _arr) : arr(_arr) {}

public:
	MultiMap() : arr() {}

	bool has(const K& key) const {
		size_t hash = Hash{}(key);
		const Option<MultiMapPair<K, V>>& op_entry = arr[hash % arr.size()];
		return op_entry.has() && op_entry.get().key.has() && op_entry.get().key.get() == key;
	}

	template <typename /*const V& => void*/ Cb>
	void each_with_key(const K& key, Cb cb) const {
		assert(!arr.empty());

		size_t hash = Hash{}(key);
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
	MultiMap<K, ref<const V>, Hash> operator()(const Arr<V>& values, CbGetKey get_key) {
		if (values.empty())
			return {};

		size_t arr_size = values.size() * 2;
		Arr<Option<MultiMapPair<K, ref<const V>>>> arr = arena.fill_array<Option<MultiMapPair<K, ref<const V>>>>()(
			arr_size, [](uint i __attribute__((unused))) { return Option<MultiMapPair<K, ref<const V>>> {}; });

		for (const V& value : values) {
			const K& key = get_key(value);
			size_t hash = Hash{}(key);
			Option<MultiMapPair<K, ref<const V>>>& op_entry = arr[hash % arr.size()];
			if (op_entry.has()) {
				const MultiMapPair<K, ref<const V>>& entry = op_entry.get();
				if (entry.key.has() && entry.key.get() == key) {
					//Add this to the end of the group
					throw "tod!o";
				} else {
					throw "todo"; // false conflict, must re-hash
				}
			} else {
				op_entry = MultiMapPair<K, ref<const V>> { Option<K> { key }, ref<const V>(&value) };
			}
		}

		return { arr };
	}
};

template <typename K, typename V, typename Hash>
BuildMultiMap<K, V, Hash> build_multi_map(Arena& arena) {
	return { arena };
};
