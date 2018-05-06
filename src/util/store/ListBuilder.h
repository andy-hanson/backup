#pragma once

#include "./Arena.h"
#include "./List.h"

template <typename T>
class ListBuilder {
	Option<Ref<ListNode<T>>> head;
	Option<Ref<ListNode<T>>> tail;
	uint size;

public:
	inline ListBuilder() : head{}, tail{}, size{0} {}
	ListBuilder(const ListBuilder& other) = delete;

	inline bool is_empty() const {
		return !head.has();
	}

	void add(T value, Arena& arena) {
		Ref<ListNode<T>> next = arena.put<ListNode<T>>({ value, {} }).ptr();
		if (head.has()) {
			assert(tail.has());
			tail.get()->next = next;
		} else {
			assert(size == 0);
			head = next;
		}
		tail = next;
		++size;
	}

	List<T> finish() {
		return { head.has() ? Option<Ref<const ListNode<T>>> { head.get() } : Option<Ref<const ListNode<T>>> {}, size };
	}
};
