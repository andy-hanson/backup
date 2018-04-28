// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>

#include "./cmd.h"
#include "./test/test.h"

#include "./host/DocumentProvider.h"
#include "./util/io.h"
#include "./util/rlimit.h"
#include "compile/model/model.h"
#include "compile/parse/ast.h"
#include "util/collection_util.h"

namespace {
	void strip_last_part(std::string& path) {
		//size_t slash_index = path.rfind('/');
		//if (slash_index == std::string::npos)
		//	return;
		assert(!path.empty());
		while (path.back() != '/') {
			path.pop_back();
			assert(!path.empty());
		}
		path.pop_back();
	}

	__attribute__((unused))
	std::string test_directory() {
		char cwdbuf[FILENAME_MAX];
		if (!getcwd(cwdbuf, sizeof(cwdbuf))) throw "todo";

		std::string cwd(cwdbuf);
		strip_last_part(cwd);
		cwd += "/test";
		return cwd;
	}

	__attribute__((unused))
	void go() {
		//std::cout << compile_to_string("../stdlib/auto") << std::endl;
		//std::cout << "done" << std::endl;
		test(test_directory(), TestMode::Accept);

		//try_exec();


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
	//"/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ /home/andy/CLionProjects/oohoo/test/a/main.cpp -o /home/andy/CLionProjects/oohoo/test/a/main.exe"
	//std::string command = "../cc.py";
	//command += " /home/andy/CLionProjects/oohoo/test/a/main.cpp -o /home/andy/CLionProjects/oohoo/test/a/main.exe";

	set_limits(); //This makes subprocesses crash!!!!!!!!!!!!!!!
	unset_limits();
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
