#pragma once

#include "./Arena.h"
#include "./NonEmptyList.h"
#include "./Slice.h"

template <typename Out>
struct map_to_non_empty_list {
	template <typename In, typename /*const In& => Out*/ Cb>
	inline NonEmptyList<Out> operator()(Arena& arena, const Slice<In>& inputs, Cb cb) {
		assert(!inputs.is_empty());
		NonEmptyList<Out> list { cb(inputs[inputs.size() - 1]) };
		if (inputs.size() != 1)
			for (uint i = inputs.size() - 2; ; --i) {
				list.prepend(cb(inputs[i]), arena);
				if (i == 0) break;
			}
		return list;
	}
};
