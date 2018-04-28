#include <assert.h>

typedef void Void;

typedef bool Bool;

Bool _true();

Void run();

Bool _true() {
	return true;
}

Void run() {
	assert(_true());
}


int main() { run(); }
