#pragma once

#include "../int.h"
#include "../Option.h"
#include "./Arena.h"
#include "./ListNode.h"

template <typename T>
class List {
	Option<Ref<const ListNode<T>>> _head;
	uint _size;

public:
	inline List() : _head{}, _size{0} {}
	inline List(Option<Ref<const ListNode<T>>> head, uint size) : _head{head}, _size{size} {}

	inline bool is_empty() const {
		return !_head.has();
	}

	inline uint size() const {
		return _size;
	}

	using const_iterator = typename ListNode<T>::const_iterator;
	inline const_iterator begin() const { return { _head }; }
	inline const_iterator end() const { return { {} }; }
};
