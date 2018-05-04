#pragma once

#include "./assert.h"
#include "./int.h"
#include "./unique_ptr.h"
#include "./MaxSizeVector.h"

template <typename T>
class Grow {
	static const uint elements_per_node = 64;
	struct Node {
		union Storage {
			bool _dummy;
			T values[elements_per_node];
			Storage() {} // leave uninitialized
			~Storage() {}
		};
		Storage storage;
		Node* next;

		Node() : next(nullptr) {}
	};

	T* get_next_ptr() {
		if (empty()) {
			head = new Node();
			tail = head;
		} else if (next_index_in_node == elements_per_node) {
			Node* new_node = new Node();
			tail->next = new_node;
			tail = new_node;
			next_index_in_node = 0;
		}

		T* ptr = &tail->storage.values[next_index_in_node];
		++next_index_in_node;
		return ptr;
	}

	Node* head;
	Node* tail;
	uint next_index_in_node;

public:
	Grow() : head(nullptr), tail(nullptr), next_index_in_node(0) {}
	Grow(T first) : Grow() {
		push(first);
	}
	Grow(const Grow& other __attribute__((unused))) {
		throw "should be optimized away";
	}
	~Grow() {
		Node* n = head;
		//TODO: this assumes T has no destructor
		while (n != nullptr) {
			Node* next = n->next;
			delete n;
			n = next;
		}
	}

	uint size() const {
		uint n_nodes = 0;
		for (Node* n = head; n != nullptr; n = n->next)
			++n_nodes;
		// The last node is only partly full.
		return n_nodes == 0 ? 0 : (n_nodes - 1) * elements_per_node + next_index_in_node;
	}

	bool empty() const {
		return head == nullptr;
	}

	const T& back() const {
		assert(!empty() && tail != nullptr && next_index_in_node > 0);
		return tail->storage.values[next_index_in_node - 1];
	}

	template <typename... Arguments>
	T& emplace(Arguments&& ...arguments) {
		T* ptr = get_next_ptr();
		new (ptr) T(arguments...);
		return *ptr;
	}

	void push_copy(T value) {
		T* ptr = get_next_ptr();
		*ptr = value;
	}

	void push(T value) {
		emplace(value);
		//T* ptr = get_next_ptr();
		//*ptr = value;
	}

	class const_iterator {
		friend class Grow;
		Node* node;
		uint index;
		const_iterator(Node* _node, uint _index) : node(_node), index(_index) {}

	public:
		const T& operator*() {
			assert(node != nullptr);
			return node->storage.values[index];
		}

		void operator++() {
			++index;
			if (index == elements_per_node) {
				node = node->next;
				index = 0;
			}
		}

		bool operator==(const const_iterator& other) const {
			return node == other.node && index == other.index;
		}
		bool operator!=(const const_iterator& other) const {
			return !(*this == other);
		}
	};

	const_iterator begin() const {
		return { head, 0 };
	}
	const_iterator end() const {
		return next_index_in_node == elements_per_node ? const_iterator { nullptr, 0 } : const_iterator { tail, next_index_in_node };
	}

	friend bool operator==(const Grow<T>& a, const Grow<T>& b) {
		if (a.size() != b.size()) return false;

		Node* aNode = a.head;
		Node* bNode = b.head;

		while (aNode->next != nullptr) {
			assert(bNode->next != nullptr);

			for (uint i = 0; i != elements_per_node; ++i)
				if (aNode->storage.values[i] != bNode->storage.values[i]) return false;

			aNode = aNode->next;
			bNode = bNode->next;
		}

		assert(aNode->next == nullptr && bNode->next == nullptr);
		assert(a.next_index_in_node == b.next_index_in_node);
		for (uint i = 0; i != a.next_index_in_node; ++i)
			if (aNode->storage.values[i] != bNode->storage.values[i]) return false;

		return true;
	}
	friend bool operator!=(const Grow<T>& a, const Grow<T>& b) {
		return !(a == b);
	}

	template <typename Cb>
	void each_reverse(Cb cb) const {
		if (empty()) return;

		MaxSizeVector<32, Node*> stack;
		Node* n = head;
		while (n->next != nullptr) {
			stack.push(n);
			n = n->next;
		}
		// Last node is partially full.
		for (uint i = next_index_in_node - 1; ; --i) {
			cb(n->storage.values[i]);
			if (i == 0) break;
		}

		while (!stack.empty()) {
			n = stack.pop_and_return();
			for (uint i = elements_per_node - 1; ; --i) {
				cb(n->storage.values[i]);
				if (i == 0) break;
			}
		}
	}
};
