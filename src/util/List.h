#pragma once

#include "Arena.h"
#include "./int.h"

template <typename T>
class List {
	struct Node {
		T value;
		Node* next;
	};

	Node* _head;
	uint _size;

	List(Node* head, uint size) : _head(head), _size(size) {}

public:
	List() : _head(nullptr), _size(0) {}

	bool empty() const {
		return _head == nullptr;
	}

	uint size() const {
		return _size;
	}

	struct const_iterator {
		Node* node;

		const T& operator*() {
			return node->value;
		}
		void operator++() {
			node = node->next;
		}
		bool operator!=(const const_iterator& other) {
			return node != other.node;
		}
	};

	const_iterator begin() const { return { _head }; }
	const_iterator end() const { return { nullptr }; }

	class Builder {
		Node* head;
		Node* tail;
		uint size;

	public:
		Builder() : head(nullptr), tail(nullptr), size(0) {}
		Builder(const Builder& other) = delete;

		void add(T value, Arena& arena) {
			Node* next = arena.put<Node>({ value, nullptr }).ptr();
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
