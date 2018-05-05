#pragma once

#include "./Arena.h"
#include "./assert.h"
#include "./int.h"
#include "./Option.h"
#include "./Ref.h"
#include "./MaxSizeVector.h"

template <typename T>
class BlockedList {
	static const uint elements_per_node = 64;
	struct Node {
		T values[elements_per_node];
		Option<Ref<Node>> next;

		Node() : next(nullptr) {}
	};

	T* get_next_ptr(Arena& arena) {
		if (empty()) {
			head = arena.allocate_uninitialized<Node>();
			head.get()->next = {};
			tail = head;
		} else if (next_index_in_node == elements_per_node) {
			Ref<Node> new_node = arena.allocate_uninitialized<Node>();
			new_node->next = {};
			tail.get()->next = new_node;
			tail = new_node;
			next_index_in_node = 0;
		}

		T* ptr = &tail.get()->values[next_index_in_node];
		++next_index_in_node;
		return ptr;
	}

	Option<Ref<Node>> head;
	Option<Ref<Node>> tail;
	uint next_index_in_node;

public:
	BlockedList() : head{}, tail{}, next_index_in_node{0} {}
	BlockedList(T first) : BlockedList() {
		push(first);
	}
	BlockedList(const BlockedList& other __attribute__((unused))) {
		throw "should be optimized away";
	}

	uint size() const {
		uint n_nodes = 0;
		for (Option<Ref<Node>> n = head; n.has(); n = n.get()->next)
			++n_nodes;
		// The last node is only partly full.
		return n_nodes == 0 ? 0 : (n_nodes - 1) * elements_per_node + next_index_in_node;
	}

	bool empty() const {
		return !head.has();
	}

	const T& back() const {
		assert(!empty() && next_index_in_node > 0);
		return tail.get()->values[next_index_in_node - 1];
	}

	T& push(T value, Arena& arena) {
		T* ptr = get_next_ptr(arena);
		*ptr = value;
		return *ptr;
	}

	class const_iterator {
		friend class BlockedList;
		Option<Ref<Node>> node;
		uint index;
		const_iterator(Option<Ref<Node>> _node, uint _index) : node{_node}, index{_index} {}

	public:
		inline const T& operator*() {
			return node.get()->values[index];
		}

		inline void operator++() {
			++index;
			if (index == elements_per_node) {
				node = node.get()->next;
				index = 0;
			}
		}

		inline bool operator==(const const_iterator& other) const {
			return node == other.node && index == other.index;
		}
		inline bool operator!=(const const_iterator& other) const {
			return !(*this == other);
		}
	};

	const_iterator begin() const {
		return { head, 0 };
	}
	const_iterator end() const {
		return next_index_in_node == elements_per_node ? const_iterator { {}, 0 } : const_iterator { tail, next_index_in_node };
	}

	template <typename Cb>
	void each_reverse(Cb cb) const {
		if (empty()) return;

		MaxSizeVector<32, Ref<Node>> stack;
		Ref<Node> n = head.get();
		while (n->next.has()) {
			stack.push(n);
			n = n->next.get();
		}
		// Last node is partially full.
		for (uint i = next_index_in_node - 1; ; --i) {
			cb(n->values[i]);
			if (i == 0) break;
		}

		while (!stack.empty()) {
			n = stack.pop_and_return();
			for (uint i = elements_per_node - 1; ; --i) {
				cb(n->values[i]);
				if (i == 0) break;
			}
		}
	}
};
