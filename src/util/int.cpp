#include "./int.h"

#include "./assert.h"

ushort to_ushort(uint u) {
	assert(u <= ushort(-1));
	return ushort(u);
}
