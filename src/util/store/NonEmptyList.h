#pragma once

#include "./Arena.h"
#include "./ListNode.h"

template <typename T>
class NonEmptyList {
	ListNode<T> _head;

public:
	inline NonEmptyList(const NonEmptyList& other) = default;
	inline NonEmptyList& operator=(const NonEmptyList& other) = default;
	inline explicit NonEmptyList(T value) : _head({ value, {} }) {}

	inline T& add(T value, Arena& arena) {
		Ref<ListNode<T>> last = &_head;
		while (last->next.has()) last = last->next.get();
		Ref<ListNode<T>> new_last = arena.put<ListNode<T>>(ListNode<T> { value, /*next*/ {} });
		last->next = new_last;
		return new_last->value;
	}

	using value_type = T;
	using const_iterator = typename ListNode<T>::const_iterator;
	inline const_iterator begin() const {
		return { Option { Ref<const ListNode<T>> { &_head } } };
	}
	inline const_iterator end() const { return { {} }; }

	inline bool has_more_than_one() const {
		return _head.next.has();
	}

	inline const T& first() const {
		return _head.value;
	}

	inline const T& only() const {
		assert(!has_more_than_one());
		return _head.value;
	}
};
