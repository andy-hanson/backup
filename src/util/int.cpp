#include "./int.h"

#include <limits>
#include "./assert.h"

ushort to_ushort(uint u) {
	assert(u < std::numeric_limits<ushort>::max());
	return ushort(u);
}
