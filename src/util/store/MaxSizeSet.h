#pragma once

#include "./MaxSizeMap.h"

template <uint capacity, typename T, typename Hash>
class MaxSizeSet {
	struct Dummy {};
	MaxSizeMap<capacity, T, Dummy, Hash> inner;

public:
	bool has(const T& value) const {
		return inner.has(value);
	}

	Ref<const T> must_insert(T value) {
		return &inner.must_insert(value, {})->key;
	}

	// Get the object in the set equivalent to the argument.
	Option<const T&> get_in_set(const T& value) const {
		return inner.get_key_in_map(value);
	}

	Ref<const T> get_in_set_or_insert(T&& value) {
		Option<const T&> already = get_in_set(value);
		if (already.has())
			return Ref<const T> { &already.get() };
		else
			return must_insert(value);
	}

	InsertResult try_insert(const T& value) {
		return inner.try_insert(value, {});
	}
};
