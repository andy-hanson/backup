#pragma once

#include <unordered_set>
#include "./Option.h"
#include "./ptr.h"

template <typename T, typename Hash>
class HeapAllocatedSet {
	using Inner = typename std::unordered_set<T, Hash>;
	Inner inner;

public:
	using iterator = typename Inner::iterator;
	using const_iterator = typename Inner::const_iterator;

	struct InsertResult { ref<const T> value; bool was_added; };
	InsertResult insert(T value) {
		typename std::pair<iterator, bool> inserted = inner.insert(value);
		return { &*inserted.first, inserted.second };
	}

	bool has(const T& value) const {
		return bool(inner.count(value));
	}

	const T& only() const {
		assert(size() == 1);
		return *begin();
	}

	// Get the object in the set equivalent to the argument.
	Option<ref<const T>> get_in_set(const T& value) const {
		const_iterator found = inner.find(value);
		return found == inner.end() ? Option<ref<const T>>{} : Option<ref<const T>>{&*found};
	}

	ref<const T> get_in_set_or_insert(T&& value) {
		Option<ref<const T>> already = get_in_set(value);
		if (already.has())
			return already.get();
		else
			return must_insert(std::forward<T>(value));
	}

	size_t size() const {
		return inner.size();
	}

	ref<const T> must_insert(T value) {
		std::pair<iterator, bool> inserted = inner.insert(std::forward<T>(value));
		assert(inserted.second);
		return &*inserted.first;
	}

	const_iterator begin() const { return inner.begin(); }
	const_iterator end() const { return inner.end(); }
};
