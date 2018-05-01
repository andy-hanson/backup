#include <cassert>
#include <iostream> // cout
#include <unistd.h> // getcwd

#include "./test/test.h"

#include "./host/DocumentProvider.h"
#include "./util/rlimit.h"

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

	std::string test_directory() {
		char cwdbuf[FILENAME_MAX];
		if (!getcwd(cwdbuf, sizeof(cwdbuf))) throw "todo";

		std::string cwd(cwdbuf);
		strip_last_part(cwd);
		cwd += "/test";
		return cwd;
	}

	void go() {
		std::string test_dir = test_directory();
		test(StringSlice { test_dir.begin().base(), test_dir.end().base() }, "simple", TestMode::Test); //"module/circular-dependency"
		std::cout << "done" << std::endl;
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
