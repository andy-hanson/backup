#include "./LineAndColumnGetter.h"

namespace {
	uint mid(uint a, uint b) {
		return (a + b) / 2;
	}
}

LineAndColumnGetter LineAndColumnGetter::for_text(const StringSlice& text, Arena& arena) {
	Arena::SmallArrayBuilder<uint, 1024> lines = arena.small_array_builder<uint, 1024>();
	lines.add(0); // Line 0 starts at text index 0
	for (uint i = 0; i != text.size(); ++i)
		if (text[i] == '\n')
			lines.add(i + 1);
	return LineAndColumnGetter { lines.finish() };
}

LineAndColumn LineAndColumnGetter::line_and_column_at_pos(uint pos) const {
	uint low_line = 0; // inclusive
	uint high_line = to_uint(line_to_pos.size()); // exclusive

	while (low_line < high_line - 1) {
		uint middle_line = mid(low_line, high_line);
		uint middle_pos = line_to_pos[middle_line];
		if (pos == middle_pos)
			return { middle_line, 0 };
		else if (pos < middle_pos)
			high_line = middle_line;
		else
			low_line = middle_line;
	}

	uint line = low_line;
	uint line_start = line_to_pos[line];
	assert(pos >= line_start && line == line_to_pos.size() - 1 ||  pos <= line_to_pos[line + 1]);
	return { line, pos - line_start };
}
