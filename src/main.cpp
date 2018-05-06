#include <iostream> // cout
#include <unistd.h> // getcwd

#include "./test/test.h"

#include "./host/DocumentProvider.h"
#include "./util/rlimit.h"
#include "util/store/collection_util.h"

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
		if (!getcwd(begin, buf.remaining_capacity())) todo();
		buf.cur = strip_last_part(buf.cur, get_end(begin));
		buf << "/test";
	}

	StringSlice slice(const StringSlice& s, uint begin, uint end) {
		assert(begin < end && end <= s.size());
		return { s.begin() + begin, s.begin() + end };
	}

	bool is_substring(const StringSlice& haystack, const StringSlice& needle) {
		if (needle.size() > haystack.size()) return false;
		//TODO:PERF?
		uint max_i = to_unsigned(long(haystack.size()) - needle.size()) + 1; // + 1 because this is the first value we will *not* test
		for (uint i = 0; i != max_i; ++i) {
			if (slice(haystack, i, i + needle.size()) == needle) {
				return true;
			}
		}
		return false;
	}

	bool is_uppercase(char c) {
		return 'A' <= c && c <= 'Z';
	}

	char to_lowercase(char c) {
		return is_uppercase(c) ? c + ('a' - 'A') : c;
	}

	bool is_lowercase(const StringSlice& s) {
		return every(s, [](char c) { return !is_uppercase(c); });
	}

	void to_lowercase(const StringSlice& s, MaxSizeStringWriter& out) {
		for (char c : s)
			out << to_lowercase(c);
	}

	struct SubstrTestFilter : public TestFilter {
		const StringSlice& substr;
		SubstrTestFilter(const StringSlice& s) : substr{s} {
			assert(is_lowercase(s));
		}

		bool should_test(const StringSlice& directory) const override {
			MaxSizeString<128> lower = MaxSizeString<128>::make([&](MaxSizeStringWriter& w) { to_lowercase(directory, w); });
			return is_substring(lower.slice(), substr);
		}
	};

	int go() {
		MaxSizeString<128> test_dir = MaxSizeString<128>::make([&](MaxSizeStringWriter& w) { get_test_directory(w); });
		auto filter = SubstrTestFilter { "simple" }; // EveryTestFilter{};
		int exit_code = test(test_dir.slice(), filter, TestMode::Accept);
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
