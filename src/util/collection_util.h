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

template <typename T, typename Cb>
bool each_corresponds(const DynArray<T>& da, const DynArray<T>& db, Cb cb) {
	assert(da.size() == db.size());
	typename DynArray<T>::const_iterator ia = da.begin();
	typename DynArray<T>::const_iterator ib = db.begin();
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
bool contains_ref(const DynArray<T>& collection, ref<const T> value) {
	for (const T& v : collection)
		if (ref<const T>(&v) == value)
			return true;
	return false;
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
