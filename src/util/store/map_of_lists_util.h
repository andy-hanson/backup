#pragma once

#include "./collection_util.h" // find
#include "./Map.h"
#include "./NonEmptyList.h"

//TODO:MOVE
template <typename T>
struct TryInsertResult {
	Ref<const T> value;
	bool was_inserted; // true if we just added it in the map, false if got a cached result.
};

template<typename K, typename V, typename KH, typename /*const V& => bool*/ IsMatch, /*() => V*/ typename CreateValue>
TryInsertResult<V> add_to_map_of_lists(Map<K, NonEmptyList<V>, KH>& map, K key, Arena& arena, IsMatch is_match, CreateValue create_value) {
	//TODO:PERF avoid repeated map lookup
	if (!map.has(key)) {
		return TryInsertResult<V> { &map.must_insert(key, NonEmptyList { create_value() }).value.first(), true };
	} else {
		NonEmptyList<V>& list = map.must_get(key);
		Option<Ref<const V>> found = find(list, is_match);
		return found.has()
			? TryInsertResult<V> { found.get(), false }
			: TryInsertResult<V> { &list.add(create_value(), arena), true };
	}
}
