#pragma once

#include "../Option.h"
#include "./Arena.h"
#include "./ArenaArrayBuilders.h"
#include "./List.h"

template <typename K, typename V, typename Hash>
class MultiMap {
	template <typename, typename, typename> friend struct build_multi_map;
	Map<K, List<V>, Hash> inner;

	inline explicit MultiMap(Map<K, List<V>, Hash> _inner) : inner{_inner} {}

public:
	MultiMap() {}

	bool has(const K& key) const {
		return inner.has(key);
	}

	template <typename /*const V& => void*/ Cb>
	void each_with_key(const K& key, Cb cb) const {
		Option<const List<V>&> list = inner.get(key);
		if (!list.has()) return;
		for (const V& v : list.get())
			cb(v);
	}
};

template <typename K, typename V, typename Hash>
struct build_multi_map {
	template <typename Input, typename /*const Input& => K*/ CbGetKey, typename /*const Input& => V*/ CbGetValue>
	MultiMap<K, V, Hash> operator()(Arena& arena, const Slice<Input>& inputs, CbGetKey get_key, CbGetValue get_value) {
		Map<K, List<V>, Hash> inner = build_map<K, List<V>, Hash>()(
			arena,
			inputs,
			get_key,
			/*get_value*/ [&](const Input& input) {
				return List<V> { get_value(input), arena };
			},
			/*on_conflict*/ [&](List<V>& list, const Input& input) {
				list.prepend(get_value(input), arena);
			}
		);
		return MultiMap { inner };
	}
};
