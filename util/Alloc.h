#pragma once

#include <assert.h>
#include <memory>
#include <deque>
#include <utility> // std::forward

#include "MaxSizeVector.h"
#include "StringSlice.h"
#include "ptr.h"

template <typename T>
class List {
	friend class NoDropArena;

public: // TODO: not public!
	struct Node {
		T value;
		Node* next;
	};

	Node* head; // may be nullptr
	List(Node* _head) : head(_head) {}

public:
	List() : head(nullptr) {}

	class const_iterator {
		friend class List;
		const Node* n;
		const_iterator(const Node* _n) : n(_n) {}

	public:
		const T& operator*() {
			return n->value;
		}

		void operator++() {
			n = n->next;
		}

		bool operator==(const_iterator other) {
			return n == other.n;
		}
		bool operator!=(const_iterator other) {
			return n != other.n;
		}
	};

	const_iterator begin() const {
		return { head };
	}
	const_iterator end() const {
		return { nullptr };
	}

	size_t size() const {
		size_t size = 0;
		for (const T& _ __attribute__((unused)) : *this) ++size;
		return size;
	}
	bool empty() const { return head == nullptr; }
};

template <typename T>
class DynArray {
	T* data;
	size_t len;

	friend class Arena;

	DynArray(T* _data, size_t _len) : data(_data), len(_len) {}

public:
	DynArray() : data(nullptr), len(0) {}

	template <typename Cb>
	void fill(Cb cb) {
		for (uint i = 0; i < len; ++i) {
			data[i] = cb(i);
		}
	}

	T& operator[](size_t index) {
		assert(index < len);
		return data[index];
	}

	const T& operator[](size_t index) const {
		assert(index < len);
		return data[index];
	}

	// This is in an arena, so don't have to worry about storing the 'capacity'.
	void discard_after_length(size_t new_len) {
		assert(new_len < len);
		len = new_len;
	}

	using const_iterator = const T*;

	size_t size() const { return len; }
	bool empty() const { return len == 0; }
	const_iterator begin() { return data; }
	const_iterator end() { return data + len; }
	const T* begin() const { return data; }
	const T* end() const { return data + len; }
};

// Wrapper around StringSlice that ensures it's in an arena.
class ArenaString {
	friend class Arena;
	StringSlice _slice;
	// Keep this private to avoid using a non-arena slice.
	ArenaString(StringSlice slice) : _slice(slice) {}
public:
	operator StringSlice() const {
		return _slice;
	}
	StringSlice slice() const { return _slice; }
};

template <typename T>
class Emplacer {
	friend class Arena;

	T* ptr;
	Emplacer(T* _ptr) : ptr(_ptr) {}

public:
	template <typename... Arguments>
	ref<T> operator()(Arguments&&... arguments) {
		new(ptr) T { std::forward<Arguments>(arguments)... };
		return ptr;
	}
};

class Arena {
	void* alloc_begin;
	void* alloc_next;
	void* alloc_end;

	void* allocate(size_t n_bytes) {
		void* res = alloc_next;
		alloc_next = static_cast<char*>(alloc_next) + n_bytes;
		assert(alloc_next <= alloc_end);
		return res;
	}

public:
	Arena() {
		size_t size = 1000;
		alloc_begin = ::operator new(size);
		alloc_end = static_cast<char*>(alloc_begin) + size;
		alloc_next = alloc_begin;
	}
	Arena(const Arena& other) = delete;
	void operator=(const Arena& other) = delete;
	~Arena() {
		::operator delete(alloc_begin);
	}
	Arena(Arena&& other) {
		alloc_begin = other.alloc_begin;
		alloc_next = other.alloc_next;
		alloc_end = other.alloc_end;

		other.alloc_begin = nullptr;
		other.alloc_next = nullptr;
		other.alloc_end = nullptr;
	}

	/** User of this method is responsible for ensuring that the arena contents aren't referenced anywhere. */
	void clear() {
		alloc_next = alloc_begin;
	}

	template <typename T>
	ref<T> emplace_copy(T value) {
		T* ptr =  static_cast<T*>(allocate(sizeof(T)));
		new(ptr) T(value);
		return ptr;
	}

	template <typename T>
	Emplacer<T> emplace() {
		return Emplacer<T>(static_cast<T*>(allocate(sizeof(T))));
	}

	template <typename T>
	DynArray<T> new_array(size_t len) {
		return DynArray<T>(static_cast<T*>(allocate(sizeof(T) * len)), len);
	}

	template <typename T>
	DynArray<T> make_array(T elem) {
		return fill_array<T>(1)([&](uint i) {
			assert(i == 0);
			return elem;
		});
	}

	template <typename T>
	struct ArrayFiller {
		friend class Arena;
		DynArray<T> inner;
		ArrayFiller(DynArray<T> _inner) : inner(_inner) {}

	public:
		template <typename Cb>
		DynArray<T> operator()(Cb cb) {
			inner.fill(cb);
			return inner;
		}
	};

