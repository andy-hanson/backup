#pragma once

#include "Alloc.h"
#include "Option.h"

template <typename T, typename Pred>
Option<const T&> find(const DynArray<T>& collection, Pred pred) {
	for (const T& t : collection)
		if (pred(t))
			return { t };
	return {};
}

template <uint size, typename T, typename Pred>
Option<const T&> find(const MaxSizeVector<size, T>& collection, Pred pred) {
	for (const T& t : collection)
		if (pred(t))
			return { t };
	return {};
}


template <typename T, typename Pred>
Option<const T&> find_in_either(const DynArray<T>& collection_a, const DynArray<T>& collection_b, Pred pred) {
	return find(collection_a, pred).or_option([&]() { return find(collection_b, pred); });
};


template <typename T, typename Pred>
bool every(const DynArray<T>& collection, Pred pred) {
	for (const T& t : collection)
		if (!pred(t))
			return false;
	return true;
}

template <typename T, typename Pred>
bool some(const DynArray<T>& collection, Pred pred) {
	for (const T& t : collection)
		if (pred(t))
			return true;
	return false;
};

template <typename T, typename U, typename /*(T, U) => bool*/ Cb>
bool each_corresponds(const DynArray<T>& da, const DynArray<U>& db, Cb cb) {
	if (da.size() != db.size()) return false;
	typename DynArray<T>::const_iterator ia = da.begin();
	typename DynArray<U>::const_iterator ib = db.begin();
	while (ia != da.end()) {
		if (!cb(*ia, *ib))
			return false;
		++ia;
		++ib;
	}
	assert(ib == db.end());
	return true;
}

template <typename T>
bool operator==(DynArray<T> da, DynArray<T> db) {
	return each_corresponds(da, db, [](const T& a, const T& b) { return a == b; });
}

template <typename T>
Option<uint> try_get_index(const DynArray<T>& collection, ref<const T> value) {
	//TODO:PERF always use fast way
	ptrdiff_t fast_way = value.ptr() - collection.begin();
	if (fast_way < 0 || fast_way >= ptrdiff_t(collection.size())) return {};

	uint i = 0;
	for (const T& v : collection) {
		if (ref<const T>(&v) == value) {
			assert(i == fast_way);
			return i;
		}
		++i;
	}
	assert(false);
}

template <typename T>
uint get_index(const DynArray<T> collection, ref<const T> value) {
	return try_get_index(collection, value).get();
}

template <typename T, typename /*T => bool*/ Pred>
uint get_index_where(const DynArray<T> collection, Pred pred) {
	for (uint i = 0; i != collection.size(); ++i)
		if (pred(collection[i]))
			return i;
	assert(false);
};

template <typename T>
bool contains_ref(const DynArray<T>& collection, ref<const T> value) {
	return try_get_index(collection, value);
}


template <uint capacity, typename T, typename Pred>
void filter_unordered(MaxSizeVector<capacity, T>& collection, Pred pred) {
	for (uint i = 0; i != collection.size(); ) {
		if (pred(collection[i])) {
			++i;
		} else {
			collection[i] = collection[collection.size() - 1];
			collection.pop();
		}
	}
}
