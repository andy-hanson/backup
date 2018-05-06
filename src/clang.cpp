#include "./clang.h"

#include <cstdlib> // std::system
#include "./util/io.h" // delete_file, file_exists
#include "./util/rlimit.h"

namespace {
	const StringSlice CLANG =
	// /home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++
	"clang++ -pedantic -Weverything -Werror -Wno-missing-prototypes "; //-std=c++17-Wno-c++98-compat -Wno-c++98-compat-pedantic
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
	MaxSizeStringStorage<128> temp;
	return exec_command(file_path.get_cstring(temp));
}

void compile_cpp_file(const FileLocator& cpp_file_name, const FileLocator& exe_file_name) {
	delete_file(exe_file_name);
	Arena temp;
	MaxSizeStringStorage<1024> to_exec;
	MaxSizeStringWriter m = to_exec.writer();
	m << CLANG << cpp_file_name << " -o " << exe_file_name << '\0';
	// std::cout << "Running: " << to_exec.slice().begin << std::endl;
	without_limits([&]() { exec_command(to_exec.finish(m).begin()); });
	if (!file_exists(exe_file_name))
		throw "There was a clang error";
}
