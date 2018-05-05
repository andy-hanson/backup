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

	template <uint size>
	StringSlice get_test_directory(MaxSizeString<size>& buf) {
		char* begin = buf.slice().begin;
		if (!getcwd(begin, buf.slice().size())) throw "todo";
		//find the end
		char* c = begin;
		while (*c != '\0')
			++c;
		c = strip_last_part(buf.slice().begin, c);
		MutableStringSlice m { c, buf.slice().end };
		m << "/test";
		return { begin, m.begin };
	}

	int go() {
		MaxSizeString<128> test_dir;
		int exit_code = test(get_test_directory(test_dir), TestMode::Test);
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