	template <typename T>
	ArrayFiller<T> fill_array(size_t len) {
		return ArrayFiller<T>(new_array<T>(len));
	}

	template <typename In, typename Out>
	struct ArrayMapper {
		const DynArray<In>& in;
		DynArray<Out> inner;
		ArrayMapper(const DynArray<In>& _in, DynArray<Out> _inner) : in(_in), inner(_inner) {}

		template <typename Cb>
		DynArray<Out> operator()(Cb cb) {
			inner.fill([&](uint i) { return cb(in[i]); });
			return inner;
		}
	};
	template <typename In, typename Out>
	ArrayMapper<In, Out> map_array(const DynArray<In>& in) {
		return ArrayMapper(in, new_array<Out>(in.size()));
	}

	template <typename Out>
	struct ToArray {
		Arena& arena;

		template <typename In, typename Cb>
		DynArray<Out> operator()(const List<In>& ins, Cb cb) {
			typename List<In>::const_iterator iter = ins.begin();
			DynArray<Out> res = arena.fill_array<Out>(ins.size())([&](uint _ __attribute__((unused))) {
				assert(iter != ins.end());
				Out o = cb(*iter);
				++iter;
				return o;
			});
			assert(iter == ins.end());
			return res;
		}
	};

	template <typename Out>
	ToArray<Out> to_array() {
		return { *this };
	}

	template <typename T>
	class SmallArrayBuilder {
		friend class Arena;
		ref<Arena> arena;
		MaxSizeVector<8, T> v;

		SmallArrayBuilder(ref<Arena> _arena) : arena(_arena) {}

	public:
		void add(T value) {
			v.push(value);
		}

		template <typename... Arguments>
		ref<T> emplace(Arguments&&... arguments) {
			return v.emplace(std::forward<Arguments>(arguments)...);
		}

		DynArray<T> finish() {
			return arena->fill_array<T>(v.size())([&](uint i) {
				return v[i];
			});
		}
	};

	template <typename T>
	SmallArrayBuilder<T> small_array_builder() {
		return SmallArrayBuilder<T>(this);
	}

	template <typename T>
	class ListBuilder {
		friend class Arena;

		ref<Arena> arena;
		typename List<T>::Node* head;
		typename List<T>::Node* tail;

		ListBuilder(ref<Arena> _arena, typename List<T>::Node* _head, typename List<T>::Node* _tail) : arena(_arena), head(_head), tail(_tail) {}

	public:
		void add(T value) {
			typename List<T>::Node* next = arena->emplace<typename List<T>::Node>()(value, nullptr).ptr();
			(tail == nullptr ? head : tail->next) = next;
			tail = next;
		}
		List<T> finish() {
			return List<T> { head };
		}
	};

	template <typename T>
	ListBuilder<T> list_builder() {
		return ListBuilder<T> { this, nullptr, nullptr };
	}

	class StringBuilder {
		Arena& arena;
		char* begin;
		char* ptr;
		char* end;

		friend class Arena;
		StringBuilder(Arena& _arena, char* _begin, char* _end) : arena(_arena), begin(_begin), ptr(_begin), end(_end) {}

	public:
		void add(char c) {
			assert(ptr != end);
			*ptr = c;
			++ptr;
		}

		char back() {
			assert(ptr != begin);
			return *(ptr - 1);
		}

		void pop() {
			assert(ptr != begin);
			--ptr;
		}

		ArenaString finish() {
			assert(arena.alloc_next == end); // no intervening allocations
			arena.alloc_next = ptr; // Only used up this much space, don't waste the rest
			return { { begin, ptr } };
		}
	};

	StringBuilder string_builder(size_t size) {
		char* begin = static_cast<char*>(allocate(size));
		return StringBuilder(*this, begin, begin + size);
	}

	ArenaString str(StringSlice slice) {
		char* begin = static_cast<char*>(allocate(slice.size()));
		std::copy(slice.begin(), slice.end(), begin);
		return { { begin, begin + slice.size() } };
	}
};

// Collection that's guaranteed not to move, so elements can be pointed to.
template <typename T>
class NonMovingCollection {
	std::deque<T> data;

public:
	NonMovingCollection() {}
	// This transfers ownership of the deque, it doesn't move its contents.
	NonMovingCollection(NonMovingCollection&& other) : data(std::move(other.data)) {}

	NonMovingCollection(const NonMovingCollection& other) = delete;
	void operator=(NonMovingCollection& other) = delete;

	typename std::deque<T>::iterator begin() { return data.begin(); }
	typename std::deque<T>::iterator end() { return data.end(); }
	typename std::deque<T>::const_iterator begin() const { return data.begin(); }
	typename std::deque<T>::const_iterator end() const { return data.end(); }

	template <typename... Arguments>
	T& emplace(Arguments&&... arguments) {
		data.emplace_back(std::forward<Arguments>(arguments)...);
		return data.back();
	}
};
