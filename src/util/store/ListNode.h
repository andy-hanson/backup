#pragma once

#include "../Option.h"
#include "../Ref.h"

template <typename T>
struct ListNode {
	T value;
	Option<Ref<ListNode>> next;

	struct const_iterator {
		Option<Ref<const ListNode>> node;
		const T& operator*() const {
			return node.get()->value;
		}
		void operator++() {
			const ListNode& n = node.get();
			// Conditional needed to keep the ref 'const'
			node = n.next.has() ? Option<Ref<const ListNode>> { n.next.get() } : Option<Ref<const ListNode>>{};
		}
		bool operator!=(const const_iterator& other) const {
			return node != other.node;
		}
	};
};
