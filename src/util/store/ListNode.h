#pragma once

#include "../Option.h"
#include "../Ref.h"

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
