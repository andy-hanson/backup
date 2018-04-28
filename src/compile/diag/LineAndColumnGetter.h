#pragma once

#include "../../util/int.h"
#include "../../util/Alloc.h"
#include "../../util/Vec.h"

struct LineAndColumn { uint line; uint column; };

class LineAndColumnGetter {
	Vec<uint> line_to_pos; // Maps line to position of its start
	LineAndColumnGetter(Vec<uint>&& _line_to_pos) : line_to_pos(std::forward<Vec<uint>>(_line_to_pos)) {}

public:
	static LineAndColumnGetter for_text(StringSlice text);
	LineAndColumn line_and_column_at_pos(uint pos) const;
};
