#include "./cmd.h"

#include <iostream>
#include "./compile/compile.h"
#include "./emit/emit.h"
#include "./util/io.h"

namespace {
	const char* clang =
		// Wno-missing-prototypes because we will compile everything as a single file.
		"/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ -std=c++17 -pedantic -Weverything -Werror "
		"-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-prototypes ";

	void exec(const std::string& cmd) {
		std::cout << "Running: " << cmd << std::endl;
		std::system(cmd.c_str());
	}

	void compile_cpp_file(const std::string& cpp_file_name, const std::string& exe_file_name) {
		delete_file(exe_file_name);
		exec(std::string(clang) + cpp_file_name + " -o " + exe_file_name);
		if (!file_exists(exe_file_name)) {
			throw "There was a clang error";
		}
	}
}

void run(const std::string& file_name) {
	exec(file_name + ".exe");
}

void compile_and_run(const std::string& file_name) {
	compile_nz_file(file_name);
	run(file_name);
}

std::string compile_to_string(const std::string& file_name) {
	CompiledProgram program;
	compile(program, file_name);
	return emit(program.modules);
}

void compile_nz_file(const std::string& file_name) {
	std::string emitted = compile_to_string(file_name);
	std::string cpp = file_name + ".cpp";
	write_file(cpp, emitted);
	compile_cpp_file(cpp, file_name + ".exe");
}


