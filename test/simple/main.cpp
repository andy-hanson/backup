#include <assert.h>

struct Void {};

typedef bool Bool;

void _true(Bool* _ret);

void pointer__to(Bool* _ret, Bool* b);

// Main function
void run(Void* _ret);

void _true(Bool* _ret) {
	*_ret = true;
}

void pointer__to(Bool* _ret, Bool* b) {
	return b;
}

// Main function
void run(Void* _ret) {
	Bool b;
	_true(&b);
	Bool bp;
	pointer__to(&bp, b);
	assert(bp);
	
}

int main() { Void v; run(&v); }
