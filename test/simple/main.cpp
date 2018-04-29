#include <assert.h>

typedef void Void;

typedef bool Bool;

Bool _true();

// Main function
Void run();

Bool _true() {
	return true;
}

// Main function
Void run() {
	assert(_true());
}

int main() { run(); }
