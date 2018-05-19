#pragma once

#include "../Option.h"
#include "./Arena.h"
#include "./MaxSizeVector.h"
#include "./Slice.h"

//TODO:MOVE
template <typename T, typename U>
struct Pair {
	T first;
	U second;
};

template <typename T>
inline Slice<T> single_element_slice(Arena& arena, T elem) {
	return Slice<T> { arena.put(elem).ptr(), 1 };
}

template <typename T>
inline Slice<T> uninitialized_array(Arena& arena, uint size) {
	return Slice<T> { static_cast<T*>(arena.allocate(sizeof(T) * size)), size };
}

template <typename T>
struct fill_array {
	template <typename /*() => T*/ Cb>
	inline Slice<T> operator()(Arena& arena, uint size, Cb cb) {
		if (size == 0) return {};
		Slice<T> arr = uninitialized_array<T>(arena, size);
		for (uint i = 0; i != size; ++i) {
			T value = cb(i);
			arr[i] = value;
		}
		return arr;
	}
};

template <typename T, typename U>
struct fill_two_arrays {
	template <typename /*() => Pair<T, U>*/ Cb>
	inline Pair<Slice<T>, Slice<U>> operator()(Arena& arena_t, Arena& arena_u, uint size, Cb cb) {
		if (size == 0) return { {}, {} };

		Slice<T> ts = uninitialized_array<T>(arena_t, size);
		Slice<U> us = uninitialized_array<U>(arena_u, size);
		for (uint i = 0; i != size; ++i) {
			Pair<T, U> values = cb(i);
			ts[i] = values.first;
			us[i] = values.second;
		}
		return { ts, us };
	}
};

template <typename Out>
struct map_with_prevs {
	template <typename In, typename /*const In&, const Arr<Out>&, uint => Out*/ Cb>
	inline Slice<Out> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
		if (inputs.is_empty()) return {};
		Slice<Out> out = uninitialized_array<Out>(arena, inputs.size());
		uint i = 0;
		for (const In& input : inputs) {
			Out o = cb(input, out.slice(0, i), i);
			out[i] = o;
			++i;
		}
		assert(i == out.size());
		return out;
	}
};

template <typename Out>
struct map_op {
	// Note: we assume that the output will probably be the same size as the input, or else it's a compile error.
	template <typename In, typename /*const In& => Option<Out>*/ Cb>
	inline Slice<Out> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
		if (inputs.is_empty()) return {};
		Slice<Out> out = uninitialized_array<Out>(arena, inputs.size());
		uint i = 0;
		for (const In& input : inputs) {
			Option<Out> o = cb(input);
			if (o.has()) {
				out[i] = o.get();
				++i;
			}
		}
		// None is a failure case, so don't worry about the lost memory if we didn't use all of 'out'
		return out.slice(0, i);
	}
};

template<typename Out>
class map_or_fail {
	template <typename InCollection, typename In, typename Cb>
	inline Option<Slice<Out>> worker(Arena& arena, const InCollection& inputs, Cb cb) {
		Slice<Out> out = uninitialized_array<Out>(arena, inputs.size());
		uint i = 0;
		for (const In& input : inputs) {
			Option<Out> o = cb(input);
			if (!o.has())
				return {};
			out[i] = o.get();
			++i;
		}
		assert(i == out.size());
		return Option { out };
	}

public:
	template<typename In, typename /*const In& => Option<Out>*/ Cb>
	inline Option<Slice<Out>> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
		return inputs.is_empty() ? Option<Slice<Out>> { Slice<Out> {} } : worker<Slice<In>, In, Cb>(arena, inputs, cb);
	}
};

template <typename Out>
struct map {
	template <typename Collection, typename /*const In& => Out*/ Cb>
	inline Slice<Out> operator()(Arena& arena, const Collection& in, Cb cb) {
		if (in.is_empty()) return {};
		uint size = in.size();
		Slice<Out> arr = uninitialized_array<Out>(arena, size);
		uint i = 0;
		for (const typename Collection::value_type& input : in) {
			arr[i] = cb(input);
			++i;
		}
		assert(i == size);
		return arr;
	}
};

template <typename T>
Slice<T> clone(const Slice<T>& in, Arena& arena) {
	return map<T>{}(arena, in, [](const T& t) { return t; });
}

template <typename Out>
struct map_with_first {
	template <typename In, typename Cb>
	inline Slice<Out> operator()(Arena& arena, Option<Out> first, const Slice<In>& in, Cb cb) {
		uint offset = first.has() ? 1 : 0;
		uint size = offset + in.size();
		if (size == 0) return {};
		Slice<Out> arr = uninitialized_array<Out>(arena, size);
		if (first.has())
			arr[0] = first.get();
		uint i = 0;
		for (const In& input : in) {
			arr[i + offset] = cb(input);
			++i;
		}
		assert(i == in.size());
		return arr;
	}
};

template <typename T, typename U>
struct map_to_two_arrays {
	template <typename Collection, typename /*() => Pair<T, U>*/ Cb>
	inline Pair<Slice<T>, Slice<U>> operator()(Arena& arena_t, Arena& arena_u, const Collection& in, Cb cb) {
		if (in.is_empty()) return { {}, {} };

		uint size = in.size();
		Slice<T> ts = uninitialized_array<T>(arena_t, size);
		Slice<U> us = uninitialized_array<U>(arena_u, size);
		uint i = 0;
		for (const typename Collection::value_type& input : in) {
			Pair<T, U> values = cb(input);
			ts[i] = values.first;
			us[i] = values.second;
		}
		assert(i == size);
		return { ts, us };
	}
};

template <uint capacity, typename T>
Slice<T> to_arena(const MaxSizeVector<capacity, T>& m, Arena& arena) {
	return fill_array<T>()(arena, m.size(), [&](uint i) { return m[i]; });
}
