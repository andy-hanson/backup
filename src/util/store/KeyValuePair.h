#pragma once

template<typename K, typename V>
struct KeyValuePair {
	K key;
	V value;

	KeyValuePair(const KeyValuePair& other) = delete;
	KeyValuePair& operator=(const KeyValuePair& other) = default;
};
