#pragma once

#include <assert.h>

#include "./MaxSizeVector.h"
#include "./StringSlice.h"
#include "./ptr.h"
#include "./Option.h"

template <typename T>
class Arr {
	T* data;
	size_t len;

	friend class Arena;

	Arr(T* _data, size_t _len) : data(_data), len(_len) {}

public:
	Arr() : data(nullptr), len(0) {}

	T& operator[](size_t index) {
		assert(index < len);
		return data[index];
	}

	const T& operator[](size_t index) const {
		assert(index < len);
		return data[index];
	}

	bool contains_ref(ref<const T> r) const {
		return begin() <= r.ptr() && r.ptr() < end();
	}

	using iterator = T*;
	using const_iterator = const T*;

	size_t size() const { return len; }
	bool empty() const { return len == 0; }
	iterator begin() { return data; }
	iterator end() { return data + len; }
	const_iterator begin() const { return data; }
	const_iterator end() const { return data + len; }
};

// Wrapper around StringSlice that ensures it's in an arena.
class ArenaString {
	friend class Arena;
	StringSlice _slice;
	// Keep this private to avoid using a non-arena slice.
	ArenaString(StringSlice slice) : _slice(slice) {}
public:
	ArenaString() : _slice() {}
	inline operator StringSlice() const { return _slice; }
	inline StringSlice slice() const { return _slice; }
};
inline bool operator==(const ArenaString& a, const ArenaString& b) { return a.slice() == b.slice();}
inline bool operator==(const ArenaString& a, const StringSlice& b) { return a.slice() == b; }
namespace std {
	template <>
	struct hash<ArenaString> {
		size_t operator()(const ArenaString& a) const { return hash<StringSlice>{}(a); }
	};
}

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
		size_t size = 10000;
		alloc_begin = ::operator new(size);
		alloc_end = static_cast<char*>(alloc_begin) + size;
		alloc_next = alloc_begin;
	}
	Arena(const Arena& other) = delete;
	void operator=(const Arena& other) = delete;
	~Arena() {
		::operator delete(alloc_begin);
	}

	template <typename T>
	ref<T> put(T&& value) {
		T* ptr =  static_cast<T*>(allocate(sizeof(T)));
		new(ptr) T(std::forward<T>(value));
		return ptr;
	}

	template <typename T>
	ref<T> put_copy(T value) {
		T* ptr =  static_cast<T*>(allocate(sizeof(T)));
		new(ptr) T(value);
		return ptr;
	}

	template <typename T>
	Arr<T> new_array(size_t len) {
		return Arr<T>(static_cast<T*>(allocate(sizeof(T) * len)), len);
	}

	template <typename T>
	Arr<T> make_array(T elem) {
		return fill_array<T>()(1, [&](uint i) { assert(i == 0); return elem; });
	}

	template <typename T>
	struct ArrayFiller {
		friend class Arena;
		Arena& arena;

	public:
		template <typename Cb>
		Arr<T> operator()(size_t len, Cb cb) {
			Arr<T> arr = arena.new_array<T>(len);
			for (uint i = 0; i < len; ++i)
				arr[i] = cb(i);
			return arr;
		}
	};

	template <typename T>
	ArrayFiller<T> fill_array() { return {*this}; }

	template <typename Out>
	struct ArrayMapper {
		Arena& arena;
		template <typename In, typename /*const In& => Out*/ Cb>
		Arr<Out> operator()(const Arr<In>& in, Cb cb) {
			return arena.fill_array<Out>()(in.size(), [&](uint i) { return cb(in[i]); });
		}
	};
	template <typename Out>
	ArrayMapper<Out> map() {
		return {*this};
	}

	template <typename Out>
	struct MapWithPrevs {
		Arena& arena;
		template <typename In, typename /*const In&, const Arr<Out>&, uint => Out*/ Cb>
		Arr<Out> operator()(const Arr<In>& inputs, Cb cb) {
			Arr<Out> out = arena.new_array<Out>(inputs.size());
			out.len = 0;
			uint i = 0;
			for (const In& input : inputs) {
				Out o = cb(input, out, i);
				++out.len;
				out[i] = o;
				++i;
			}
			assert(out.len == i);
			return out;
		}
	};
	template <typename Out>
	MapWithPrevs<Out> map_with_prevs() { return {*this}; }

	template <typename Out>
	struct MapOp {
		Arena& arena;

		template <typename In, typename /*const In& => Option<Out>*/ Cb>
		Arr<Out> operator()(const Arr<In>& inputs, Cb cb) {
			Arr<Out> out = arena.new_array<Out>(inputs.size());
			uint i = 0;
			for (const In& input : inputs) {
				Option<Out> o = cb(input);
				if (o) {
					out[i] = o.get();
					++i;
				}
			}
			out.len = i;
			return out;
		}
	};
	// Note: we assume that the output will probably be the same size as the input, or else it's a compile error.
	template <typename Out>
	MapOp<Out> map_op() {
		return MapOp<Out>{*this};
	}

	template <typename Out>
	struct MapOpWithPrevs {
		Arena& arena;

		template <typename In, typename /*(const In&, const Arr<Out>&) => Option<Out>*/ Cb>
		Arr<Out> operator()(const Arr<In>& inputs, Cb cb) {
			Arr<Out> out = arena.new_array<Out>(inputs.size());
			out.len = 0;
			uint i = 0;
			for (const In& input : inputs) {
				Option<Out> o = cb(input, out);
				if (o) {
					++out.len;
					out[i] = o.get();
					++i;
				}
			}
			out.len = i;
			return out;
		}
	};
	template <typename Out>
	MapOpWithPrevs<Out> map_op_with_prevs() { return MapOpWithPrevs<Out>{*this}; }

	template <typename T>
	class SmallArrayBuilder {
		friend class Arena;
		ref<Arena> arena;
		MaxSizeVector<8, T> v;

		SmallArrayBuilder(ref<Arena> _arena) : arena(_arena) {}

	public:
		void add(T value) { v.push(value); }

		Arr<T> finish() {
			return arena->fill_array<T>()(v.size(), [&](uint i) { return v[i]; });
		}
	};

	template <typename T>
	SmallArrayBuilder<T> small_array_builder() {
		return SmallArrayBuilder<T>(this);
	}

	class StringBuilder {
		Arena& arena;
		char* begin;
		char* ptr;
		char* end;

		friend class Arena;
		StringBuilder(Arena& _arena, char* _begin, char* _end) : arena(_arena), begin(_begin), ptr(_begin), end(_end) {}

	public:
		StringBuilder& operator<<(char c) {
			assert(ptr != end);
			*ptr = c;
			++ptr;
			return *this;
		}

		StringBuilder& operator<<(uint u) {
			assert(ptr != end);
			//TODO: better
			if (u < 10) {
				*ptr = '0' + char(u);
				++ptr;
			} else if (u < 100) {
				assert(ptr + 1 != end);
				*ptr = '0' + char(u % 10);
				*(ptr + 1) = '0' + char(u / 10);
				++ptr;
			} else
				throw "todo";
			return *this;
		}

		StringBuilder& operator<<(StringSlice s) {
			for (char c : s)
				*this << c;
			return *this;
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

	StringBuilder string_builder(size_t max_size) {
		char* begin = static_cast<char*>(allocate(max_size));
		return StringBuilder(*this, begin, begin + max_size);
	}

	ArenaString str(StringSlice slice) {
		char* begin = static_cast<char*>(allocate(slice.size()));
		char* end = std::copy(slice.begin(), slice.end(), begin);
		assert(size_t(end - begin) == slice.size());
		assert(alloc_next == end);
		return { { begin, end } };
	}
};
