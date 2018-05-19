#pragma once

#include "../int.h"
#include "../Option.h"
#include "./Arena.h"
#include "./ListNode.h"

template <typename T>
class List {
	Option<Ref<ListNode<T>>> _head;
	uint _size;

public:
	inline List() : _head{}, _size{0} {}
	inline List(Option<Ref<ListNode<T>>> head, uint size) : _head{head}, _size{size} {}
	inline List(T value, Arena& arena) : _head{Option<Ref<ListNode<T>>> { arena.put(ListNode<T> { value, {} }) }}, _size{1} {}

	inline bool is_empty() const {
		return !_head.has();
	}

	inline uint size() const {
		return _size;
	}

	void prepend(T value, Arena& arena) {
		_head = arena.put(ListNode<T> { value, _head });
		++_size;
	}

	using value_type = T;
	using const_iterator = typename ListNode<T>::const_iterator;
	inline const_iterator begin() const {
		// Conditional needed to make it 'const'
		return { _head.has() ? Option<Ref<const ListNode<T>>> { _head.get() } : Option<Ref<const ListNode<T>>> {} };
	}
	inline const_iterator end() const {
		return { {} };
	}
};
