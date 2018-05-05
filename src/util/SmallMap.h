#pragma once

#include "./int.h"
#include "./KeyValuePair.h"
#include "./MaxSizeVector.h"

template <uint capacity, typename K, typename V>
class SmallMap {
	MaxSizeVector<capacity, KeyValuePair<K, V>> pairs;

public:
	void push(K key, V value) {
		pairs.push({ key, value });
	}

	void pop() {
		pairs.pop();
	}

	V must_get(const K& key) {
		for (const KeyValuePair<K, V>& pair : pairs)
			if (pair.key == key)
				return pair.value;
		unreachable();
	}
};
