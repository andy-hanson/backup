#pragma once

#include "../../util/int.h"
#include "../../util/StringSlice.h"

struct SourceRange {
	ushort begin;
	ushort end;
	inline SourceRange(ushort _begin, ushort _end) : begin(_begin), end(_end) {
		assert(end >= begin);
	}

	inline static SourceRange inner_slice(StringSlice outer, StringSlice inner) {
		assert(inner.end() <= outer.end());
		return { to_ushort(to_unsigned(inner.begin() - outer.begin())), to_ushort(to_unsigned(inner.end() - outer.begin())) };
	}
};
