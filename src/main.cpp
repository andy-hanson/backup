#include <iostream> // cout
#include <unistd.h> // getcwd

#include "./test/test.h"

#include "./host/DocumentProvider.h"
#include "./util/rlimit.h"

namespace {
	char* strip_last_part(char* begin, char* end) {
		while (end != begin && *end != '/')
			--end;
		return end;
	}

	char* get_end(char* begin) {
		char* c = begin;
		while (*c != '\0')
			++c;
		return c;
	}

	void get_test_directory(MaxSizeStringWriter& buf) {
		char* begin = buf.cur;
		if (!getcwd(begin, buf.capacity())) todo();
		buf.cur = strip_last_part(buf.cur, get_end(begin));
		buf << "/test";
	}

	int go() {
		MaxSizeStringStorage<128> test_dir;
		MaxSizeStringWriter slice = test_dir.writer();
		get_test_directory(slice);
		int exit_code = test(test_dir.finish(slice), TestMode::Test);
		std::cout << "done" << std::endl;
		return exit_code;
	}
}

int main() {
	set_limits();
	int exit_code;
	try {
		exit_code = go();
	} catch (std::bad_alloc a) {
		std::cerr << "Bad allocation -- probably due to memory limit" << std::endl;
		throw a;
	}
	return exit_code;
}
