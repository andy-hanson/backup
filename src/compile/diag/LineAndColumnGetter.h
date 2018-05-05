#pragma once

#include "../../util/int.h"
#include "../../util/Arena.h"
#include "../../util/Slice.h"
#include "../../util/StringSlice.h"

struct LineAndColumn { uint line; uint column; };

class LineAndColumnGetter {
	Slice<uint> line_to_pos; // Maps line to source index of its first character
	inline LineAndColumnGetter(Slice<uint> _line_to_pos) : line_to_pos(_line_to_pos) {}

public:
	static LineAndColumnGetter for_text(const StringSlice& text, Arena& arena);
	LineAndColumn line_and_column_at_pos(uint pos) const;
};
