// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <sys/resource.h>

#include <iostream>
#include <fstream>

#include "./cmd.h"

namespace {
	void set_limits() {
		// Should take less than 1 second
		rlimit time_limit { /*soft*/ 1, /*hard*/ 1 };
		setrlimit(RLIMIT_CPU, &time_limit);
		// Should not consume more than 2^27 bytes (~125 megabytes)
		rlimit mem_limit { /*soft*/ 1 << 27, /*hard*/ 1 << 27 };
		setrlimit(RLIMIT_AS, &mem_limit);
	}


}

int main() {
	set_limits();

	std::cout << compile_to_string("../stdlib/auto") << std::endl;
	std::cout << "done" << std::endl;


	//run("../stdlib/auto");
	//compile_and_run("../stdlib/auto");
	//std::string out = exec(std::string(clang) + " ../test.cpp -o ../test.exe");
	//std::cout << out << std::endl;

	//std::string f = read_file("../stdlib/auto.nz");
	//std::cout << f << std::endl;

	/*
	Module m = parse_file(StringSlice { f.begin().base(), f.end().base() });
	std::string emitted = emit(m);
	std::cout << emitted << std::endl;
	std::cout << "done" << std::endl;
	*/
}


/*
cpp struct UnsafeInt = int
cpp fun UnsafeInt -(UnsafeInt a, UnsafeInt b)
	return a - b;
cpp fun UnsafeInt *(UnsafeInt a, UnsafeInt b)
	return a * b;
cpp fun UnsafeInt /(UnsafeInt a, UnsafeInt b)
	return a / b;


cpp fun UnsafeInt +(UnsafeInt a, UnsafeInt b)
	return a + b;

fun Int +(Int a, Int b)
	sum = a.u + b.u
	Int sum

struct Int
	UnsafeInt u
*/
