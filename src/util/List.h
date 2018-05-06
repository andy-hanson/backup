#pragma once

#include "./Arena.h"
#include "./int.h"
#include "./Option.h"

template <typename T>
struct ListNode {
	T value;
	Option<Ref<const ListNode>> next;

	struct const_iterator {
		Option<Ref<const ListNode>> node;
		const T& operator*() const {
			return node.get()->value;
		}
		void operator++() {
			node = node.get()->next;
		}
		bool operator!=(const const_iterator& other) const {
			return node != other.node;
		}
	};
};

template <typename T>
class NonEmptyList {
	template<typename> friend class List;
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

	using const_iterator = typename ListNode<T>::const_iterator;
	inline const_iterator begin() const {
		return { Option { Ref<const ListNode<T>> { &_head } } };
	}
	inline const_iterator end() const { return { {} }; }

	bool has_more_than_one() const {
		return _head.next.has();
	}

	const T& first() const {
		return _head.value;
	}
};

template <typename T>
class List {
	Option<Ref<const ListNode<T>>> _head;
	uint _size;

	List(Option<Ref<const ListNode<T>>> head, uint size) : _head(head), _size(size) {}

public:
	List() : _head{}, _size{0} {}

	bool is_empty() const {
		return !_head.has();
	}

	uint size() const {
		return _size;
	}

	using const_iterator = typename ListNode<T>::const_iterator;
	const_iterator begin() const { return { _head }; }
	const_iterator end() const { return { {} }; }

	class Builder {
		Option<Ref<ListNode<T>>> head;
		Option<Ref<ListNode<T>>> tail;
		uint size;

	public:
		Builder() : head{}, tail{}, size{0} {}
		Builder(const Builder& other) = delete;

		bool is_empty() const {
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

		List finish() {
			return { head.has() ? Option<Ref<const ListNode<T>>> { head.get() } : Option<Ref<const ListNode<T>>> {}, size };
		}
	};
};
