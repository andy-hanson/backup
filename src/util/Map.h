#pragma once

#include "Arena.h"
#include "./ArenaArrayBuilders.h"
#include "./Option.h"
#include "./KeyValuePair.h"

template <typename K, typename V, typename Hash>
class Map {
	template <typename, typename, typename> friend struct BuildMap;
	Arr<Option<KeyValuePair<K, V>>> arr;
	Map(Arr<Option<KeyValuePair<K, V>>> _arr) : arr(_arr) {}

public:
	Map() : arr() {}

	Option<const V&> get(const K& key) const {
		assert(!arr.empty());

		hash_t hash = Hash{}(key);
		const Option<KeyValuePair<K, V>>& op_entry = arr[hash % arr.size()];
		if (op_entry.has()) {
			const KeyValuePair<K, V>& entry = op_entry.get();
			if (entry.key == key) return Option<const V&> { entry.value };
		}
		return Option<const V&> {};
	}
};

template <typename K, typename V, typename Hash>
struct BuildMap {
	Arena& arena;

	template <typename /*const V& => K*/ CbGetKey, typename /*(const V&, const V&) => void*/ CbConflict>
	Map<K, ref<const V>, Hash> operator()(const Arr<V>& values, CbGetKey get_key, CbConflict on_conflict) {
		if (values.empty())
			return {};

		Arr<Option<KeyValuePair<K, ref<const V>>>> arr = fill_array<Option<KeyValuePair<K, ref<const V>>>>()(
			arena, values.size() * 2, [](uint i __attribute__((unused))) { return Option<KeyValuePair<K, ref<const V>>> {}; });

		for (const V& value : values) {
			const K& key = get_key(value);
			hash_t hash = Hash{}(key);
			Option<KeyValuePair<K, ref<const V>>>& op_entry = arr[hash % arr.size()];
			if (op_entry.has()) {
				const KeyValuePair<K, ref<const V>>& entry = op_entry.get();
				if (entry.key == key) {
					// True conflict
					on_conflict(entry.value, value);
				} else {
					throw "todo"; // false conflict, must re-hash
				}
			} else {
				op_entry = KeyValuePair<K, ref<const V>> { key, ref<const V>(&value) };
			}
		}

		return { arr };
	}
};

template <typename K, typename V, typename Hash>
BuildMap<K, V, Hash> build_map(Arena& arena) {
	return { arena };
}
