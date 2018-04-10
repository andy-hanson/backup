#pragma once

#include <vector>
#include <unordered_map>

#include "../util/ptr.h"
#include "../util/StringSlice.h"

template <typename T>
class Table {
	std::unordered_map<StringSlice, T> map;

public:
	Table() = default;
	Table(const Table<T>& other) = delete;

	Option<const T&> get(StringSlice slice) const {
		auto it = map.find(slice);
		return it == map.end() ? Option<const T&> {} : Option<const T&> { it->second };
	}

	Option<T&> get(StringSlice slice) {
		auto it = map.find(slice);
		return it == map.end() ? Option<T&> {} : Option<T&> { it->second };
	}

	T& must_get(StringSlice slice) {
		return get(slice).get();
	}

	T& get_or_create(StringSlice slice) {
		return map[slice];
	}

	// Creates a new element constructed with T { std::string(slice) }
	// Returns false if unable to insert.
	bool insert(StringSlice slice, T value) {
		return map.insert({ slice, value }).second;
	}
};

// Group of functions with the same name.
struct OverloadGroup {
	std::vector<ref<const Fun>> funs;
};
using StructsTable = Table<ref<const StructDeclaration>>;
using FunsTable = Table<OverloadGroup>;
