// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <cassert>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <sys/resource.h>
#include <vector>

#include "./cmd.h"
#include "./test/test.h"

namespace {
	void on_signal(int sig) {
		if (sig == SIGXCPU) std::cerr << "Hit CPU time limit -- probably an infinite loop somewhere" << std::endl;
		exit(sig);
	}

	void set_limits() {
		ulong time_limit = 1;
		// Should take less than 1 second
		rlimit time_limits { /*soft*/ time_limit, /*hard*/ time_limit + 1 };
		setrlimit(RLIMIT_CPU, &time_limits);
		ulong mem_limit = 1 << 25;
		// Should not consume more than 2^27 bytes (~125 megabytes)
		rlimit mem_limits { /*soft*/ mem_limit, /*hard*/ mem_limit + 1 };
		setrlimit(RLIMIT_AS, &mem_limits);

		signal(SIGXCPU, on_signal);
	}

	void go() {
		int end = 0;
		int begin = 0;
		std::cout << begin << std::endl;

		//std::cout << compile_to_string("../stdlib/auto") << std::endl;
		//std::cout << "done" << std::endl;

		//test();


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
}

int main() {
	set_limits();
	try {
		go();
	} catch (std::bad_alloc a) {
		std::cerr << "Bad allocation -- probably due to memory limit" << std::endl;
		throw a;
	}

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
