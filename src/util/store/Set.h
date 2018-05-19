#pragma once

#include "./Map.h"

template <typename T, typename Hash>
class Set {
	struct Dummy {};
	Map<T, Dummy, Hash> map;

public:
	Set(uint initial_capacity, Arena& arena) : map{initial_capacity, arena} {}

	bool has(const T& value) const {
		return map.has(value);
	}

	const T& must_insert(T value) {
		return map.must_insert(value, {}).key;
	}

	InsertResult<T, Dummy> try_insert(T value) {
		return map.try_insert(value, {});
	}

	Option<const T&> get_in_set(const T& value) const {
		return map.get_key_in_map(value);
	}

	const T& get_in_set_or_insert(T&& value) {
		Option<const T&> already = get_in_set(value);
		if (already.has())
			return already.get();
		else
			return must_insert(value);
	}
};
