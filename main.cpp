// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <sys/resource.h>

#include <iostream>

#include "emit.h"
#include "parse/parser.h"

namespace {
	void set_limits() {
		// Should take less than 1 second
		rlimit time_limit { /*soft*/ 1, /*hard*/ 1 };
		setrlimit(RLIMIT_CPU, &time_limit);
		// Should not consume more than 2^27 bytes (~125 megabytes)
		rlimit mem_limit { /*soft*/ 1 << 27, /*hard*/ 1 << 27 };
		setrlimit(RLIMIT_AS, &mem_limit);
	}

	const char* sample = R"EOF(
c++ struct Int = int
c++ struct Bool = bool

fun<T> T f(T t)
	t

fun Bool g(Bool i)
	i f

)EOF";
}

int main() {
	set_limits();
	Module m = parse_file(sample);
	std::string emitted = emit(m);
	std::cout << emitted << std::endl;
	std::cout << "done" << std::endl;
}


/*
c++ struct UnsafeInt = int
c++ fun UnsafeInt -(UnsafeInt a, UnsafeInt b)
	return a - b;
c++ fun UnsafeInt *(UnsafeInt a, UnsafeInt b)
	return a * b;
c++ fun UnsafeInt /(UnsafeInt a, UnsafeInt b)
	return a / b;


c++ fun UnsafeInt +(UnsafeInt a, UnsafeInt b)
	return a + b;

fun Int +(Int a, Int b)
	sum = a.u + b.u
	Int sum

struct Int
	UnsafeInt u
*/
