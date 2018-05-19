#include <assert.h>

struct Void {
	
};

typedef bool Bool;

void _true(Bool* _ret);
void _main(Void* _ret);

void _true(Bool* _ret) {*_ret = true;
}

void _main(Void* _ret) {
	Bool b;
	_true(&b);
	assert(b);
}

int main() { Void v; _main(&v); }
