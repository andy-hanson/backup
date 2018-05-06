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
