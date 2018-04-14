#include <cstring>
#include "StringSlice.h"

bool operator==(StringSlice a, StringSlice b) {
	return std::equal(a._begin, a._end, b._begin, b._end);
}
