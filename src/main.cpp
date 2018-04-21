// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <sys/resource.h>

#include <iostream>
#include <limits>
#include <fstream>

#include "compile/emit/emit.h"
#include "compile/parse/parser.h"
#include "compile/check/check.h"

/*
set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# Must have -Wno-used-but-marked-unused because of https://youtrack.jetbrains.com/issue/CPP-2151
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-used-but-marked-unused -Werror")
*/



namespace {
	void set_limits() {
		// Should take less than 1 second
		rlimit time_limit { /*soft*/ 1, /*hard*/ 1 };
		setrlimit(RLIMIT_CPU, &time_limit);
		// Should not consume more than 2^27 bytes (~125 megabytes)
		rlimit mem_limit { /*soft*/ 1 << 27, /*hard*/ 1 << 27 };
		setrlimit(RLIMIT_AS, &mem_limit);
	}

	__attribute__((unused))
	std::string read_file(const std::string& file_name) {
		std::ifstream i(file_name);
		if (!i) throw "todo";
		return std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>());
	}

	void write_file(const std::string& file_name, const std::string& contents) {
		std::ofstream out(file_name);
		out << contents;
		out.close();
	}

	void delete_file(const std::string& file_name) {
		std::remove(file_name.c_str());
	}

	bool file_exists(const std::string& file_name) {
		//todo: std::filesystem::exists would be nice
		return bool(std::ifstream(file_name));
	}

	const char* clang =
		// Wno-missing-prototypes because we will compile everything as a single file.
		"/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ -std=c++17 -pedantic -Weverything -Werror "
		"-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes ";

	void exec(const std::string& cmd) {
		std::cout << "Running: " << cmd << std::endl;
		std::system(cmd.c_str());
		/*std::array<char, 128> buffer;
		std::cout << "Running: " << cmd << std::endl;
		std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
		if (!pipe) throw "todo";
		std::string result;
		while (!feof(pipe.get())) {
			if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
				result += buffer.data();
		}
		return result;*/
	}

	void compile_cpp_file(const std::string& cpp_file_name, const std::string& exe_file_name) {
		delete_file(exe_file_name);
		exec(std::string(clang) + cpp_file_name + " -o " + exe_file_name);
		if (!file_exists(exe_file_name)) {
			throw "There was a clang error";
		}
	}

	StringSlice to_slice(const std::string& s) {
		return StringSlice { s.begin().base(), s.end().base() };
	}

	ref<Module> parse_and_check_file(StringSlice file_path, Identifier module_name, StringSlice file_content, Arena& arena) {
		Vec<DeclarationAst> declarations = parse_file(file_content, arena);
		return check(file_path, module_name, declarations, arena);
	}

	std::string get_compiled_file(const std::string& file_name) {
		std::string nz_file_name = file_name + ".nz";
		std::string nz_file_content = read_file(file_name + ".nz");
		Arena arena;
		Identifier module_name = Identifier{arena.str("foo")}; //TODO
		Vec<ref<Module>> modules;
		modules.push_back(parse_and_check_file(to_slice(nz_file_name), module_name, to_slice(nz_file_content), arena));
		return emit(modules);
	}

	void compile_nz_file(const std::string& file_name) {
		std::string emitted = get_compiled_file(file_name);
		std::string cpp = file_name + ".cpp";
		write_file(cpp, emitted);
		compile_cpp_file(cpp, file_name + ".exe");
	}

	__attribute__((unused))
	void run(const std::string& file_name) {
		exec(file_name + ".exe");
	}

	__attribute__((unused))
	void compile_and_run(const std::string& file_name) {
		compile_nz_file(file_name);
		run(file_name);
	}
}

int main() {
	set_limits();

	std::cout << get_compiled_file("../stdlib/auto") << std::endl;


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
