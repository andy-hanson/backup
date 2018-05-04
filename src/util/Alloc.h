#pragma once

#include <new> // new ()
#include "./MaxSizeVector.h"
#include "./StringSlice.h"
#include "./ptr.h"
#include "./Option.h"
#include "./Slice.h"
#include "Grow.h"

// Wrapper around StringSlice that ensures it's in an arena.
class ArenaString {
	friend class Arena;
	char* _begin;
	char* _end;
	// Keep this private to avoid using a non-arena slice.
	ArenaString(char* begin, char* end) : _begin(begin), _end(end) {}
public:
	ArenaString(const ArenaString& other) = default;
	ArenaString& operator=(const ArenaString& other) = default;
	ArenaString() : _begin(nullptr), _end(nullptr) {}
	char* begin() { return _begin; }
	char* end() { return _end; }
	inline operator StringSlice() const { return slice(); }
	inline StringSlice slice() const { return { _begin, _end }; }
	inline char& operator[](size_t index) {
		assert(index < slice().size());
		return *(_begin + index);
	}

	struct hash {
		inline size_t operator()(const ArenaString& a) const { return StringSlice::hash{}(a); }
	};
};
inline bool operator==(const ArenaString& a, const ArenaString& b) { return a.slice() == b.slice();}
inline bool operator==(const ArenaString& a, const StringSlice& b) { return a.slice() == b; }

class Arena {
	void* alloc_begin;
	void* alloc_next;
	void* alloc_end;

	void* allocate(size_t n_bytes);

	template <typename T>
	Arr<T> uninitialized_array(size_t len) {
		return Arr<T>(static_cast<T*>(allocate(sizeof(T) * len)), len);
	}

public:
	Arena();
	Arena(const Arena& other) = delete;
	void operator=(const Arena& other) = delete;
	~Arena();

	template <typename T>
	ref<T> allocate_uninitialized() {
		return static_cast<T*>(allocate(sizeof(T)));
	}

	template <typename T>
	ref<T> put(T value) {
		ref<T> ptr = allocate_uninitialized<T>();
		new(ptr.ptr()) T(value);
		return ptr;
	}

	template <typename T>
	Arr<T> single_element_array(T elem) {
		return Arr<T>(put(elem).ptr(), 1);
	}

	template <typename T>
	struct ArrayFiller {
		friend class Arena;
		Arena& arena;

	public:
		template <typename Cb>
		Arr<T> operator()(size_t len, Cb cb) {
			if (len == 0) return {};
			Arr<T> arr = arena.uninitialized_array<T>(len);
			for (uint i = 0; i < len; ++i)
				arr[i] = cb(i);
			return arr;
		}
	};

	template <typename T>
	ArrayFiller<T> fill_array() { return {*this}; }

	template <typename Out>
	struct Mapper {
		Arena& arena;

		template <typename In, typename /*const In& => Out*/ Cb>
		Arr<Out> operator()(const Arr<In>& in, Cb cb) {
			if (in.empty()) return {};
			return arena.fill_array<Out>()(in.size(), [&](uint i) { return cb(in[i]); });
		}

		template <typename In, typename /*const In& => Out*/ Cb>
		Arr<Out> operator()(const Grow<In>& in, Cb cb) {
			if (in.empty()) return {};
			uint size = in.size();
			Arr<Out> arr = arena.uninitialized_array<Out>(size);
			uint i = 0;
			for (const In& input : in) {
				arr[i] = cb(input);
				++i;
			}
			assert(i == size);
			return arr;
		}
	};
	template <typename Out>
	Mapper<Out> map() {
		return {*this};
	}

	template <typename Out>
	struct MapWithPrevs {
		Arena& arena;
		template <typename In, typename /*const In&, const Arr<Out>&, uint => Out*/ Cb>
		Arr<Out> operator()(const Arr<In>& inputs, Cb cb) {
			if (inputs.empty()) return {};
			Arr<Out> out = arena.uninitialized_array<Out>(inputs.size());
			out._size = 0;
			uint i = 0;
			for (const In& input : inputs) {
				Out o = cb(input, out, i);
				++out._size;
				out[i] = o;
				++i;
			}
			assert(out._size == i);
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
			Arr<Out> out = arena.uninitialized_array<Out>(inputs.size());
			uint i = 0;
			for (const In& input : inputs) {
				Option<Out> o = cb(input);
				if (o.has()) {
					out[i] = o.get();
					++i;
				}
			}
			out._size = i;
			return out;
		}
	};
	// Note: we assume that the output will probably be the same size as the input, or else it's a compile error.
	template <typename Out>
	MapOp<Out> map_op() {
		return MapOp<Out>{*this};
	}

