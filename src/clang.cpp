#include <iostream>
#include "./clang.h"
#include "./util/io.h"
#include "./util/rlimit.h"

namespace {
	const char* clang =
	// /home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++
	"clang++ -pedantic -Weverything -Werror -Wno-missing-prototypes "; //-std=c++17-Wno-c++98-compat -Wno-c++98-compat-pedantic
	//"g++ -pedantic -Werror -Wextra -Wall ";

	bool is_relative(const std::string& path) {
		return path[0] == '.';
	}

	/*
	void exec_file() {
		const char* command = "...";
		std::cout << "Running: " << command << std::endl;
		FILE* file = popen(command, "r");
		if (!file) throw "todo";

		char data[1000];
		while (fgets(data, sizeof(data), file))
			std::cerr << data << std::endl;

		int err = pclose(file);
		if (err == -1) {
			perror("ohno");
			throw "todo";
		}
		std::cout << "exited?: " << WIFEXITED(err) << ", exit code: " << WEXITSTATUS(err) << std::endl;
	}
	*/

	int exec_command(const std::string& command) {
		return std::system(command.c_str());
	}
}

int execute_file(const std::string& file_path) {
	return exec_command(file_path);
}

void compile_cpp_file(const std::string& cpp_file_name, const std::string& exe_file_name) {
	assert(!is_relative(cpp_file_name) && !is_relative(exe_file_name));
	delete_file(exe_file_name);
	std::string to_exec = std::string(clang);
	to_exec += cpp_file_name;
	to_exec += " -o ";
	to_exec += exe_file_name;
	std::cout << "Running: " << to_exec << std::endl;
	without_limits([&]() { exec_command(to_exec.c_str()); });
	if (!file_exists(exe_file_name)) {
		throw "There was a clang error";
	}
}
