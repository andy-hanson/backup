#pragma once

#include "../Option.h"

template <typename Collection, typename Pred>
Option<Ref<const typename Collection::value_type>> find(const Collection& collection, Pred pred) {
	for (const typename Collection::value_type& t : collection)
		if (pred(t))
			return Option<Ref<const typename Collection::value_type>>{&t};
	return {};
}

template <typename Collection, typename Pred>
Option<Ref<const typename Collection::value_type>> find_in_either(const Collection& collection_a, const Collection& collection_b, Pred pred) {
	return or_option(find(collection_a, pred), [&]() { return find(collection_b, pred); });
};

template <typename Collection, typename Pred>
bool some(const Collection& collection, Pred pred) {
	return find(collection, pred).has();
};

template <typename Collection, typename Pred>
bool every(const Collection& collection, Pred pred) {
	return !some(collection, [&](const typename Collection::value_type v) { return !pred(v); });
}

template <typename Collection>
bool contains(const Collection& collection, const typename Collection::value_type& value) {
	return some(collection, [&](const typename Collection::value_type& v) { return v == value; });
}

template <typename Collection1, typename Collection2, typename /*const T&, U& => void*/ Cb>
void zip(const Collection1& a, Collection2& b, Cb cb) {
	assert(a.size() == b.size());
	uint i = 0;
	for (const typename Collection1::value_type& t : a) {
		cb(t, b[i]);
		++i;
	}
}

template <typename Collection1, typename Collection2, typename /*const T&, U&, bool => void*/ Cb>
void zip_with_is_last(const Collection1& a, Collection2& b, Cb cb) {
	assert(a.size() == b.size());
	uint i = 0;
	for (const typename Collection1::value_type& t : a) {
		cb(t, b[i], i == b.size() - 1);
		++i;
	}
}
