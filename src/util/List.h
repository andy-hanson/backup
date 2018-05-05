#pragma once

#include "Arena.h"
#include "./int.h"

template <typename T>
struct ListNode {
	T value;
	ListNode* next;

	struct const_iterator {
		const ListNode<T>* node;
		const T& operator*() const {
			return node->value;
		}
		void operator++() {
			node = node->next;
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
	NonEmptyList(T value) : _head({ value, nullptr }) {}

	// Note: this moves the current head out.
	void prepend(T value, Arena& arena) {
		ListNode<T>* next = arena.put<ListNode<T>>(_head).ptr();
		_head = { value, next };
	}

	using const_iterator = typename ListNode<T>::const_iterator;
	const_iterator begin() const { return { &_head }; }
	const_iterator end() const { return { nullptr }; }

	bool has_more_than_one() const {
		return _head.next != nullptr;
	}

	const T& first() const {
		return _head.value;
	}
};

template <typename T>
class List {
	ListNode<T>* _head;
	uint _size;

	List(ListNode<T>* head, uint size) : _head(head), _size(size) {}

public:
	List() : _head(nullptr), _size(0) {}

	bool empty() const {
		return _head == nullptr;
	}

	uint size() const {
		return _size;
	}

	using const_iterator = typename ListNode<T>::const_iterator;
	const_iterator begin() const { return { _head }; }
	const_iterator end() const { return { nullptr }; }

	class Builder {
		ListNode<T>* head;
		ListNode<T>* tail;
		uint size;

	public:
		Builder() : head(nullptr), tail(nullptr), size(0) {}
		Builder(const Builder& other) = delete;

		bool empty() const {
			return head == nullptr;
		}

		void add(T value, Arena& arena) {
			ListNode<T>* next = arena.put<ListNode<T>>({ value, nullptr }).ptr();
			if (head == nullptr) {
				assert(size == 0);
				head = next;
			} else {
				assert(tail != nullptr);
				tail->next = next;
			}
			tail = next;
			++size;
		}

		List finish() {
			return { head, size };
		}
	};
};
