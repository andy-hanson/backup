#pragma once

#include "./ListNode.h"

template <typename T>
class NonEmptyList {
	ListNode<T> _head;

public:
	NonEmptyList(const NonEmptyList& other) = default;
	NonEmptyList& operator=(const NonEmptyList& other) = default;
	NonEmptyList(T value) : _head({ value, {} }) {}

	// Note: this moves the current head out.
	void prepend(T value, Arena& arena) {
		Ref<ListNode<T>> next = arena.put<ListNode<T>>(_head).ptr();
		_head = ListNode<T> { value, Option<Ref<const ListNode<T>>> { next } };
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
};
