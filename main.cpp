// clang++-5.0 -std=c++17 -pedantic -Wall -Wconversion -Werror -Ofast main.cpp

#include <sys/resource.h>

#include <iostream>
#include <limits>

#include "emit.h"
#include "parse/parser.h"

namespace {
	void set_limits() {
		// Should take less than 1 second
		rlimit time_limit { /*soft*/ 1, /*hard*/ 1 };
		setrlimit(RLIMIT_CPU, &time_limit);
		// Should not consume more than 2^27 bytes (~125 megabytes)
		rlimit mem_limit { /*soft*/ 1 << 27, /*hard*/ 1 << 27 };
		setrlimit(RLIMIT_AS, &mem_limit);
	}

	const StringSlice sample __attribute__((unused)) { R"EOF(
c++ struct String
	const char* begin;
	const char* end;
	template <size_t N>
	// Note: the char array will include a '\0', but we don't want that included in the slice.
	constexpr String(char const (&c)[N]) : StringSlice(c, c + N - 1) { static_assert(N > 0); }

c++ struct Bool = bool
c++ struct Char = char
c++ struct Nat = ulong

c++ fun Nat literal(String s)
	if (s.size() == 0) throw "todo";
	const char *begin = s.begin();
	while (*begin == '0') ++begin;
	ulong power = 1;
	ulong result = 0;
	for (const char *r = s.end() - 1; ; --r) {
		char c = *r;
		if (c < '0' || c > '9') throw "todo";
		ulong digit = ulong(c - '0');
		if (digit > std::numeric_limits<ulong>::max() / power) throw "todo";
		ulong dp = digit * power;
		if (std::numeric_limits<ulong>::max() - result < dp) throw "todo";
		result += dp;
		if (r == begin) return result;
		if (power > std::numeric_limits<ulong>::max() / 10) throw "todo";
		power *= 10;
	}

fun Nat f()
	0

)EOF"
	};
}


namespace {

	/*	if (begin == "+") {
			++begin;
		} else if (begin == "-") {
			power *= -1;
			++begin;
		}
	*/

	//const ulong powers[20] = { 1, 10, 100, 1000, 10000, 100000, };


	/*int string_to_int(StringSlice s) {
		if (s.size() == 0) throw "todo";
		const char *begin = s.begin();
		while (*begin == '0') ++begin;
		int power = 1;
		int result = 0;
		for (const char *r = s.end() - 1; r != begin; --r) {
			result += safe_mul(to_digit(*r), power);
			power = safe_mul(power, 10);
		}
		return safe_add(result, safe_mul(to_digit(*begin), power));
	}*/
}

int main() {
	set_limits();

	Module m = parse_file(sample);
	std::string emitted = emit(m);
	std::cout << emitted << std::endl;
	std::cout << "done" << std::endl;
}


/*
c++ struct UnsafeInt = int
c++ fun UnsafeInt -(UnsafeInt a, UnsafeInt b)
	return a - b;
c++ fun UnsafeInt *(UnsafeInt a, UnsafeInt b)
	return a * b;
c++ fun UnsafeInt /(UnsafeInt a, UnsafeInt b)
	return a / b;


c++ fun UnsafeInt +(UnsafeInt a, UnsafeInt b)
	return a + b;

fun Int +(Int a, Int b)
	sum = a.u + b.u
	Int sum

struct Int
	UnsafeInt u
*/
