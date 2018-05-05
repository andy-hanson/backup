#pragma once

#include "./MaxSizeVector.h"
#include "./StringSlice.h"
#include "./ptr.h"
#include "./Option.h"
#include "./Slice.h"
#include "Grow.h"

class Arena {
	friend class StringBuilder; //TODO
	void* alloc_begin;
	void* alloc_next;
	void* alloc_end;

public:
	Arena();
	Arena(const Arena& other) = delete;
	void operator=(const Arena& other) = delete;
	~Arena();

	void* allocate(size_t n_bytes);

	template <typename T>
	ref<T> allocate_uninitialized() {
		return static_cast<T*>(allocate(sizeof(T)));
	}

	template <typename T>
	Arr<T> uninitialized_array(size_t len) {
		return Arr<T>(static_cast<T*>(allocate(sizeof(T) * len)), len);
	}

	template <typename T>
	ref<T> put(T value) {
		ref<T> ptr = allocate_uninitialized<T>();
		*ptr.ptr() = value;
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
};




template <typename Out>
struct Mapper {
	template <typename In, typename /*const In& => Out*/ Cb>
	Arr<Out> operator()(Arena& arena, const Arr<In>& in, Cb cb) {
		if (in.empty()) return {};
		return arena.fill_array<Out>()(in.size(), [&](uint i) { return cb(in[i]); });
	}

	template <typename InCollection, typename /*const In& => Out*/ Cb>
	Arr<Out> operator()(Arena& arena, const InCollection& in, Cb cb) {
		if (in.empty()) return {};
		uint size = in.size();
		Arr<Out> arr = arena.uninitialized_array<Out>(size);
		uint i = 0;
		for (const auto& input : in) {
			arr[i] = cb(input);
			++i;
		}
		assert(i == size);
		return arr;
	}
};
template <typename Out>
Mapper<Out> map() { return {}; }
