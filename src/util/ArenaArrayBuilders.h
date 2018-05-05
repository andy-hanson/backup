#pragma once

#include "Arena.h"
#include "./Grow.h"
#include "./List.h"
#include "./Option.h"

template <typename T>
Arr<T> uninitialized_array(Arena& arena, size_t len) {
	return Arr<T> { static_cast<T*>(arena.allocate(sizeof(T) * len)), len };
}

template <typename T>
Arr<T> single_element_array(Arena& arena, T elem) {
	return Arr<T> { arena.put(elem).ptr(), 1 };
}

template <typename T>
struct FillArray {
public:
	template <typename Cb>
	Arr<T> operator()(Arena& arena, size_t len, Cb cb) {
		if (len == 0) return {};
		Arr<T> arr = uninitialized_array<T>(arena, len);
		for (uint i = 0; i < len; ++i)
			arr[i] = cb(i);
		return arr;
	}
};
template <typename T>
FillArray<T> fill_array() { return {}; }

template <typename Out>
struct MapWithPrevs {
	template <typename In, typename /*const In&, const Arr<Out>&, uint => Out*/ Cb>
	Arr<Out> operator()(Arena& arena, const Arr<In>& inputs, Cb cb) {
		if (inputs.empty()) return {};
		Arr<Out> out = uninitialized_array<Out>(arena, inputs.size());
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
MapWithPrevs<Out> map_with_prevs() { return {}; }

template <typename Out>
struct MapOp {
	template <typename In, typename /*const In& => Option<Out>*/ Cb>
	Arr<Out> operator()(Arena& arena, const Arr<In>& inputs, Cb cb) {
		if (inputs.empty()) return {};
		Arr<Out> out = uninitialized_array<Out>(arena, inputs.size());
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
// Note: we assume that the output will probably be the same size as the input, or else it's a compile error.
template <typename Out>
MapOp<Out> map_op() { return {}; }

template<typename Out>
class MapOrFail {
	template <typename InCollection, typename In, typename Cb>
	Option<Arr<Out>> worker(Arena& arena, const InCollection& inputs, Cb cb) {
		Arr<Out> out = uninitialized_array<Out>(arena, inputs.size());
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
	Option<Arr<Out>> operator()(Arena& arena, const Arr<In>& inputs, Cb cb) {
		return worker<Arr<In>, In, Cb>(arena, inputs, cb);
	}

	template<typename In, typename /*const In& => Option<Out>*/ Cb>
	Option<Arr<Out>> operator()(Arena& arena, const Grow<In>& inputs, Cb cb) {
		return worker<Grow<In>, In, Cb>(arena, inputs, cb);
	}
};
template<typename Out>
MapOrFail<Out> map_or_fail() { return {}; }

template <typename Out>
struct MapOrFailReverse {
	template <typename In, typename /*const In&, uninitialized T* => Out*/ Cb>
	Option<Arr<Out>> operator()(Arena& arena, const Grow<In>& inputs, Cb cb) {
		Arr<Out> out = uninitialized_array<Out>(arena, inputs.size());
		uint i = 0;
		bool success = true;
		inputs.each_reverse([&](const In& input) {
			if (!success) return;
			success = cb(input, &out[i]);
			++i;
		});
		assert(i == out.size());
		return success ? Option { out } : Option<Arr<Out>> {};
	}
};
template <typename Out>
MapOrFailReverse<Out> map_or_fail_reverse() { return {}; }

template <typename T, uint max_size = 8>
class SmallArrayBuilder {
	MaxSizeVector<max_size, T> v;

public:
	void add(T value) { v.push(value); }

	Arr<T> finish(Arena& arena) {
		return fill_array<T>()(arena, v.size(), [&](uint i) { return v[i]; });
	}
};

template <typename Out>
struct Mapper {
	template <typename In, typename /*const In& => Out*/ Cb>
	Arr<Out> operator()(Arena& arena, const Arr<In>& in, Cb cb) {
		if (in.empty()) return {};
		return fill_array<Out>()(arena, in.size(), [&](uint i) { return cb(in[i]); });
	}

	template <typename In, typename /*const In& => Out*/ Cb>
	Arr<Out> operator()(Arena& arena, const List<In> in, Cb cb) {
		if (in.empty()) return {};
		uint size = in.size();
		Arr<Out> arr = uninitialized_array<Out>(arena, size);
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
Mapper<Out> map() { return {}; }
