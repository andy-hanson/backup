#pragma once

#include "../Option.h"
#include "./Arena.h"
#include "./MaxSizeVector.h"
#include "./Slice.h"

template <typename T>
Slice<T> uninitialized_array(Arena& arena, uint len) {
	return Slice<T> { static_cast<T*>(arena.allocate(sizeof(T) * len)), len };
}

template <typename T>
struct fill_array {
public:
	template <typename /*() => T*/ Cb>
	Slice<T> operator()(Arena& arena, uint len, Cb cb) {
		if (len == 0) return {};
		Slice<T> arr = uninitialized_array<T>(arena, len);
		for (uint i = 0; i < len; ++i) {
			T value = cb(i);
			arr[i] = value;
		}
		return arr;
	}
};

template <typename Out>
struct map_with_prevs {
	template <typename In, typename /*const In&, const Arr<Out>&, uint => Out*/ Cb>
	Slice<Out> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
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
	Slice<Out> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
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
	Option<Slice<Out>> worker(Arena& arena, const InCollection& inputs, Cb cb) {
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
	Option<Slice<Out>> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
		return worker<Slice<In>, In, Cb>(arena, inputs, cb);
	}
};

template <typename T, uint max_size = 8>
class SmallArrayBuilder {
	MaxSizeVector<max_size, T> v;

public:
	void add(T value) { v.push(value); }

	Slice<T> finish(Arena& arena) {
		return fill_array<T>()(arena, v.size(), [&](uint i) { return v[i]; });
	}
};

template <typename Out>
struct map {
	template <typename Collection, typename /*const In& => Out*/ Cb>
	Slice<Out> operator()(Arena& arena, const Collection& in, Cb cb) {
		if (in.is_empty()) return {};
		uint size = in.size();
		Slice<Out> arr = uninitialized_array<Out>(arena, size);
		uint i = 0;
		for (const auto& input : in) {
			arr[i] = cb(input);
			++i;
		}
		assert(i == size);
		return arr;
	}
};