	template<typename Out>
	struct MapOrFail {
		Arena& arena;

		template <typename InCollection, typename In, typename Cb>
		Option<Arr<Out>> worker(const InCollection& inputs, Cb cb) {
			Arr<Out> out = arena.uninitialized_array<Out>(inputs.size());
			uint i = 0;
			for (const In& input : inputs) {
				Option<Out> o = cb(input);
				if (!o.has())
					return {};
				out[i] = o.get();
				++i;
			}
			assert(i == out._size);
			return Option { out };
		}

		template<typename In, typename /*const In& => Option<Out>*/ Cb>
		Option<Arr<Out>> operator()(const Arr<In>& inputs, Cb cb) {
			return worker<Arr<In>, In, Cb>(inputs, cb);
		}

		template<typename In, typename /*const In& => Option<Out>*/ Cb>
		Option<Arr<Out>> operator()(const Grow<In>& inputs, Cb cb) {
			return worker<Grow<In>, In, Cb>(inputs, cb);
		}
	};
	template<typename Out>
	MapOrFail<Out> map_or_fail() {
		return {*this};
	}

	template <typename Out>
	struct MapOrFailReverse {
		Arena& arena;
		template <typename In, typename /*const In&, uninitialized T* => Out*/ Cb>
		Option<Arr<Out>> operator()(const Grow<In>& inputs, Cb cb) {
			Arr<Out> out = arena.uninitialized_array<Out>(inputs.size());
			uint i = 0;
			bool success = true;
			inputs.each_reverse([&](const In& input) {
				if (!success) return;
				success = cb(input, &out[i]);
				++i;
			});
			assert(i == out._size);
			return success ? Option { out } : Option<Arr<Out>> {};
		}
	};
	template <typename Out>
	MapOrFailReverse<Out> map_or_fail_reverse() {
		return {*this};
	}

	template <typename Out>
	struct MapOpWithPrevs {
		Arena& arena;

		template <typename In, typename /*(const In&, const Arr<Out>&) => Option<Out>*/ Cb>
		Arr<Out> operator()(const Arr<In>& inputs, Cb cb) {
			Arr<Out> out = arena.uninitialized_array<Out>(inputs.size());
			out._size = 0;
			uint i = 0;
			for (const In& input : inputs) {
				Option<Out> o = cb(input, out);
				if (o) {
					++out._size;
					out[i] = o.get();
					++i;
				}
			}
			out._size = i;
			return out;
		}
	};
	template <typename Out>
	MapOpWithPrevs<Out> map_op_with_prevs() { return MapOpWithPrevs<Out>{*this}; }

	template <typename T, uint max_size = 8>
	class SmallArrayBuilder {
		friend class Arena;
		ref<Arena> arena;
		MaxSizeVector<max_size, T> v;
		SmallArrayBuilder(ref<Arena> _arena) : arena(_arena) {}

	public:
		void add(T value) { v.push(value); }

		Arr<T> finish() {
			return arena->fill_array<T>()(v.size(), [&](uint i) { return v[i]; });
		}
	};
	template <typename T, uint max_size = 8>
	SmallArrayBuilder<T, max_size> small_array_builder() {
		return { this };
	}

	class StringBuilder {
		Arena& arena;
		ArenaString slice;
		char* ptr;

		friend class Arena;
		StringBuilder(Arena& _arena, ArenaString _slice) : arena(_arena), slice(_slice), ptr(_slice._begin) {}

	public:
		inline StringBuilder& operator<<(char c) {
			assert(ptr != slice._end);
			*ptr = c;
			++ptr;
			return *this;
		}

		StringBuilder& operator<<(uint u);

		StringBuilder& operator<<(StringSlice s);

		inline bool empty() const { return ptr == slice._begin; }

		inline char back() const {
			assert(ptr != slice._begin);
			return *(ptr - 1);
		}

		inline void pop() {
			assert(ptr != slice._begin);
			--ptr;
		}

		ArenaString finish();
	};

	StringBuilder string_builder(size_t max_size);

	ArenaString allocate_slice(size_t size);

	ArenaString str(const StringSlice& slice);
};
