#include "./clang.h"

#include <cstdlib> // std::system
#include "./util/io.h" // delete_file, file_exists
#include "./util/rlimit.h"

namespace {
	const StringSlice CLANG =
	// /home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++
	"clang++ -pedantic -Weverything -Werror -Wno-missing-prototypes -Wno-unused-parameter -Wno-missing-noreturn "; //-std=c++17-Wno-c++98-compat -Wno-c++98-compat-pedantic
	//"g++ -pedantic -Werror -Wextra -Wall ";

	/*
	void exec_file() {
		const char* command = "...";
		std::cout << "Running: " << command << std::endl;
		FILE* file = popen(command, "r");
		if (!file) todo();

		char data[1000];
		while (fgets(data, sizeof(data), file))
			std::cerr << data << std::endl;

		int err = pclose(file);
		if (err == -1) {
			perror("ohno");
			todo();
		}
		std::cout << "exited?: " << WIFEXITED(err) << ", exit code: " << WEXITSTATUS(err) << std::endl;
	}
	*/

	int exec_command(const char* command) {
		return std::system(command);
	}
}

int execute_file(const FileLocator& file_path) {
	MaxSizeString<128> temp = MaxSizeString<128>::make([&](MaxSizeStringWriter& w) { w << file_path << '\0'; });
	return exec_command(temp.slice().begin());
}

void compile_cpp_file(const FileLocator& cpp_file_name, const FileLocator& exe_file_name) {
	delete_file(exe_file_name);
	Arena temp;
	MaxSizeString<256> o = MaxSizeString<256>::make([&](MaxSizeStringWriter& m) {
		m << CLANG << cpp_file_name << " -o " << exe_file_name << '\0';
	});
	// std::cout << "Running: " << to_exec.slice().begin << std::endl;
	without_limits([&]() { exec_command(o.slice().begin()); });
	if (!file_exists(exe_file_name))
		throw "There was a clang error";
}
