#pragma once

#include "../int.h"
#include "../Option.h"
#include "../Ref.h"
#include "./Slice.h"

template <typename T, typename U, typename /*const T&, U& => void*/ Cb >
void zip(const Slice<T>& da, Slice<U>& db, Cb cb) {
	assert(da.size() == db.size());
	typename Slice<T>::const_iterator ia = da.begin();
	typename Slice<U>::iterator ib = db.begin();
	while (ia != da.end()) {
		cb(*ia, *ib);
		++ia;
		++ib;
	}
	assert(ib == db.end());
}

template <typename T, typename U, typename /*T, U => bool*/ Cb>
bool each_corresponds(const Slice<T>& da, const Slice<U>& db, Cb cb) {
	if (da.size() != db.size()) return false;
	typename Slice<T>::const_iterator ia = da.begin();
	typename Slice<U>::const_iterator ib = db.begin();
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
bool operator==(Slice<T> da, Slice<T> db) {
	return each_corresponds(da, db, [](const T& a, const T& b) { return a == b; });
}

template <typename T>
Option<uint> try_get_index(const Slice<T>& collection, Ref<const T> value) {
	long idx = value.ptr() - collection.begin();
	if (idx < 0 || idx >= collection.size()) return {};
	return Option<uint> { to_unsigned(idx) };
}

template <typename T>
uint get_index(const Slice<T> collection, Ref<const T> value) {
	return try_get_index(collection, value).get();
}

template <typename T>
bool contains_ref(const Slice<T>& collection, Ref<const T> value) {
	return try_get_index(collection, value).has();
}
